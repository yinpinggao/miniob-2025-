/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/07/05.
//

#include "sql/expr/expression.h"
#include "common/fulltext/jieba_util.h"
#include "sql/expr/tuple.h"
#include "sql/expr/arithmetic_operator.hpp"
#include "storage/table/table.h"
#include "storage/index/fulltext_index.h"
#include "storage/record/record.h"
#include <cmath>

#include "sql/stmt/select_stmt.h"
#include "sql/operator/logical_operator.h"
#include "sql/operator/physical_operator.h"
#include "sql/optimizer/logical_plan_generator.h"
#include "sql/optimizer/physical_plan_generator.h"
#include "common/lang/defer.h"

#define check_type(str, rule)           \
  if (0 == strcasecmp(type_str, str)) { \
    type = rule;                        \
    return RC::SUCCESS;                 \
  }

RC FieldExpr::get_value(const Tuple &tuple, Value &value)
{
  return tuple.find_cell(TupleCellSpec(table_name(), field_name(), table_alias_.c_str()), value);
}

bool FieldExpr::equal(const Expression &other) const
{
  if (this == &other) {
    return true;
  }
  if (other.type() != ExprType::FIELD) {
    return false;
  }
  const auto &other_field_expr = static_cast<const FieldExpr &>(other);
  return table_name() == other_field_expr.table_name() && field_name() == other_field_expr.field_name();
}

// TODO: 在进行表达式计算时，`chunk` 包含了所有列，因此可以通过 `field_id` 获取到对应列。
// 后续可以优化成在 `FieldExpr` 中存储 `chunk` 中某列的位置信息。
RC FieldExpr::get_column(Chunk &chunk, Column &column)
{
  if (pos_ != -1) {
    column.reference(chunk.column(pos_));
  } else {
    column.reference(chunk.column(field().meta()->field_id()));
  }
  return RC::SUCCESS;
}

bool ValueExpr::equal(const Expression &other) const
{
  if (this == &other) {
    return true;
  }
  if (other.type() != ExprType::VALUE) {
    return false;
  }
  const auto &other_value_expr = static_cast<const ValueExpr &>(other);
  return value_.compare(other_value_expr.get_value()) == 0;
}

RC ValueExpr::get_value(const Tuple &tuple, Value &value)
{
  value = value_;
  return RC::SUCCESS;
}

