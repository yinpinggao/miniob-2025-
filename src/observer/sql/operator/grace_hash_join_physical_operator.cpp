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
// Created for Grace Hash Join
//

#include "sql/operator/grace_hash_join_physical_operator.h"
#include "sql/operator/external_sort/tuple_serializer.h"
#include "sql/expr/tuple.h"
#include "common/log/log.h"
#include <functional>

GraceHashJoinPhysicalOperator::GraceHashJoinPhysicalOperator(size_t memory_limit, size_t num_partitions)
    : memory_limit_(memory_limit),
      num_partitions_(num_partitions),
      current_partition_(0),
      partition_done_(false),
      right_partition_count_(0),
      right_partition_index_(0),
      current_left_tuple_(nullptr),
      current_right_tuple_(nullptr),
      build_index_(0),
      partition_join_done_(false)
{}

GraceHashJoinPhysicalOperator::~GraceHashJoinPhysicalOperator() { close(); }

RC GraceHashJoinPhysicalOperator::open(Trx *trx)
{
  if (children_.size() != 2) {
    LOG_WARN("grace hash join operator should have 2 children");
    return RC::INTERNAL;
  }

  trx_   = trx;
  left_  = children_[0].get();
  right_ = children_[1].get();

  // 打开左右表
  RC rc = left_->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open left operator");
    return rc;
  }

  rc = right_->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open right operator");
    left_->close();
    return rc;
  }

  // 执行分区阶段
  LOG_INFO("grace hash join: starting partition phase with %lu partitions", num_partitions_);
  rc = partition_phase();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed in partition phase");
    return rc;
  }

  partition_done_ = true;
  LOG_INFO("grace hash join: partition phase completed");

  // 关闭左右表（已经分区完成）
  left_->close();
  right_->close();

  // 准备join阶段
  current_partition_     = 0;
  partition_join_done_   = true;  // 初始状态为true，会触发加载第一个分区
  return RC::SUCCESS;
}

RC GraceHashJoinPhysicalOperator::partition_phase()
{
  // 分区左表
  RC rc = partition_input(left_, left_partition_files_);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to partition left table");
    return rc;
  }

  LOG_INFO("grace hash join: left table partitioned into %lu files", left_partition_files_.size());

  // 分区右表
  rc = partition_input(right_, right_partition_files_);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to partition right table");
    return rc;
  }

  LOG_INFO("grace hash join: right table partitioned into %lu files", right_partition_files_.size());

  return RC::SUCCESS;
}

RC GraceHashJoinPhysicalOperator::partition_input(PhysicalOperator *input, std::vector<std::string> &partition_files)
{
  // 创建分区文件
  partition_files.resize(num_partitions_);
  std::vector<std::ofstream> partition_streams(num_partitions_);
  std::vector<size_t>        partition_counts(num_partitions_, 0);

  for (size_t i = 0; i < num_partitions_; i++) {
    partition_files[i] = temp_file_mgr_.create_temp_file("partition");
    partition_streams[i].open(partition_files[i], std::ios::binary);
    if (!partition_streams[i].is_open()) {
      LOG_WARN("failed to create partition file: %s", partition_files[i].c_str());
      return RC::IOERR_OPEN;
    }
  }

  // 遍历输入，分区每个tuple
  size_t total_tuples = 0;
  while (true) {
    RC rc = input->next();
    if (rc == RC::RECORD_EOF) {
      break;
    }
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get next tuple from input");
      return rc;
    }

    Tuple *original_tuple = input->current_tuple();
    Tuple *tuple          = flatten_tuple(original_tuple);
    if (tuple == nullptr) {
      LOG_WARN("failed to flatten tuple");
      return RC::INTERNAL;
    }

    // 计算分区
    size_t partition_id = hash_tuple(tuple) % num_partitions_;

    // 序列化到对应分区文件
    rc = TupleSerializer::serialize(partition_streams[partition_id], tuple);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to serialize tuple to partition %lu", partition_id);
      delete tuple;
      return rc;
    }

    partition_counts[partition_id]++;
    total_tuples++;
    delete tuple;
  }

  // 写入每个分区的tuple数量（在文件开头）
  for (size_t i = 0; i < num_partitions_; i++) {
    partition_streams[i].close();

    // 重新打开文件，在开头写入count
    std::fstream fs(partition_files[i], std::ios::in | std::ios::out | std::ios::binary);
    if (fs.is_open()) {
      // 读取原内容
      fs.seekg(0, std::ios::end);
      size_t      file_size = fs.tellg();
      fs.seekg(0, std::ios::beg);
      std::vector<char> buffer(file_size);
      fs.read(buffer.data(), file_size);

      // 重写文件：先写count，再写原内容
      fs.close();
      std::ofstream ofs(partition_files[i], std::ios::binary | std::ios::trunc);
      size_t        count = partition_counts[i];
      ofs.write(reinterpret_cast<const char *>(&count), sizeof(count));
      ofs.write(buffer.data(), file_size);
      ofs.close();

      LOG_DEBUG("partition %lu: %lu tuples", i, count);
    }
  }

  LOG_INFO("partitioned %lu tuples into %lu partitions", total_tuples, num_partitions_);
  return RC::SUCCESS;
}

