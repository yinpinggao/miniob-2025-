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

#include "sql/operator/physical_operator.h"
#include "sql/expr/tuple.h"
#include "sql/expr/expression_tuple.h"
#include <functional>
#include <queue>
#include <memory>

class ExternalSorter;

class OrderByPhysicalOperator : public PhysicalOperator
{
public:
  OrderByPhysicalOperator(std::vector<OrderBySqlNode> order_by);

  virtual ~OrderByPhysicalOperator();

  PhysicalOperatorType type() const override { return PhysicalOperatorType::ORDER_BY; }

  std::vector<OrderBySqlNode> &order_by() { return order_by_; }

  RC fetch_and_sort_tables();
  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

private:
  /**
   * @brief 获取可用内存大小
   */
  size_t get_available_memory() const;

  /**
   * @brief 使用外部排序
   */
  RC external_sort_open(Trx *trx);

  /**
   * @brief 使用内存排序（原有逻辑）
   */
  RC memory_sort_open(Trx *trx);

private:
  std::vector<OrderBySqlNode> order_by_;

  // 内存排序使用的数据结构
  using order_line = pair<vector<Value>, Tuple *>;
  using order_func = std::function<bool(const order_line &, const order_line &)>;
  using order_list = std::priority_queue<order_line, vector<order_line>, order_func>;
  order_list order_and_field_line;

  // 外部排序使用的数据结构
  std::unique_ptr<ExternalSorter> external_sorter_;
  bool                            use_external_sort_ = false;

  Tuple *tuple_ = nullptr;
};