RC ValueExpr::get_column(Chunk &chunk, Column &column)
{
  column.init(value_);
  return RC::SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
CastExpr::CastExpr(unique_ptr<Expression> child, AttrType cast_type) : child_(std::move(child)), cast_type_(cast_type)
{}

CastExpr::~CastExpr() {}

RC CastExpr::cast(const Value &value, Value &cast_value) const
{
  RC rc = RC::SUCCESS;
  if (this->value_type() == value.attr_type()) {
    cast_value = value;
    return rc;
  }
  rc = Value::cast_to(value, cast_type_, cast_value);
  return rc;
}

RC CastExpr::get_value(const Tuple &tuple, Value &result)
{
  Value value;
  RC    rc = child_->get_value(tuple, value);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return cast(value, result);
}

RC CastExpr::try_get_value(Value &result) const
{
  Value value;
  RC    rc = child_->try_get_value(value);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return cast(value, result);
}

////////////////////////////////////////////////////////////////////////////////

ComparisonExpr::ComparisonExpr(CompOp comp, Expression *left, Expression *right)
    : comp_(comp), left_(std::unique_ptr<Expression>(left)), right_(std::unique_ptr<Expression>(right))
{}

ComparisonExpr::ComparisonExpr(CompOp comp, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : comp_(comp), left_(std::move(left)), right_(std::move(right))
{}

ComparisonExpr::~ComparisonExpr() = default;

RC ComparisonExpr::compare_value(const Value &left, const Value &right, bool &result) const
{
  if (comp_ == IS_OP) {
    if (right.attr_type() != AttrType::NULLS) {
      return RC::NOT_NULL_AFTER_IS;
    }
    result = left.is_null();
    return RC::SUCCESS;
  }
  if (comp_ == IS_NOT_OP) {
    if (right.attr_type() != AttrType::NULLS) {
      return RC::NOT_NULL_AFTER_IS;
    }
    result = !left.is_null();
    return RC::SUCCESS;
  }
  if (left.is_null() || right.is_null()) {
    result = false;
    return RC::SUCCESS;
  }

  RC rc = RC::SUCCESS;

  if (comp_ == LIKE_OP || comp_ == NOT_LIKE_OP) {
    ASSERT(left.is_str() && right.is_str(), "LIKE ONLY SUPPORT STRING TYPE!");
    result = comp_ == LIKE_OP ? left.LIKE(right) : !left.LIKE(right);
    return rc;
  }

  int cmp_result = left.compare(right);
  result         = false;
  switch (comp_) {
    case EQUAL_TO: {
      result = (0 == cmp_result);
    } break;
    case LESS_EQUAL: {
      result = (cmp_result <= 0);
    } break;
    case NOT_EQUAL: {
      result = (cmp_result != 0);
    } break;
    case LESS_THAN: {
      result = (cmp_result < 0);
    } break;
    case GREAT_EQUAL: {
      result = (cmp_result >= 0);
    } break;
    case GREAT_THAN: {
      result = (cmp_result > 0);
    } break;
    default: {
      LOG_WARN("unsupported comparison. %d", comp_);
      rc = RC::INTERNAL;
    } break;
  }

  return rc;
}

RC ComparisonExpr::try_get_value(Value &cell) const
{
  if (left_->type() == ExprType::VALUE && right_->type() == ExprType::VALUE) {
    ValueExpr   *left_value_expr  = static_cast<ValueExpr *>(left_.get());
    ValueExpr   *right_value_expr = static_cast<ValueExpr *>(right_.get());
    const Value &left_cell        = left_value_expr->get_value();
    const Value &right_cell       = right_value_expr->get_value();

    bool value = false;
    RC   rc    = compare_value(left_cell, right_cell, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to compare tuple cells. rc=%s", strrc(rc));
    } else {
      cell.set_boolean(value);
    }
    return rc;
  }

  return RC::INVALID_ARGUMENT;
}

RC ComparisonExpr::get_value(const Tuple &tuple, Value &value)
{
  RC    rc = RC::SUCCESS;
  Value left_value;
  Value right_value;

  SubQueryExpr *left_subquery_expr  = nullptr;
  SubQueryExpr *right_subquery_expr = nullptr;

  // Lambda to check if the expression is a subquery and open it
  auto open_subquery = [&tuple](const std::unique_ptr<Expression> &expr, SubQueryExpr *&subquery_expr) -> RC {
    if (expr->type() == ExprType::SUBQUERY) {
      subquery_expr = dynamic_cast<SubQueryExpr *>(expr.get());
      RC rc         = subquery_expr->open(nullptr, tuple);  // Open the subquery expression (pass nullptr for now)
      if (OB_FAIL(rc)) {
        return rc;
      }
    }
    return RC::SUCCESS;
  };

  // Check and open the left subquery expression if it exists
  rc = open_subquery(left_, left_subquery_expr);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open left subquery expression. rc=%s", strrc(rc));
    return rc;
  }

  // Check and open the right subquery expression if it exists
  rc = open_subquery(right_, right_subquery_expr);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open right subquery expression. rc=%s", strrc(rc));
    return rc;
  }
  DEFER(if (nullptr != left_subquery_expr) left_subquery_expr->close();
        if (nullptr != right_subquery_expr) right_subquery_expr->close(););

  // Handle EXISTS/NOT EXISTS by only checking whether the right subquery returns rows
  if (comp_ == EXISTS_OP || comp_ == NOT_EXISTS_OP) {
    if (right_subquery_expr == nullptr) {
      LOG_WARN("exists operator requires subquery expression on the right side.");
      return RC::INVALID_ARGUMENT;
    }

    Value exists_value;
    rc           = right_subquery_expr->get_value(tuple, exists_value);
    bool has_row = false;
    if (rc == RC::SUCCESS) {
      has_row = true;
    } else if (rc == RC::RECORD_EOF) {
      has_row = false;
    } else {
      LOG_WARN("failed to execute exists subquery. rc=%s", strrc(rc));
      return rc;
    }

    value.set_boolean(comp_ == EXISTS_OP ? has_row : !has_row);
    return RC::SUCCESS;
  }

  // Get the value of the left expression
  rc = left_->get_value(tuple, left_value);
  if (rc != RC::SUCCESS && rc != RC::RECORD_EOF) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }

  // Check if the left subquery has more rows (error if true)
  if (left_subquery_expr && left_subquery_expr->has_more_row(tuple)) {
    return RC::SUBQUERY_RETURNED_MULTIPLE_ROWS;
  }

  // Handle IN and NOT IN operations
  if (comp_ == IN_OP || comp_ == NOT_IN_OP) {
    if (left_value.is_null()) {
      value.set_boolean(false);
      return RC::SUCCESS;
    }

    if (right_->type() == ExprType::EXPRLIST) {
      static_cast<ListExpr *>(right_.get())->reset();
    }

    // 比较表达式的结果，如果进入 while 循环且没有提前退出，那么结果即为该值
    bool res = comp_ == NOT_IN_OP;

    rc = right_->get_value(tuple, right_value);
    if (rc == RC::RECORD_EOF) {
      // 子查询结果为空，返回 null 值
    } else if (OB_FAIL(rc)) {
      // 其他错误
      return rc;
    } else if (left_value.compare(right_value) == 0) {
      // 不为空才能比较，null 是不可比较的
      res = comp_ == IN_OP;
    } else {
      while (RC::SUCCESS == (rc = right_->get_value(tuple, right_value))) {
        if (right_value.is_null()) {
          // 对于 not in，一边有 null 就为假
          if (comp_ == NOT_IN_OP) {
            res = false;
            break;
          }
        } else if (left_value.compare(right_value) == 0) {
          res = comp_ == IN_OP;
          break;
        }
      }
    }
    value.set_boolean(res);
    return rc == RC::RECORD_EOF ? RC::SUCCESS : rc;
  }

  // Get the value of the right expression
  rc = right_->get_value(tuple, right_value);
  if (rc != RC::SUCCESS && rc != RC::RECORD_EOF) {
    LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
    return rc;
  }

  // Check if the right subquery has more rows (error if true)
  if (right_subquery_expr && right_subquery_expr->has_more_row(tuple)) {
    return RC::SUBQUERY_RETURNED_MULTIPLE_ROWS;
  }

  // Compare the left and right values
  bool bool_value = false;
  rc              = compare_value(left_value, right_value, bool_value);
  if (rc == RC::SUCCESS) {
    value.set_boolean(bool_value);
  }
  return rc;
}

