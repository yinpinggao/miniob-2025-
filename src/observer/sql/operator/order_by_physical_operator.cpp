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
// Created by HuXin on 24-10-9.
//

#include "order_by_physical_operator.h"
#include "sql/operator/external_sort/external_sorter.h"
#include "common/log/log.h"

#include <utility>
#include <functional>
#include <algorithm>

OrderByPhysicalOperator::OrderByPhysicalOperator(vector<OrderBySqlNode> order_by) : order_by_(std::move(order_by))
{
  order_and_field_line = order_list([this](const order_line &cells_a, const order_line &cells_b) -> bool {
    auto  order_size   = order_by_.size();
    auto &order_line_a = cells_a.first;
    auto &order_line_b = cells_b.first;
    assert(order_line_a.size() == order_size);
    assert(order_line_b.size() == order_size);
    assert(order_by_.size() == order_size);

    for (size_t i = 0; i < order_size; i++) {
      auto &a      = order_line_a[i];
      auto &b      = order_line_b[i];
      auto  result = a.compare(b);
      auto  is_asc = order_by_[i].is_asc;
      if (result < 0) {
        // a < b
        return !is_asc;
      } else if (result > 0) {
        // a > 0
        return is_asc;
      }
    }

    // order_line_a == order_line_b
    return true;
  });
}

OrderByPhysicalOperator::~OrderByPhysicalOperator()
{
  // 清理内存排序的数据
  while (!order_and_field_line.empty()) {
    delete order_and_field_line.top().second;
    order_and_field_line.pop();
  }
  tuple_holder_.reset();
  tuple_ = nullptr;
}

