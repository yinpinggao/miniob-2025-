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
// Created for Grace Hash Join - External Hash Join
//

#pragma once

#include "sql/operator/physical_operator.h"
#include "sql/operator/external_sort/temp_file_manager.h"
#include "sql/parser/parse.h"
#include <unordered_map>
#include <memory>

/**
 * @brief Grace Hash Join - 外部Hash Join算子
 * @details 两阶段算法：
 * 1. Partition阶段：将左右表按hash分区，写入临时文件
 * 2. Join阶段：逐个分区加载到内存进行hash join
 * @ingroup PhysicalOperator
 */
class GraceHashJoinPhysicalOperator : public PhysicalOperator
{
public:
  GraceHashJoinPhysicalOperator(size_t memory_limit, size_t num_partitions = 16);
  virtual ~GraceHashJoinPhysicalOperator();

  PhysicalOperatorType type() const override { return PhysicalOperatorType::NESTED_LOOP_JOIN; }

  RC     open(Trx *trx) override;
  RC     next() override;
  RC     close() override;
  Tuple *current_tuple() override;

private:
  /**
   * @brief 分区阶段：将左右表分区到临时文件
   */
  RC partition_phase();

  /**
   * @brief 分区一个输入算子的所有tuple
   * @param input 输入算子
   * @param partition_files 分区文件列表（输出）
   */
  RC partition_input(PhysicalOperator *input, std::vector<std::string> &partition_files);

  /**
   * @brief 连接阶段：逐个分区进行hash join
   */
  RC join_phase();

  /**
   * @brief 对单个分区进行hash join
   * @param partition_id 分区ID
   */
  RC join_partition(size_t partition_id);

  /**
   * @brief 计算tuple的hash值用于分区
   */
  size_t hash_tuple(const Tuple *tuple) const;

  /**
   * @brief 从hash表中查找匹配的tuple
   */
  bool probe_hash_table(const Tuple *probe_tuple);

  /**
   * @brief 扁平化tuple（避免深拷贝）
   */
  Tuple *flatten_tuple(const Tuple *tuple) const;

private:
  Trx *trx_ = nullptr;

  PhysicalOperator *left_  = nullptr;
  PhysicalOperator *right_ = nullptr;

  size_t memory_limit_;    // 内存限制（字节）
  size_t num_partitions_;  // 分区数量

  TempFileManager temp_file_mgr_;  // 临时文件管理

  std::vector<std::string> left_partition_files_;   // 左表分区文件
  std::vector<std::string> right_partition_files_;  // 右表分区文件

  // Join阶段的状态
  size_t current_partition_;  // 当前处理的分区
  bool   partition_done_;     // 分区阶段是否完成

  // 当前分区的hash表（左表在内存中）
  struct HashEntry
  {
    Tuple                *tuple;
    std::vector<Tuple *> matches;  // 对于多对多join
  };
  std::vector<Tuple *> build_tuples_;  // 左表（build side）的所有tuple

  // Probe阶段的状态
  std::ifstream  right_partition_stream_;  // 右表分区文件流
  size_t         right_partition_count_;   // 右表分区的tuple数量
  size_t         right_partition_index_;   // 当前右表分区的index
  Tuple         *current_left_tuple_;      // 当前左表tuple
  Tuple         *current_right_tuple_;     // 当前右表tuple
  JoinedTuple    joined_tuple_;            // 当前join结果
  size_t         build_index_;             // build side遍历索引
  bool           partition_join_done_;     // 当前分区join是否完成
};