RC ComparisonExpr::eval(Chunk &chunk, std::vector<uint8_t> &select)
{
  RC     rc = RC::SUCCESS;
  Column left_column;
  Column right_column;

  rc = left_->get_column(chunk, left_column);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }
  rc = right_->get_column(chunk, right_column);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
    return rc;
  }
  if (left_column.attr_type() != right_column.attr_type()) {
    LOG_WARN("cannot compare columns with different types");
    return RC::INTERNAL;
  }
  if (left_column.attr_type() == AttrType::INTS) {
    rc = compare_column<int>(left_column, right_column, select);
  } else if (left_column.attr_type() == AttrType::FLOATS) {
    rc = compare_column<float>(left_column, right_column, select);
  } else {
    // TODO: support string compare
    LOG_WARN("unsupported data type %d", left_column.attr_type());
    return RC::INTERNAL;
  }
  return rc;
}

template <typename T>
RC ComparisonExpr::compare_column(const Column &left, const Column &right, std::vector<uint8_t> &result) const
{
  RC rc = RC::SUCCESS;

  bool left_const  = left.column_type() == Column::Type::CONSTANT_COLUMN;
  bool right_const = right.column_type() == Column::Type::CONSTANT_COLUMN;
  if (left_const && right_const) {
    compare_result<T, true, true>((T *)left.data(), (T *)right.data(), left.count(), result, comp_);
  } else if (left_const && !right_const) {
    compare_result<T, true, false>((T *)left.data(), (T *)right.data(), right.count(), result, comp_);
  } else if (!left_const && right_const) {
    compare_result<T, false, true>((T *)left.data(), (T *)right.data(), left.count(), result, comp_);
  } else {
    compare_result<T, false, false>((T *)left.data(), (T *)right.data(), left.count(), result, comp_);
  }
  return rc;
}

////////////////////////////////////////////////////////////////////////////////
ConjunctionExpr::ConjunctionExpr(Type type, vector<unique_ptr<Expression>> &children)
    : conjunction_type_(type), children_(std::move(children))
{}

ConjunctionExpr::ConjunctionExpr(Type type, Expression *left, Expression *right) : conjunction_type_(type)
{
  children_.emplace_back(left);
  children_.emplace_back(right);
}

ConjunctionExpr::ConjunctionExpr(Type type, std::unique_ptr<Expression> children) : conjunction_type_(type)
{
  children_.push_back(std::move(children));
}

RC ConjunctionExpr::get_value(const Tuple &tuple, Value &value)
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    value.set_boolean(true);
    return rc;
  }

  Value tmp_value;
  for (const unique_ptr<Expression> &expr : children_) {
    rc = expr->get_value(tuple, tmp_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value by child expression. rc=%s", strrc(rc));
      return rc;
    }
    bool bool_value = tmp_value.get_boolean();
    if ((conjunction_type_ == Type::AND && !bool_value) || (conjunction_type_ == Type::OR && bool_value)) {
      value.set_boolean(bool_value);
      return rc;
    }
  }

  bool default_value = (conjunction_type_ == Type::AND);
  value.set_boolean(default_value);
  return rc;
}

////////////////////////////////////////////////////////////////////////////////

ArithmeticExpr::ArithmeticExpr(ArithmeticExpr::Type type, Expression *left, Expression *right)
    : arithmetic_type_(type), left_(left), right_(right)
{}
ArithmeticExpr::ArithmeticExpr(ArithmeticExpr::Type type, unique_ptr<Expression> left, unique_ptr<Expression> right)
    : arithmetic_type_(type), left_(std::move(left)), right_(std::move(right))
{}

bool ArithmeticExpr::equal(const Expression &other) const
{
  if (this == &other) {
    return true;
  }
  if (type() != other.type()) {
    return false;
  }
  auto &other_arith_expr = static_cast<const ArithmeticExpr &>(other);
  return arithmetic_type_ == other_arith_expr.arithmetic_type() && left_->equal(*other_arith_expr.left_) &&
         right_->equal(*other_arith_expr.right_);
}
AttrType ArithmeticExpr::value_type() const
{
  if (!right_) {
    return left_->value_type();
  }

  if (left_->value_type() == AttrType::INTS && right_->value_type() == AttrType::INTS &&
      arithmetic_type_ != Type::DIV) {
    return AttrType::INTS;
  }

  if (left_->value_type() == AttrType::VECTORS && right_->value_type() == AttrType::VECTORS) {
    return AttrType::VECTORS;
  }

  return AttrType::FLOATS;
}

