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
// Created for external sort
//

#pragma once

#include "sql/operator/physical_operator.h"
#include "sql/operator/external_sort/temp_file_manager.h"
#include "sql/parser/parse_defs.h"
#include "common/rc.h"
#include <vector>
#include <string>
#include <fstream>
#include <queue>
#include <functional>

/**
 * @brief 外部排序器，实现大数据集的排序
 * @details 使用两阶段外部排序算法：
 * 1. Run Generation: 生成排序好的run文件
 * 2. Multi-way Merge: 多路归并run文件
 */
class ExternalSorter
{
public:
  /**
   * @brief 构造函数
   * @param order_by 排序字段和顺序（注意：不复制，使用引用）
   * @param memory_limit 内存限制（字节）
   */
  ExternalSorter(const std::vector<OrderBySqlNode> &order_by, size_t memory_limit);

  ~ExternalSorter();

  /**
   * @brief 执行外部排序
   * @param input 输入算子
   * @return RC 操作结果
   */
  RC sort(PhysicalOperator *input);

  /**
   * @brief 获取下一个排序好的tuple
   * @param tuple 输出的tuple
   * @return RC::SUCCESS 成功，RC::RECORD_EOF 结束
   */
  RC next(Tuple *&tuple);

  /**
   * @brief 关闭排序器，清理资源
   */
  RC close();

private:
  /**
   * @brief Run读取器，用于从run文件读取tuple
   */
  class RunReader
  {
  public:
    RunReader(const std::string &filename);
    ~RunReader();

    RC    open();
    RC    next(Tuple *&tuple);
    bool  has_next() const { return has_next_; }
    void  close();
    Tuple *current() const { return current_tuple_; }

  private:
    std::string   filename_;
    std::ifstream file_;
    Tuple        *current_tuple_;
    bool          has_next_;
  };

  /**
   * @brief 堆节点，用于多路归并
   */
  struct HeapNode
  {
    Tuple *tuple;
    size_t reader_index;

    HeapNode(Tuple *t, size_t idx) : tuple(t), reader_index(idx) {}
  };

  /**
   * @brief Phase 1: 生成排序好的run文件
   */
  RC generate_runs(PhysicalOperator *input);

  /**
   * @brief Phase 2: 多路归并run文件
   */
  RC merge_runs();

  /**
   * @brief 比较两个tuple的大小
   * @return true if t1 > t2
   */
  bool compare_tuples(const Tuple *t1, const Tuple *t2) const;

  /**
   * @brief 刷新缓冲区到run文件
   */
  RC flush_buffer_to_run();

  /**
   * @brief 估算tuple的内存大小
   */
  size_t estimate_tuple_memory(const Tuple *tuple) const;

  /**
   * @brief 将tuple扁平化为ValueListTuple，避免JoinedTuple的深拷贝
   */
  Tuple *flatten_tuple(const Tuple *tuple) const;

private:
  const std::vector<OrderBySqlNode> &order_by_;       // 排序字段和顺序（引用，不复制）
  size_t                             memory_limit_;   // 内存限制
  TempFileManager                    temp_file_mgr_;  // 临时文件管理器
  std::vector<std::string>           run_files_;      // 生成的run文件列表
  std::vector<Tuple *>               buffer_;         // 内存缓冲区
  size_t                             current_memory_; // 当前内存使用
  std::vector<std::unique_ptr<RunReader>> readers_;   // Run读取器
  using HeapCompare = std::function<bool(const HeapNode &, const HeapNode &)>;
  std::priority_queue<HeapNode, std::vector<HeapNode>, HeapCompare> *merge_heap_;  // 归并堆
  bool sorted_;  // 是否已排序
};