size_t GraceHashJoinPhysicalOperator::hash_tuple(const Tuple *tuple) const
{
  // 简单的hash函数：对所有cell的值进行hash
  std::hash<std::string> hasher;
  size_t                 hash_value = 0;

  int cell_num = tuple->cell_num();
  for (int i = 0; i < cell_num; i++) {
    Value value;
    RC    rc = tuple->cell_at(i, value);
    if (rc == RC::SUCCESS) {
      std::string str = value.to_string();
      hash_value ^= hasher(str) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
    }
  }

  return hash_value;
}

Tuple *GraceHashJoinPhysicalOperator::flatten_tuple(const Tuple *tuple) const
{
  if (tuple == nullptr) {
    return nullptr;
  }

  int cell_num = tuple->cell_num();

  // 提取所有cell的值
  std::vector<Value> values;
  values.reserve(cell_num);
  for (int i = 0; i < cell_num; i++) {
    Value value;
    RC    rc = tuple->cell_at(i, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get cell at %d from tuple", i);
      return nullptr;
    }
    values.push_back(value);
  }

  // 提取所有cell的spec
  std::vector<TupleCellSpec> specs;
  specs.reserve(cell_num);
  for (int i = 0; i < cell_num; i++) {
    TupleCellSpec spec;
    RC            rc = tuple->spec_at(i, spec);
    if (rc != RC::SUCCESS) {
      specs.push_back(TupleCellSpec("", "", nullptr));
    } else {
      specs.push_back(spec);
    }
  }

  // 创建ValueListTuple
  ValueListTuple *value_list_tuple = new ValueListTuple();
  value_list_tuple->set_cells(values);
  value_list_tuple->set_names(specs);

  return value_list_tuple;
}

RC GraceHashJoinPhysicalOperator::next()
{
  if (!partition_done_) {
    LOG_WARN("partition phase not completed");
    return RC::INTERNAL;
  }

  // 如果当前分区join完成，加载下一个分区
  while (partition_join_done_) {
    if (current_partition_ >= num_partitions_) {
      // 所有分区都处理完成
      return RC::RECORD_EOF;
    }

    // 加载新分区
    RC rc = join_partition(current_partition_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to join partition %lu", current_partition_);
      return rc;
    }

    current_partition_++;
    partition_join_done_ = false;
    build_index_         = 0;
    current_right_tuple_ = nullptr;
  }

  // 从当前分区获取下一个join结果
  // 使用nested loop方式遍历：对每个right tuple，遍历所有left tuples
  while (true) {
    // 如果没有当前right tuple，读取下一个
    if (current_right_tuple_ == nullptr) {
      if (right_partition_index_ >= right_partition_count_) {
        // 当前分区的right表遍历完成
        partition_join_done_ = true;

        // 清理当前分区的资源
        for (Tuple *t : build_tuples_) {
          delete t;
        }
        build_tuples_.clear();
        if (right_partition_stream_.is_open()) {
          right_partition_stream_.close();
        }

        return next();  // 递归调用，加载下一个分区
      }

      // 从right分区文件读取下一个tuple
      Tuple *tuple = nullptr;
      RC     rc    = TupleSerializer::deserialize(right_partition_stream_, tuple);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to deserialize right tuple");
        return rc;
      }

      current_right_tuple_ = tuple;
      right_partition_index_++;
      build_index_ = 0;  // 重置left表遍历索引
    }

    // 遍历left表（build side）
    if (build_index_ < build_tuples_.size()) {
      current_left_tuple_ = build_tuples_[build_index_];
      build_index_++;

      // 构造joined tuple
      joined_tuple_.set_left(current_left_tuple_);
      joined_tuple_.set_right(current_right_tuple_);

      return RC::SUCCESS;
    }

    // 当前right tuple的所有left tuples都遍历完，准备下一个right tuple
    delete current_right_tuple_;
    current_right_tuple_ = nullptr;
  }

  return RC::RECORD_EOF;
}