RC ArithmeticExpr::calc_value(const Value &left_value, const Value &right_value, Value &value) const
{
  RC rc = RC::SUCCESS;
  if (left_value.is_null() || right_value.is_null()) {
    value.set_null(true);
    return RC::SUCCESS;
  }

  const AttrType target_type = value_type();
  value.set_type(target_type);

  switch (arithmetic_type_) {
    case Type::ADD: {
      return Value::add(left_value, right_value, value);
    } break;

    case Type::SUB: {
      return Value::subtract(left_value, right_value, value);
    } break;

    case Type::MUL: {
      return Value::multiply(left_value, right_value, value);
    } break;

    case Type::DIV: {
      return Value::divide(left_value, right_value, value);
    } break;

    case Type::NEGATIVE: {
      return Value::negative(left_value, value);
    } break;

    default: {
      return RC::INTERNAL;
    } break;
  }
  return rc;
}

template <bool LEFT_CONSTANT, bool RIGHT_CONSTANT>
RC ArithmeticExpr::execute_calc(
    const Column &left, const Column &right, Column &result, Type type, AttrType attr_type) const
{
  RC rc = RC::SUCCESS;
  switch (type) {
    case Type::ADD: {
      if (attr_type == AttrType::INTS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, int, AddOperator>(
            (int *)left.data(), (int *)right.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, float, AddOperator>(
            (float *)left.data(), (float *)right.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
    } break;
    case Type::SUB:
      if (attr_type == AttrType::INTS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, int, SubtractOperator>(
            (int *)left.data(), (int *)right.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, float, SubtractOperator>(
            (float *)left.data(), (float *)right.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
      break;
    case Type::MUL:
      if (attr_type == AttrType::INTS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, int, MultiplyOperator>(
            (int *)left.data(), (int *)right.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, float, MultiplyOperator>(
            (float *)left.data(), (float *)right.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
      break;
    case Type::DIV:
      if (attr_type == AttrType::INTS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, int, DivideOperator>(
            (int *)left.data(), (int *)right.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, float, DivideOperator>(
            (float *)left.data(), (float *)right.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
      break;
    case Type::NEGATIVE:
      if (attr_type == AttrType::INTS) {
        unary_operator<LEFT_CONSTANT, int, NegateOperator>((int *)left.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        unary_operator<LEFT_CONSTANT, float, NegateOperator>(
            (float *)left.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
      break;
    default: rc = RC::UNIMPLEMENTED; break;
  }
  if (rc == RC::SUCCESS) {
    result.set_count(result.capacity());
  }
  return rc;
}

RC ArithmeticExpr::get_value(const Tuple &tuple, Value &value)
{
  RC rc = RC::SUCCESS;

  Value left_value;
  Value right_value;

  if (left_) {
    rc = left_->get_value(tuple, left_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
      return rc;
    }
  }
  if (right_) {
    rc = right_->get_value(tuple, right_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
      return rc;
    }
  }
  return calc_value(left_value, right_value, value);
}

RC ArithmeticExpr::get_column(Chunk &chunk, Column &column)
{
  RC rc = RC::SUCCESS;
  if (pos_ != -1) {
    column.reference(chunk.column(pos_));
    return rc;
  }
  Column left_column;
  Column right_column;

  rc = left_->get_column(chunk, left_column);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get column of left expression. rc=%s", strrc(rc));
    return rc;
  }
  rc = right_->get_column(chunk, right_column);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get column of right expression. rc=%s", strrc(rc));
    return rc;
  }
  return calc_column(left_column, right_column, column);
}

RC ArithmeticExpr::calc_column(const Column &left_column, const Column &right_column, Column &column) const
{
  RC rc = RC::SUCCESS;

  const AttrType target_type = value_type();
  column.init(target_type, left_column.attr_len(), std::max(left_column.count(), right_column.count()));
  bool left_const  = left_column.column_type() == Column::Type::CONSTANT_COLUMN;
  bool right_const = right_column.column_type() == Column::Type::CONSTANT_COLUMN;
  if (left_const && right_const) {
    column.set_column_type(Column::Type::CONSTANT_COLUMN);
    rc = execute_calc<true, true>(left_column, right_column, column, arithmetic_type_, target_type);
  } else if (left_const && !right_const) {
    column.set_column_type(Column::Type::NORMAL_COLUMN);
    rc = execute_calc<true, false>(left_column, right_column, column, arithmetic_type_, target_type);
  } else if (!left_const && right_const) {
    column.set_column_type(Column::Type::NORMAL_COLUMN);
    rc = execute_calc<false, true>(left_column, right_column, column, arithmetic_type_, target_type);
  } else {
    column.set_column_type(Column::Type::NORMAL_COLUMN);
    rc = execute_calc<false, false>(left_column, right_column, column, arithmetic_type_, target_type);
  }
  return rc;
}

RC ArithmeticExpr::try_get_value(Value &value) const
{
  RC rc = RC::SUCCESS;

  Value left_value;
  Value right_value;

  rc = left_->try_get_value(left_value);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }

  if (right_) {
    rc = right_->try_get_value(right_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
      return rc;
    }
  }

  return calc_value(left_value, right_value, value);
}

////////////////////////////////////////////////////////////////////////////////

UnboundFunctionExpr::UnboundFunctionExpr(const char *aggregate_name, std::vector<std::unique_ptr<Expression>> child)
    : function_name_(aggregate_name), args_(std::move(child))
{}

////////////////////////////////////////////////////////////////////////////////
AggregateFunctionExpr::AggregateFunctionExpr(AggregateFunctionType type, Expression *child)
    : aggregate_type_(type), child_(child)
{}

AggregateFunctionExpr::AggregateFunctionExpr(AggregateFunctionType type, unique_ptr<Expression> child)
    : aggregate_type_(type), child_(std::move(child))
{}

RC AggregateFunctionExpr::get_column(Chunk &chunk, Column &column)
{
  RC rc = RC::SUCCESS;
  if (pos_ != -1) {
    column.reference(chunk.column(pos_));
  } else {
    rc = RC::INTERNAL;
  }
  return rc;
}

bool AggregateFunctionExpr::equal(const Expression &other) const
{
  if (this == &other) {
    return true;
  }
  if (other.type() != type()) {
    return false;
  }
  const AggregateFunctionExpr &other_aggr_expr = static_cast<const AggregateFunctionExpr &>(other);
  return aggregate_type_ == other_aggr_expr.aggregate_type() && child_->equal(*other_aggr_expr.child());
}

unique_ptr<Aggregator> AggregateFunctionExpr::create_aggregator() const
{
  unique_ptr<Aggregator> aggregator;
  switch (aggregate_type_) {
    case AggregateFunctionType::SUM: {
      aggregator = make_unique<SumAggregator>();
      break;
    }
    case AggregateFunctionType::COUNT: {
      aggregator = make_unique<CountAggregator>();
      break;
    }
    case AggregateFunctionType::AVG: {
      aggregator = make_unique<AvgAggregator>();
      break;
    }
    case AggregateFunctionType::MAX: {
      aggregator = make_unique<MaxAggregator>();
      break;
    }
    case AggregateFunctionType::MIN: {
      aggregator = make_unique<MinAggregator>();
      break;
    }
    default: {
      ASSERT(false, "unsupported aggregate type");
      break;
    }
  }
  return aggregator;
}

RC AggregateFunctionExpr::get_value(const Tuple &tuple, Value &value)
{
  RC rc = tuple.find_cell(TupleCellSpec(name()), value);
  if (rc == RC::NOTFOUND) {
    value = Value(NullValue());
    return RC::SUCCESS;
  }
  return rc;
}

RC AggregateFunctionExpr::type_from_string(const char *type_str, AggregateFunctionType &type)
{
  check_type("sum", AggregateFunctionType::SUM);
  check_type("avg", AggregateFunctionType::AVG);
  check_type("max", AggregateFunctionType::MAX);
  check_type("min", AggregateFunctionType::MIN);
  check_type("count", AggregateFunctionType::COUNT);
  return RC::INVALID_ARGUMENT;
}

SubQueryExpr::SubQueryExpr(SelectSqlNode &select_node) : sql_node_(select_node) {}

SubQueryExpr::~SubQueryExpr() = default;

RC SubQueryExpr::generate_select_stmt(Db *db, const std::unordered_map<std::string, BaseTable *> &tables)
{
  // 仿照普通 select 的执行流程，tables 用来传递别名
  Stmt *stmt = nullptr;
  RC    rc   = SelectStmt::create(db, sql_node_, stmt, tables);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to create subquery select statement. return %s", strrc(rc));
    return rc;
  }

  // 确保生成的 stmt 类型为 SELECT 类型
  if (stmt->type() != StmtType::SELECT) {
    LOG_WARN("subquery stmt type is not SELECT.");
    return RC::INVALID_ARGUMENT;
  }

  // 动态转换为 SelectStmt 类型，并进行子查询列数校验
  auto *select_stmt = dynamic_cast<SelectStmt *>(stmt);
  if (select_stmt == nullptr) {
    LOG_WARN("failed to cast subquery stmt to SelectStmt. ");
    return RC::INVALID_ARGUMENT;
  }

  // 子查询不能有超过一个列
  if (!allow_multi_column_ && select_stmt->query_expressions_size() > 1) {
    LOG_WARN("too many columns in subquery expression.");
    return RC::TO_LONG_SUBQUERY_EXPR;
  }

  // 将 select_stmt_ 指针设置为 select_stmt，使用 std::unique_ptr 来管理
  select_stmt_ = std::unique_ptr<SelectStmt>(select_stmt);
  return RC::SUCCESS;
}

RC SubQueryExpr::generate_logical_oper()
{
  RC rc = LogicalPlanGenerator::create(select_stmt_.get(), logical_oper_);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to generate logical operator for subquery. return %s", strrc(rc));
    return rc;
  }
  return RC::SUCCESS;
}

RC SubQueryExpr::generate_physical_oper()
{
  RC rc = PhysicalPlanGenerator::create(*logical_oper_, physical_oper_);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to generate physical operator for subquery. return %s", strrc(rc));
    return rc;
  }
  return rc;
}

bool SubQueryExpr::one_row_ret() const { return res_query.size() <= 1; }

// 子算子树的 open 和 close 逻辑由外部控制
RC SubQueryExpr::open(Trx *trx, const Tuple &tuple)
{
  RC rc = RC::SUCCESS;
  physical_oper_->set_parent_tuple(&tuple);
  rc = physical_oper_->open(trx);

  return rc;
}

RC SubQueryExpr::reset()
{
  visited_index = 0;
  return RC::SUCCESS;
}

bool SubQueryExpr::has_more_row(const Tuple &tuple) const
{
  physical_oper_->set_parent_tuple(&tuple);
  return physical_oper_->next() != RC::RECORD_EOF;
}

RC SubQueryExpr::get_value(const Tuple &tuple, Value &value)
{
  physical_oper_->set_parent_tuple(&tuple);
  RC rc = physical_oper_->next();
  if (rc == RC::RECORD_EOF) {
    // 先返回 null 类型的值，之后再完善具体选择的列类型
    // 如果已经成功执行过一次，结果不为空，那么这里的 value 不会被用到
    value.set_type(AttrType::NULLS);
    value.set_null(true);
    // 给调用者判断结果是否为空，而不是直接返回 RC::SUCCESS
    return rc;
  } else if (OB_FAIL(rc)) {
    // 其他错误
    return rc;
  }

  // 到这里确保有一条记录
  rc = physical_oper_->current_tuple()->cell_at(0, value);
  if (OB_FAIL(rc)) {
    return rc;
  }
  return RC::SUCCESS;
}

RC SubQueryExpr::close() { return physical_oper_->close(); }

RC SubQueryExpr::try_get_value(Value &value) const { return RC::UNIMPLEMENTED; }

ExprType SubQueryExpr::type() const { return ExprType::SUBQUERY; }

AttrType SubQueryExpr::value_type() const { return AttrType::UNDEFINED; }

//new exist class complete
ExistsExpr::ExistsExpr(bool not_exists, SelectSqlNode &select_node)
    : not_exists_(not_exists), subquery_expr_(std::make_unique<SubQueryExpr>(select_node))
{
  subquery_expr_->set_allow_multi_column(true);
}

ExistsExpr::ExistsExpr(bool not_exists, std::unique_ptr<SubQueryExpr> subquery_expr)
    : not_exists_(not_exists), subquery_expr_(std::move(subquery_expr))
{
  if (subquery_expr_) {
    subquery_expr_->set_allow_multi_column(true);
  }
}

RC ExistsExpr::generate_select_stmt(Db *db, const std::unordered_map<std::string, BaseTable *> &tables)
{
  if (subquery_expr_ == nullptr) {
    return RC::INVALID_ARGUMENT;
  }
  subquery_expr_->set_allow_multi_column(true);
  return subquery_expr_->generate_select_stmt(db, tables);
}

RC ExistsExpr::generate_logical_oper()
{
  if (subquery_expr_ == nullptr) {
    return RC::INVALID_ARGUMENT;
  }
  return subquery_expr_->generate_logical_oper();
}

RC ExistsExpr::generate_physical_oper()
{
  if (subquery_expr_ == nullptr) {
    return RC::INVALID_ARGUMENT;
  }
  return subquery_expr_->generate_physical_oper();
}

RC ExistsExpr::reset()
{
  if (subquery_expr_ == nullptr) {
    return RC::SUCCESS;
  }
  return subquery_expr_->reset();
}

RC ExistsExpr::get_value(const Tuple &tuple, Value &value)
{
  if (subquery_expr_ == nullptr) {
    return RC::INVALID_ARGUMENT;
  }

  RC rc = subquery_expr_->open(nullptr, tuple);
  if (OB_FAIL(rc)) {
    return rc;
  }
  DEFER(subquery_expr_->close());

  Value dummy;
  rc           = subquery_expr_->get_value(tuple, dummy);
  bool has_row = false;
  if (rc == RC::SUCCESS) {
    has_row = true;
  } else if (rc == RC::RECORD_EOF) {
    has_row = false;
    rc      = RC::SUCCESS;
  } else {
    return rc;
  }

  value.set_type(AttrType::BOOLEANS);
  value.set_boolean(not_exists_ ? !has_row : has_row);
  return rc;
}

////////////////////////////////////////////////////////////////////////////////
// MatchAgainstExpr

MatchAgainstExpr::MatchAgainstExpr(std::unique_ptr<Expression> field_expr, std::unique_ptr<Expression> search_expr)
    : field_expr_(std::move(field_expr)), search_expr_(std::move(search_expr))
{}

RC MatchAgainstExpr::get_value(const Tuple &tuple, Value &value)
{
  // 获取搜索字符串
  Value search_value;
  RC rc = search_expr_->get_value(tuple, search_value);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get search value in MATCH...AGAINST expression");
    return rc;
  }

  // 检查搜索字符串类型
  if (!search_value.is_str()) {
    LOG_WARN("MATCH...AGAINST search text must be string type");
    value = Value(0.0f);
    return RC::INVALID_ARGUMENT;
  }

  std::string search_text = search_value.to_string();

  // 对搜索文本进行分词
  std::vector<std::string> query_tokens;
  rc = JiebaUtil::instance().tokenize(search_text, query_tokens);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to tokenize search text");
    value = Value(0.0f);
    return rc;
  }

  // 尝试获取表和RID，使用全文索引计算BM25
  // 首先检查 field_expr_ 是否是 FieldExpr 类型
  if (field_expr_->type() == ExprType::FIELD) {
    FieldExpr *field_expr = static_cast<FieldExpr *>(field_expr_.get());
    const Field &field = field_expr->field();
    const char *field_name = field.field_name();
    BaseTable *base_table = const_cast<BaseTable *>(field.table());
    
    // 尝试将 BaseTable 转换为 Table
    Table *table = dynamic_cast<Table *>(base_table);
    if (table != nullptr) {
      // 尝试获取 RID，检查 tuple 是否是 RowTuple
      const RowTuple *row_tuple = dynamic_cast<const RowTuple *>(&tuple);
      if (row_tuple != nullptr) {
        const Record &record = row_tuple->record();
        RID rid = record.rid();
        
        // 获取全文索引
        FullTextIndex *ft_idx = table->get_fulltext_index(field_name);
        if (ft_idx != nullptr) {
          // 使用全文索引计算BM25分数
          double score = ft_idx->calculate_bm25(query_tokens, rid);
          value = Value(static_cast<float>(score));
          return RC::SUCCESS;
        }
      }
    }
  }
  
  // 如果无法使用全文索引（例如 tuple 不是 RowTuple，或者没有全文索引）
  // 则回退到简化的BM25计算（这是为了兼容性，但不应该发生在有全文索引的情况下）
  
  // 获取字段值
  Value field_value;
  rc = field_expr_->get_value(tuple, field_value);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get field value in MATCH...AGAINST expression");
    return rc;
  }

  // 检查类型
  if (!field_value.is_str()) {
    LOG_WARN("MATCH...AGAINST only supports string types");
    value = Value(0.0f);
    return RC::INVALID_ARGUMENT;
  }

  std::string field_text = field_value.to_string();

  // 对字段文本进行分词
  std::vector<std::string> field_tokens;
  rc = JiebaUtil::instance().tokenize(field_text, field_tokens);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to tokenize field text");
    value = Value(0.0f);
    return rc;
  }

  // 计算简化的BM25分数（回退方案，不应该在生产环境使用）
  LOG_WARN("Using fallback BM25 calculation without fulltext index");
  const double k1 = 1.5;
  const double b = 0.75;
  
  double score = 0.0;
  size_t doc_length = field_tokens.size();
  const double avg_doc_length = 20.0;  // 估计值
  
  for (const std::string &query_term : query_tokens) {
    if (query_term.empty()) {
      continue;
    }
    
    int term_freq = 0;
    for (const std::string &field_term : field_tokens) {
      if (field_term == query_term) {
        term_freq++;
      }
    }
    
    if (term_freq > 0) {
      double idf = std::log(2.0);  // 固定IDF值
      double numerator = term_freq * (k1 + 1.0);
      double denominator = term_freq + k1 * (1.0 - b + b * (static_cast<double>(doc_length) / avg_doc_length));
      double term_score = numerator / denominator;
      score += idf * term_score;
    }
  }
  
  value = Value(static_cast<float>(score));
  return RC::SUCCESS;
}

RC MatchAgainstExpr::try_get_value(Value &value) const
{
  // MATCH...AGAINST无法在编译时计算，因为需要实际的文档内容
  return RC::UNIMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// ListExpr

ListExpr::ListExpr(std::vector<Expression *> &&exprs)
{
  for (auto expr : exprs) {
    exprs_.emplace_back(std::unique_ptr<Expression>(expr));
  }
  exprs.clear();
}

RC NormalFunctionExpr::type_from_string(const char *type_str, NormalFunctionType &type)
{
  check_type("typeof", NormalFunctionType::TYPEOF);
  check_type("month", NormalFunctionType::MONTH);
  check_type("year", NormalFunctionType::YEAR);
  check_type("date_format", NormalFunctionType::DATE_FORMAT);
  check_type("length", NormalFunctionType::LENGTH);
  check_type("round", NormalFunctionType::ROUND);
  check_type("l2_distance", NormalFunctionType::L2_DISTANCE);
  check_type("cosine_distance", NormalFunctionType::COSINE_DISTANCE);
  check_type("inner_product", NormalFunctionType::INNER_PRODUCT);
  check_type("string_to_vector", NormalFunctionType::STRING_TO_VECTOR);
  check_type("vector_to_string", NormalFunctionType::VECTOR_TO_STRING);
  check_type("vector_dim", NormalFunctionType::VECTOR_DIM);
  check_type("tokenize", NormalFunctionType::TOKENIZE);
  return RC::INVALID_ARGUMENT;
}

RC NormalFunctionExpr::get_value(const Tuple &tuple, Value &result)
{
  vector<Value> args_values_;
  for (auto &expr : args()) {
    Value value;
    RC    rc = expr->get_value(tuple, value);
    if (OB_FAIL(rc)) {
      return rc;
    }
    args_values_.push_back(value);
  }
  switch (type_) {
    case NormalFunctionType::LENGTH: return builtin::length(args_values_, result);
    case NormalFunctionType::ROUND: return builtin::round(args_values_, result);
    case NormalFunctionType::DATE_FORMAT: return builtin::date_format(args_values_, result);
    case NormalFunctionType::STRING_TO_VECTOR: return builtin::string_to_vector(args_values_, result);
    case NormalFunctionType::VECTOR_TO_STRING: return builtin::vector_to_string(args_values_, result);
    case NormalFunctionType::VECTOR_DIM: return builtin::vector_dim(args_values_, result);
    case NormalFunctionType::YEAR: return builtin::year(args_values_, result);
    case NormalFunctionType::MONTH: return builtin::month(args_values_, result);
    case NormalFunctionType::DAY: return builtin::day(args_values_, result);
    case NormalFunctionType::L2_DISTANCE: return builtin::l2_distance(args_values_, result);
    case NormalFunctionType::COSINE_DISTANCE: return builtin::cosine_distance(args_values_, result);
    case NormalFunctionType::INNER_PRODUCT: return builtin::inner_product(args_values_, result);
    case NormalFunctionType::TYPEOF: return builtin::_typeof(args_values_, result);
    case NormalFunctionType::TOKENIZE: return builtin::tokenize(args_values_, result);
  }
  return RC::INTERNAL;
}

RC NormalFunctionExpr::try_get_value(Value &result) const
{
  vector<Value> args_values_;
  for (auto &expr : args()) {
    Value value;
    RC    rc = expr->try_get_value(value);
    if (OB_FAIL(rc)) {
      return rc;
    }
    args_values_.push_back(value);
  }
  switch (type_) {
    case NormalFunctionType::LENGTH: return builtin::length(args_values_, result);
    case NormalFunctionType::ROUND: return builtin::round(args_values_, result);
    case NormalFunctionType::DATE_FORMAT: return builtin::date_format(args_values_, result);
    case NormalFunctionType::STRING_TO_VECTOR: return builtin::string_to_vector(args_values_, result);
    case NormalFunctionType::VECTOR_TO_STRING: return builtin::vector_to_string(args_values_, result);
    case NormalFunctionType::VECTOR_DIM: return builtin::vector_dim(args_values_, result);
    case NormalFunctionType::YEAR: return builtin::year(args_values_, result);
    case NormalFunctionType::MONTH: return builtin::month(args_values_, result);
    case NormalFunctionType::DAY: return builtin::day(args_values_, result);
    case NormalFunctionType::L2_DISTANCE: return builtin::l2_distance(args_values_, result);
    case NormalFunctionType::COSINE_DISTANCE: return builtin::cosine_distance(args_values_, result);
    case NormalFunctionType::INNER_PRODUCT: return builtin::inner_product(args_values_, result);
    case NormalFunctionType::TYPEOF: return builtin::_typeof(args_values_, result);
    case NormalFunctionType::TOKENIZE: return builtin::tokenize(args_values_, result);
  }
  return RC::INTERNAL;
}

AttrType NormalFunctionExpr::value_type() const
{
  switch (type_) {
    case NormalFunctionType::LENGTH: return AttrType::INTS;
    case NormalFunctionType::ROUND: return AttrType::FLOATS;
    case NormalFunctionType::DATE_FORMAT: return AttrType::CHARS;
    case NormalFunctionType::STRING_TO_VECTOR: return AttrType::VECTORS;
    case NormalFunctionType::VECTOR_TO_STRING: return AttrType::CHARS;
    case NormalFunctionType::VECTOR_DIM: return AttrType::INTS;
    case NormalFunctionType::YEAR: return AttrType::INTS;
    case NormalFunctionType::MONTH: return AttrType::INTS;
    case NormalFunctionType::DAY: return AttrType::INTS;
    case NormalFunctionType::L2_DISTANCE: return AttrType::FLOATS;
    case NormalFunctionType::COSINE_DISTANCE: return AttrType::FLOATS;
    case NormalFunctionType::INNER_PRODUCT: return AttrType::FLOATS;
    case NormalFunctionType::TYPEOF: return AttrType::CHARS;
    case NormalFunctionType::TOKENIZE: return AttrType::CHARS;
  }
  return AttrType::UNDEFINED;
}