RC OrderByPhysicalOperator::fetch_and_sort_tables()
{
  RC rc = RC::SUCCESS;

  while (RC::SUCCESS == (rc = children_[0]->next())) {
    // 获取 order by 字段的 values
    vector<Value> order_by_line;
    for (auto &[expr, asc] : order_by_) {
      Value cell;
      rc = expr->get_value(*children_[0]->current_tuple(), cell);
      if (OB_FAIL(rc)) {
        return rc;
      }
      order_by_line.emplace_back(cell);
    }

    order_and_field_line.emplace(order_by_line, children_[0]->current_tuple()->copy());
  }

  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::open(Trx *trx)
{
  RC rc = RC::SUCCESS;
  if (children_.size() != 1) {
    return RC::INTERNAL;
  }

  tuple_holder_.reset();
  tuple_ = nullptr;
  
  rc = children_[0]->open(trx);
  if (OB_FAIL(rc)) {
    return rc;
  }

  // ===== 科学的外部排序启动决策 =====
  
  // 1. 估算输入数据量
  size_t estimated_rows = estimate_input_rows();
  size_t estimated_tuple_size = estimate_tuple_size();
  size_t estimated_memory = estimated_rows * estimated_tuple_size;
  
  // 2. 获取内存阈值
  size_t memory_threshold = get_sort_memory_threshold();
  
  // 3. 决策：根据估算的内存需求选择排序算法
  if (estimated_memory > memory_threshold) {
    // 预计内存需求超过阈值，使用外部排序
    LOG_INFO("using external sort: estimated_rows=%lu, tuple_size=%lu, estimated_memory=%lu bytes, threshold=%lu bytes",
             estimated_rows, estimated_tuple_size, estimated_memory, memory_threshold);
    use_external_sort_ = true;
    rc = external_sort_open(trx);
  } else {
    // 预计内存需求在阈值内，使用内存排序（更快）
    LOG_INFO("using in-memory sort: estimated_rows=%lu, tuple_size=%lu, estimated_memory=%lu bytes",
             estimated_rows, estimated_tuple_size, estimated_memory);
    use_external_sort_ = false;
    rc = memory_sort_open(trx);
  }

  return rc;
}

RC OrderByPhysicalOperator::next()
{
  if (use_external_sort_) {
    // 使用外部排序
    Tuple *next_tuple = nullptr;
    RC     rc         = external_sorter_->next(next_tuple);
    if (rc == RC::SUCCESS) {
      tuple_holder_.reset(next_tuple);
      tuple_ = tuple_holder_.get();
    } else {
      tuple_holder_.reset();
      tuple_ = nullptr;
    }
    return rc;
  } else {
    // 使用内存排序
    if (order_and_field_line.empty()) {
      tuple_holder_.reset();
      tuple_ = nullptr;
      return RC::RECORD_EOF;
    }

    Tuple *next_tuple = order_and_field_line.top().second;
    order_and_field_line.pop();
    tuple_holder_.reset(next_tuple);
    tuple_ = tuple_holder_.get();
    return RC::SUCCESS;
  }
}

RC OrderByPhysicalOperator::close()
{
  if (use_external_sort_ && external_sorter_) {
    external_sorter_->close();
    external_sorter_.reset();
  }
  tuple_holder_.reset();
  tuple_ = nullptr;
  return children_[0]->close();
}

Tuple *OrderByPhysicalOperator::current_tuple() { return tuple_; }

size_t OrderByPhysicalOperator::get_available_memory() const
{
  // 外部排序的缓冲区大小
  // 策略：使用固定的、合理的缓冲区大小
  // - 太小（如256KB）：会产生过多的run文件，merge阶段效率低
  // - 太大（如64MB）：可能与其他操作（JOIN、BufferPool）竞争内存
  // - 合理值：5MB - 足够容纳约6000-16000个tuple，减少run文件数量
  
  const size_t SORT_BUFFER_SIZE = 5 * 1024 * 1024;  // 5MB
  
  LOG_DEBUG("external sort buffer size: %lu bytes", SORT_BUFFER_SIZE);
  return SORT_BUFFER_SIZE;
}

RC OrderByPhysicalOperator::external_sort_open(Trx *trx)
{
  size_t memory_limit = get_available_memory();

  // 创建外部排序器
  external_sorter_ = std::make_unique<ExternalSorter>(order_by_, memory_limit);

  // 执行排序
  RC rc = external_sorter_->sort(children_[0].get());
  if (rc != RC::SUCCESS) {
    LOG_WARN("external sort failed");
    return rc;
  }

  LOG_INFO("external sort completed successfully");
  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::memory_sort_open(Trx *trx)
{
  // 原有的内存排序逻辑
  return fetch_and_sort_tables();
}

size_t OrderByPhysicalOperator::estimate_input_rows() const
{
  return estimate_input_rows_internal(children_[0].get());
}

size_t OrderByPhysicalOperator::estimate_input_rows_internal(PhysicalOperator *op) const
{
  if (op == nullptr) {
    return 0;
  }

  switch (op->type()) {
    case PhysicalOperatorType::TABLE_SCAN:
    case PhysicalOperatorType::TABLE_SCAN_VEC:
      return 1000;
    case PhysicalOperatorType::INDEX_SCAN:
      return 500;
    case PhysicalOperatorType::VECTOR_INDEX_SCAN:
      return 200;
    case PhysicalOperatorType::GRACE_HASH_JOIN:
    case PhysicalOperatorType::NESTED_LOOP_JOIN: {
      size_t left_rows  = op->children().size() > 0 ? estimate_input_rows_internal(op->children()[0].get()) : 1000;
      size_t right_rows = op->children().size() > 1 ? estimate_input_rows_internal(op->children()[1].get()) : 1000;
      // 对乘积做上限，避免估计爆炸，同时保证至少返回两侧的较大值
      const size_t cap = 200000;
      size_t       estimate = left_rows * std::max<size_t>(right_rows, 1);
      if (estimate > cap) {
        estimate = cap;
      }
      estimate = std::max<size_t>(estimate, std::max(left_rows, right_rows));

      // 如果JOIN层级较深，提升估计结果，触发外部排序
      int join_depth = calc_join_depth(op);
      if (join_depth >= 3) {
        estimate = std::max<size_t>(estimate, 150000);
      } else if (join_depth == 2) {
        estimate = std::max<size_t>(estimate, 80000);
      }
      return estimate;
    }
    case PhysicalOperatorType::PREDICATE:
    case PhysicalOperatorType::PREDICATE_VEC:
    case PhysicalOperatorType::PROJECT:
    case PhysicalOperatorType::PROJECT_VEC:
    case PhysicalOperatorType::LIMIT:
    case PhysicalOperatorType::ORDER_BY:
    case PhysicalOperatorType::CALC:
    case PhysicalOperatorType::EXPR_VEC:
    case PhysicalOperatorType::SCALAR_GROUP_BY:
    case PhysicalOperatorType::HASH_GROUP_BY:
    case PhysicalOperatorType::GROUP_BY_VEC:
    case PhysicalOperatorType::AGGREGATE_VEC:
    case PhysicalOperatorType::UNION:
      if (!op->children().empty()) {
        return estimate_input_rows_internal(op->children()[0].get());
      }
      return 1000;
    default:
      return 1000;
  }
}

PhysicalOperator *OrderByPhysicalOperator::unwrap_single_child(PhysicalOperator *op) const
{
  if (op == nullptr) {
    return nullptr;
  }
  switch (op->type()) {
    case PhysicalOperatorType::PROJECT:
    case PhysicalOperatorType::PROJECT_VEC:
    case PhysicalOperatorType::PREDICATE:
    case PhysicalOperatorType::PREDICATE_VEC:
    case PhysicalOperatorType::LIMIT:
    case PhysicalOperatorType::EXPR_VEC:
    case PhysicalOperatorType::ORDER_BY:
    case PhysicalOperatorType::CALC:
    case PhysicalOperatorType::UNION:
      if (!op->children().empty()) {
        return unwrap_single_child(op->children()[0].get());
      }
      return op;
    default:
      return op;
  }
}

int OrderByPhysicalOperator::calc_join_depth(PhysicalOperator *op) const
{
  if (op == nullptr) {
    return 0;
  }
  PhysicalOperator *real_op = unwrap_single_child(op);
  if (real_op == nullptr) {
    return 0;
  }

  switch (real_op->type()) {
    case PhysicalOperatorType::NESTED_LOOP_JOIN:
    case PhysicalOperatorType::GRACE_HASH_JOIN: {
      int left_depth  = real_op->children().size() > 0 ? calc_join_depth(real_op->children()[0].get()) : 0;
      int right_depth = real_op->children().size() > 1 ? calc_join_depth(real_op->children()[1].get()) : 0;
      return 1 + std::max(left_depth, right_depth);
    }
    default:
      if (!real_op->children().empty()) {
        return calc_join_depth(real_op->children()[0].get());
      }
      return 0;
  }
}

size_t OrderByPhysicalOperator::estimate_tuple_size() const
{
  TupleSchema schema;
  RC          rc = children_[0]->tuple_schema(schema);
  if (rc == RC::SUCCESS && schema.cell_num() > 0) {
    const size_t base_overhead = 64;
    const size_t per_cell      = 24;
    size_t       estimate      = base_overhead + per_cell * static_cast<size_t>(schema.cell_num());
    int          join_depth    = calc_join_depth(children_[0].get());
    if (join_depth > 0) {
      estimate += static_cast<size_t>(join_depth) * 64;
    }
    return std::max<size_t>(estimate, 300);
  }

  PhysicalOperator *effective_child = unwrap_single_child(children_[0].get());
  PhysicalOperatorType child_type   = effective_child != nullptr ? effective_child->type() : children_[0]->type();

  if (child_type == PhysicalOperatorType::NESTED_LOOP_JOIN || child_type == PhysicalOperatorType::GRACE_HASH_JOIN) {
    return 1000;
  }

  return 300;
}

size_t OrderByPhysicalOperator::get_sort_memory_threshold() const
{
  // 内存排序的阈值设置策略：
  // 1. 小数据集（< 10MB）：直接内存排序，性能最优
  // 2. 中等数据集（10-50MB）：仍可内存排序，但接近上限
  // 3. 大数据集（> 50MB）：必须使用外部排序
  
  // TODO: 从配置文件读取
  // size_t threshold = Config::get_instance().get_int("order_by_memory_threshold", 50 * 1024 * 1024);
  
  // 当前实现：固定阈值 50MB
  // 这个值的选择考虑：
  // - 需要为其他操作（JOIN、BufferPool等）预留内存
  // - 50MB可以容纳约6-16万行数据（取决于tuple大小）
  // - 对于4表JOIN的160,000行场景（约136MB），会触发外部排序
  
  const size_t DEFAULT_THRESHOLD = 50 * 1024 * 1024;  // 50 MB
  
  return DEFAULT_THRESHOLD;
}
