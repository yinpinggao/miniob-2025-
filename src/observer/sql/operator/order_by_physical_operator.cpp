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
  // TODO: 从子算子获取统计信息
  // 当前实现：使用启发式估算
  
  // 方法1: 尝试从子算子获取预估行数（如果实现了统计信息接口）
  // if (children_[0]->has_row_estimate()) {
  //   return children_[0]->get_estimated_rows();
  // }
  
  // 方法2: 根据子算子类型进行启发式估算
  PhysicalOperatorType child_type = children_[0]->type();
  
  switch (child_type) {
    case PhysicalOperatorType::TABLE_SCAN:
      // 表扫描：假设是大表，使用保守估计
      return 100000;  // 假设10万行
      
    case PhysicalOperatorType::INDEX_SCAN:
      // 索引扫描：通常返回较少行
      return 10000;   // 假设1万行
      
    case PhysicalOperatorType::NESTED_LOOP_JOIN:
      // JOIN：可能产生大量数据，使用保守估计
      return 100000;  // 假设10万行（笛卡尔积可能更多）
      
    case PhysicalOperatorType::PREDICATE:
      // 过滤：递归估算子算子，然后应用选择率
      if (!children_[0]->children().empty()) {
        // 假设过滤掉50%的数据
        return estimate_input_rows() / 2;
      }
      return 10000;
      
    case PhysicalOperatorType::PROJECT:
      // 投影不改变行数，递归估算
      return 10000;
      
    default:
      // 其他情况：使用中等规模的保守估计
      return 10000;   // 默认1万行
  }
}

size_t OrderByPhysicalOperator::estimate_tuple_size() const
{
  // TODO: 从schema精确计算tuple大小
  // 当前实现：使用经验值估算
  
  // 考虑因素：
  // 1. JoinedTuple的嵌套层数（每层增加指针开销）
  // 2. 每个字段的类型和大小
  // 3. Tuple对象本身的开销（vtable指针、对齐等）
  
  // 启发式估算：
  // - 基础tuple（RowTuple）：约200-400字节（包括20个int字段 + 开销）
  // - JoinedTuple每层嵌套：额外16字节（2个指针）
  // - 多层JOIN场景（如4表JOIN）：需要考虑3层嵌套
  
  PhysicalOperatorType child_type = children_[0]->type();
  
  if (child_type == PhysicalOperatorType::NESTED_LOOP_JOIN) {
    // JOIN结果：考虑JoinedTuple的嵌套结构
    // 保守估计：每个tuple约800字节（适用于多表JOIN）
    return 800;
  } 
  
  // 简单表扫描或单表操作：每个tuple约300字节
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
