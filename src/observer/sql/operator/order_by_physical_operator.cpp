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

#ifdef WITH_MEMTRACER
#include "memtracer/mt_info.h"
#endif

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
  
  rc = children_[0]->open(trx);
  if (OB_FAIL(rc)) {
    return rc;
  }

  // 判断是否使用外部排序：
  // 1. 如果有 MemTracer 且内存受限，使用外部排序
  // 2. 否则使用内存排序（更高效，适合小数据集）
#ifdef WITH_MEMTRACER
  size_t memory_limit = memtracer::memory_limit();
  if (memory_limit > 0) {
    // 如果设置了内存限制，使用外部排序
    size_t available_memory = get_available_memory();
    LOG_INFO("using external sort with memory limit: %lu bytes", available_memory);
    use_external_sort_ = true;
    rc = external_sort_open(trx);
  } else {
    // 没有内存限制，使用内存排序
    use_external_sort_ = false;
    rc = memory_sort_open(trx);
  }
#else
  // 没有 MemTracer，使用传统的内存排序
  use_external_sort_ = false;
  rc = memory_sort_open(trx);
#endif

  return rc;
}

RC OrderByPhysicalOperator::next()
{
  if (use_external_sort_) {
    // 使用外部排序
    RC rc = external_sorter_->next(tuple_);
    return rc;
  } else {
    // 使用内存排序
    if (order_and_field_line.empty()) {
      return RC::RECORD_EOF;
    }

    tuple_ = order_and_field_line.top().second;
    order_and_field_line.pop();
    return RC::SUCCESS;
  }
}

RC OrderByPhysicalOperator::close()
{
  if (use_external_sort_ && external_sorter_) {
    external_sorter_->close();
    external_sorter_.reset();
  }
  return children_[0]->close();
}

Tuple *OrderByPhysicalOperator::current_tuple() { return tuple_; }

size_t OrderByPhysicalOperator::get_available_memory() const
{
#ifdef WITH_MEMTRACER
  size_t limit   = memtracer::memory_limit();
  size_t current = memtracer::allocated_memory();

  if (limit > 0) {
    // 使用较小比例的可用内存作为排序缓冲区，为JOIN等操作留足空间
    const float SAFETY_FACTOR   = 0.08;  // 降低到8%，为大JOIN留足空间
    size_t      available       = limit > current ? limit - current : 0;
    size_t      sort_buffer     = static_cast<size_t>(available * SAFETY_FACTOR);
    const size_t MIN_BUFFER_SIZE = 256 * 1024;   // 最小256KB
    const size_t MAX_BUFFER_SIZE = 3 * 1024 * 1024;  // 最大3MB，严格控制内存使用

    if (sort_buffer < MIN_BUFFER_SIZE) {
      sort_buffer = MIN_BUFFER_SIZE;
    }
    if (sort_buffer > MAX_BUFFER_SIZE) {
      sort_buffer = MAX_BUFFER_SIZE;
    }

    LOG_INFO("memory limit: %lu, current: %lu, available: %lu, sort buffer: %lu", limit, current, available,
        sort_buffer);
    return sort_buffer;
  }
#endif

  // 没有MemTracer或没有限制，使用默认值
  return 64 * 1024 * 1024;  // 默认64MB
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
