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

#pragma once

#include "sql/operator/physical_operator.h"
#include "sql/expr/tuple.h"
#include "sql/expr/expression_tuple.h"
#include <functional>
#include <queue>
#include <fstream>
#include <memory>

class OrderByPhysicalOperator : public PhysicalOperator
{
public:
  OrderByPhysicalOperator(std::vector<OrderBySqlNode> order_by);

  virtual ~OrderByPhysicalOperator();

  PhysicalOperatorType type() const override { return PhysicalOperatorType::ORDER_BY; }

  std::vector<OrderBySqlNode> &order_by() { return order_by_; }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

private:
  // ========== 配置参数 ==========
  static constexpr size_t RUN_MEMORY_LIMIT = 30 * 1024 * 1024;   // 30MB per run
  static constexpr size_t RUN_BUFFER_SIZE = 1 * 1024 * 1024;     // 1MB per reader
  
  // ========== Phase 1: 排序数据结构 ==========
  struct RowData {
    std::vector<Value> keys;    // 排序键值
    std::vector<Value> row;     // 完整行数据
    
    bool operator<(const RowData& other) const;
  };
  
  std::vector<RowData> sort_buffer_;
  size_t buffer_memory_usage_ = 0;
  
  // ========== Phase 2: 归并数据结构 ==========
  struct RunReader {
    int run_id;
    std::ifstream file;
    std::vector<Value> current_keys;    // 当前排序键
    std::vector<Value> current_row;     // 当前完整行
    bool has_data = false;
    OrderByPhysicalOperator* parent = nullptr;  // 用于访问比较函数
    
    RC read_next();
  };
  
  struct RunReaderComparator {
    bool operator()(RunReader* a, RunReader* b) const;
  };
  
  std::vector<std::unique_ptr<RunReader>> readers_;
  std::priority_queue<RunReader*, std::vector<RunReader*>, RunReaderComparator> merge_heap_;
  
  // ========== 临时文件管理 ==========
  std::string temp_dir_;
  std::vector<std::string> run_files_;
  bool temp_dir_created_ = false;
  
  // ========== 输出管理 ==========
  ValueListTuple* current_output_tuple_ = nullptr;
  
  // ========== 排序字段 ==========
  std::vector<OrderBySqlNode> order_by_;
  
  // ========== 核心方法 ==========
  // Phase 1: 生成排序run
  RC generate_sorted_runs();
  RC flush_sort_buffer(int run_id);
  
  // Phase 2: 归并
  RC open_merge_phase();
  
  // 序列化
  RC serialize_row(const std::vector<Value>& row, std::ostream& out);
  RC deserialize_row(std::istream& in, std::vector<Value>& row);
  RC serialize_value(const Value& value, std::ostream& out);
  RC deserialize_value(std::istream& in, Value& value);
  
  // 工具方法
  size_t estimate_row_size(const std::vector<Value>& row);
  int compare_keys(const std::vector<Value>& a, const std::vector<Value>& b) const;
  RC create_temp_dir();
  void cleanup_temp_files();
};
