/***************************************************************
 *                                                             *
 * @Author      : Koschei                                      *
 * @Email       : nitianzero@gmail.com                         *
 * @Date        : 2024/10/13                                     *
 * @Description : Brief description of the file's purpose      *
 *                                                             *
 * Copyright (c) 2024 Koschei                                  *
 * All rights reserved.                                        *
 *                                                             *
 ***************************************************************/

#include "sql/operator/view_scan_physical_operator.h"
#include "event/sql_debug.h"
#include "storage/table/table.h"
#include "sql/expr/expression_tuple.h"
#include "storage/table/view.h"
#include "sql/stmt/select_stmt.h"
#include "sql/optimizer/logical_plan_generator.h"
#include "sql/optimizer/physical_plan_generator.h"

RC ViewScanPhysicalOperator::init()
{
  // 【赛题 22 create-view】View 不拥有独立数据文件；扫描视图时重新解析/绑定
  // 保存的 SELECT，并生成一棵子查询物理计划。视图本质是命名查询。
  RC rc = RC::SUCCESS;

  auto select_sql = view_->select_sql();

  ParsedSqlResult parsed_sql_result;
  parse(select_sql.c_str(), &parsed_sql_result);

  if (parsed_sql_result.sql_nodes().empty()) {
    LOG_ERROR("Parsing failed: No SQL nodes found");
    return RC::INTERNAL;
  }

  if (parsed_sql_result.sql_nodes().size() > 1) {
    LOG_WARN("got multi sql commands but only 1 will be handled");
  }

  std::unique_ptr<ParsedSqlNode> sql_node = std::move(parsed_sql_result.sql_nodes().front());
  if (sql_node->flag == SCF_ERROR) {
    LOG_ERROR("Syntax error in SQL");
    return RC::SQL_SYNTAX;
  }
  if (sql_node->flag != SCF_SELECT) {
    LOG_ERROR("Unexpected SQL command type. Expected SELECT, got flag: %d", sql_node->flag);
    return RC::INTERNAL;
  }

  Stmt *stmt = nullptr;
  rc         = SelectStmt::create(view_->db(), sql_node->selection, stmt);
  if (rc != RC::SUCCESS && rc != RC::UNIMPLEMENTED) {
    LOG_WARN("Failed to create select statement. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  auto                             select_stmt  = dynamic_cast<SelectStmt *>(stmt);
  std::unique_ptr<LogicalOperator> logical_oper = nullptr;
  LogicalPlanGenerator::create(select_stmt, logical_oper);

  if (logical_oper == nullptr) {
    LOG_ERROR("Failed to create logical plan");
    return RC::INTERNAL;
  }

  std::unique_ptr<PhysicalOperator> physical_oper = nullptr;
  PhysicalPlanGenerator::create(*logical_oper, physical_oper);

  if (physical_oper == nullptr) {
    LOG_ERROR("Failed to create physical plan");
    return RC::INTERNAL;
  }

  select_expr_ = std::move(physical_oper);

  return RC::SUCCESS;
}

RC ViewScanPhysicalOperator::open(Trx *trx)
{
  RC rc = RC::SUCCESS;

  if (select_expr_ == nullptr) {
    rc = init();
    if (OB_FAIL(rc)) {
      return rc;
    }
  }

  rc = select_expr_->open(trx);
  if (rc == RC::SUCCESS) {
    tuple_.set_schema(view_, view_->table_meta().field_metas());
  }
  trx_ = trx;
  return rc;
}

RC ViewScanPhysicalOperator::next()
{
  // 从视图 SELECT 的结果 Tuple 中映射出视图列，再应用下推到视图扫描上的谓词。
  RC rc = RC::SUCCESS;

  bool filter_result = false;
  while (OB_SUCC(rc = next_record())) {
    LOG_TRACE("got a record. rid=%s", current_record_.rid().to_string().c_str());

    tuple_.set_record(&current_record_);
    rc = filter(tuple_, filter_result);
    if (rc != RC::SUCCESS) {
      LOG_TRACE("record filtered failed=%s", strrc(rc));
      return rc;
    }

    if (filter_result) {
      sql_debug("get a tuple: %s", tuple_.to_string().c_str());
      break;
    } else {
      sql_debug("a tuple is filtered: %s", tuple_.to_string().c_str());
    }
  }
  return rc;
}

RC ViewScanPhysicalOperator::close() { return select_expr_->close(); }

Tuple *ViewScanPhysicalOperator::current_tuple() { return &tuple_; }

RC ViewScanPhysicalOperator::tuple_schema(TupleSchema &schema) const
{
  schema = TupleSchema();
  if (view_ == nullptr) {
    return RC::INTERNAL;
  }

  const TableMeta &table_meta = view_->table_meta();
  const int        sys_fields = table_meta.sys_field_num();
  const int        field_num  = table_meta.field_num();

  for (int i = sys_fields; i < field_num; i++) {
    const FieldMeta *field_meta = table_meta.field(i);
    if (field_meta == nullptr) {
      continue;
    }
    if (!field_meta->visible()) {
      continue;
    }
    const char *alias = table_alias_.empty() ? nullptr : table_alias_.c_str();
    schema.append_cell(TupleCellSpec(view_->name(), field_meta->name(), alias));
  }

  if (schema.cell_num() == 0) {
    return RC::UNIMPLEMENTED;
  }
  return RC::SUCCESS;
}

string ViewScanPhysicalOperator::param() const { return view_->name(); }

void ViewScanPhysicalOperator::set_predicates(vector<unique_ptr<Expression>> &&exprs)
{
  predicates_ = std::move(exprs);
}

RC ViewScanPhysicalOperator::filter(RowTuple &tuple, bool &result)
{
  RC          rc = RC::SUCCESS;
  Value       value;
  Tuple      *tp = &tuple;
  JoinedTuple jt;
  jt.set_left(&tuple);
  jt.set_right(const_cast<Tuple *>(parent_tuple_));
  if (parent_tuple_) {
    tp = &jt;
  }
  for (unique_ptr<Expression> &expr : predicates_) {
    rc = expr->get_value(*tp, value);
    if (rc != RC::SUCCESS) {
      close();
      return rc;
    }

    bool tmp_result = value.get_boolean();
    if (!tmp_result) {
      result = false;
      return rc;
    }
  }

  result = true;
  return rc;
}

RC ViewScanPhysicalOperator::next_record()
{
  RC rc = select_expr_->next();
  if (OB_FAIL(rc)) {
    return rc;
  }

  auto tuple = select_expr_->current_tuple();
  tuple_.set_base_rids(tuple->base_rids());
  // 构造一个 view 的 record
  int                cell_num = tuple->cell_num();
  std::vector<Value> values(cell_num);
  for (int i = 0; i < cell_num; i++) {
    Value cell;
    rc = tuple->cell_at(i, cell);
    if (OB_FAIL(rc)) {
      return rc;
    }
    values[i] = std::move(cell);
  }

  rc = view_->make_record(static_cast<int>(values.size()), values.data(), current_record_);
  return rc;
}