RC GraceHashJoinPhysicalOperator::join_partition(size_t partition_id)
{
  LOG_DEBUG("joining partition %lu", partition_id);

  // 清理之前的状态
  for (Tuple *t : build_tuples_) {
    delete t;
  }
  build_tuples_.clear();
  if (right_partition_stream_.is_open()) {
    right_partition_stream_.close();
  }

  // 加载左表分区到内存（build phase）
  std::ifstream left_stream(left_partition_files_[partition_id], std::ios::binary);
  if (!left_stream.is_open()) {
    LOG_WARN("failed to open left partition file: %s", left_partition_files_[partition_id].c_str());
    return RC::IOERR_OPEN;
  }

  // 读取左表分区的tuple数量
  size_t left_count = 0;
  left_stream.read(reinterpret_cast<char *>(&left_count), sizeof(left_count));

  LOG_DEBUG("partition %lu: loading %lu left tuples", partition_id, left_count);

  // 加载所有左表tuples到内存
  for (size_t i = 0; i < left_count; i++) {
    Tuple *tuple = nullptr;
    RC     rc    = TupleSerializer::deserialize(left_stream, tuple);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to deserialize left tuple %lu", i);
      left_stream.close();
      return rc;
    }
    build_tuples_.push_back(tuple);
  }

  left_stream.close();

  // 打开右表分区文件（probe phase）
  right_partition_stream_.open(right_partition_files_[partition_id], std::ios::binary);
  if (!right_partition_stream_.is_open()) {
    LOG_WARN("failed to open right partition file: %s", right_partition_files_[partition_id].c_str());
    return RC::IOERR_OPEN;
  }

  // 读取右表分区的tuple数量
  right_partition_stream_.read(reinterpret_cast<char *>(&right_partition_count_), sizeof(right_partition_count_));
  right_partition_index_ = 0;

  LOG_DEBUG("partition %lu: %lu left tuples, %lu right tuples", partition_id, left_count, right_partition_count_);

  return RC::SUCCESS;
}

RC GraceHashJoinPhysicalOperator::close()
{
  // 清理内存
  for (Tuple *t : build_tuples_) {
    delete t;
  }
  build_tuples_.clear();

  if (current_right_tuple_ != nullptr) {
    delete current_right_tuple_;
    current_right_tuple_ = nullptr;
  }

  if (right_partition_stream_.is_open()) {
    right_partition_stream_.close();
  }

  // 清理分区文件会在TempFileManager析构时自动完成
  left_partition_files_.clear();
  right_partition_files_.clear();

  return RC::SUCCESS;
}

Tuple *GraceHashJoinPhysicalOperator::current_tuple()
{
  // 合并base_rids（与NestedLoopJoin保持一致）
  auto left_base_rids  = joined_tuple_.left_base_rids();
  auto right_base_rids = joined_tuple_.right_base_rids();
  left_base_rids.insert(left_base_rids.end(), right_base_rids.begin(), right_base_rids.end());
  joined_tuple_.set_base_rids(left_base_rids);

  return &joined_tuple_;
}

