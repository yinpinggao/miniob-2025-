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

#include "sql/operator/external_sort/external_sorter.h"
#include "sql/operator/external_sort/tuple_serializer.h"
#include "common/log/log.h"
#include <algorithm>
#include <memory>

ExternalSorter::ExternalSorter(const std::vector<OrderBySqlNode> &order_by, size_t memory_limit)
    : order_by_(order_by), memory_limit_(memory_limit), current_memory_(0), merge_heap_(nullptr), sorted_(false)
{}

ExternalSorter::~ExternalSorter() { close(); }

RC ExternalSorter::sort(PhysicalOperator *input)
{
  if (sorted_) {
    LOG_WARN("already sorted");
    return RC::INTERNAL;
  }

  // Phase 1: 生成run文件
  RC rc = generate_runs(input);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to generate runs");
    return rc;
  }

  // Phase 2: 多路归并
  if (!run_files_.empty()) {
    rc = merge_runs();
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to merge runs");
      return rc;
    }
  }

  sorted_ = true;
  LOG_INFO("external sort completed, %lu run files generated", run_files_.size());
  return RC::SUCCESS;
}

RC ExternalSorter::generate_runs(PhysicalOperator *input)
{
  LOG_INFO("generating runs with memory limit: %lu bytes", memory_limit_);

  while (true) {
    // 从输入读取tuple
    RC rc = input->next();
    if (rc == RC::RECORD_EOF) {
      // 处理完最后一批数据
      if (!buffer_.empty()) {
        rc = flush_buffer_to_run();
        if (rc != RC::SUCCESS) {
          return rc;
        }
      }
      break;
    }

    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get next tuple from input");
      return rc;
    }

    // 优化：将JoinedTuple扁平化为ValueListTuple，避免深拷贝
    Tuple *original_tuple = input->current_tuple();
    Tuple *tuple          = flatten_tuple(original_tuple);
    if (tuple == nullptr) {
      LOG_WARN("failed to flatten tuple");
      return RC::INTERNAL;
    }
    size_t tuple_size = estimate_tuple_memory(tuple);

    // 检查是否需要刷新缓冲区（基于内存大小或tuple数量）
    const size_t MAX_TUPLES_PER_RUN = 1000;  // 限制每个run最多1000个tuple，降低内存峰值
    bool should_flush = (current_memory_ + tuple_size > memory_limit_ && !buffer_.empty()) ||
                        (buffer_.size() >= MAX_TUPLES_PER_RUN);
    
    if (should_flush) {
      rc = flush_buffer_to_run();
      if (rc != RC::SUCCESS) {
        delete tuple;
        return rc;
      }
    }

    buffer_.push_back(tuple);
    current_memory_ += tuple_size;
  }

  LOG_INFO("generated %lu run files", run_files_.size());
  
  // 重置缓存，因为归并阶段会重新反序列化
  cached_specs_.clear();
  specs_cached_ = false;
  
  return RC::SUCCESS;
}

RC ExternalSorter::flush_buffer_to_run()
{
  if (buffer_.empty()) {
    return RC::SUCCESS;
  }

  // 在内存中排序
  std::sort(buffer_.begin(), buffer_.end(), [this](const Tuple *a, const Tuple *b) {
    return compare_tuples(b, a);  // 注意：这里反过来是为了得到升序
  });

  // 创建run文件
  std::string run_file = temp_file_mgr_.create_temp_file("run");
  run_files_.push_back(run_file);

  std::ofstream ofs(run_file, std::ios::binary);
  if (!ofs.is_open()) {
    LOG_WARN("failed to create run file: %s", run_file.c_str());
    return RC::IOERR_OPEN;
  }

  // 写入tuple数量
  size_t tuple_count = buffer_.size();
  ofs.write(reinterpret_cast<const char *>(&tuple_count), sizeof(tuple_count));

  // 写入所有tuple
  for (Tuple *tuple : buffer_) {
    RC rc = TupleSerializer::serialize(ofs, tuple);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to serialize tuple to run file");
      ofs.close();
      return rc;
    }
    delete tuple;  // 释放内存
  }

  ofs.close();

  // 清空缓冲区
  buffer_.clear();
  current_memory_ = 0;

  LOG_DEBUG("flushed run file: %s, tuples: %lu", run_file.c_str(), tuple_count);
  return RC::SUCCESS;
}

RC ExternalSorter::merge_runs()
{
  if (run_files_.empty()) {
    return RC::SUCCESS;
  }

  LOG_INFO("merging %lu run files", run_files_.size());

  // 创建run读取器
  readers_.reserve(run_files_.size());
  for (const auto &run_file : run_files_) {
    auto reader = std::make_unique<RunReader>(run_file);
    RC   rc     = reader->open();
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to open run reader for %s", run_file.c_str());
      return rc;
    }
    readers_.push_back(std::move(reader));
  }

  // 创建最小堆用于多路归并
  HeapCompare heap_compare = [this](const HeapNode &a, const HeapNode &b) {
    return compare_tuples(a.tuple, b.tuple);  // 最小堆，所以a > b返回true
  };
  merge_heap_ = new std::priority_queue<HeapNode, std::vector<HeapNode>, HeapCompare>(heap_compare);

  // 从每个reader读取第一个tuple放入堆
  for (size_t i = 0; i < readers_.size(); i++) {
    Tuple *tuple = nullptr;
    RC     rc    = readers_[i]->next(tuple);
    if (rc == RC::SUCCESS && tuple != nullptr) {
      merge_heap_->push(HeapNode(tuple, i));
    }
  }

  LOG_INFO("merge heap initialized with %lu initial tuples", merge_heap_->size());
  return RC::SUCCESS;
}

RC ExternalSorter::next(Tuple *&tuple)
{
  if (!sorted_) {
    LOG_WARN("not sorted yet");
    return RC::INTERNAL;
  }

  if (merge_heap_ == nullptr || merge_heap_->empty()) {
    return RC::RECORD_EOF;
  }

  // 从堆顶取出最小元素
  HeapNode node = merge_heap_->top();
  merge_heap_->pop();

  tuple = node.tuple;

  // 从该reader读取下一个元素
  Tuple *next_tuple = nullptr;
  RC     rc         = readers_[node.reader_index]->next(next_tuple);
  if (rc == RC::SUCCESS && next_tuple != nullptr) {
    merge_heap_->push(HeapNode(next_tuple, node.reader_index));
  }

  return RC::SUCCESS;
}

RC ExternalSorter::close()
{
  // 清理缓冲区
  for (Tuple *tuple : buffer_) {
    delete tuple;
  }
  buffer_.clear();
  current_memory_ = 0;

  // 关闭readers
  readers_.clear();

  // 清理堆
  if (merge_heap_ != nullptr) {
    while (!merge_heap_->empty()) {
      HeapNode node = merge_heap_->top();
      merge_heap_->pop();
      delete node.tuple;
    }
    delete merge_heap_;
    merge_heap_ = nullptr;
  }

  // 清理临时文件
  temp_file_mgr_.cleanup_all();
  run_files_.clear();

  // 清理缓存的specs
  cached_specs_.clear();
  specs_cached_ = false;

  sorted_ = false;
  return RC::SUCCESS;
}

bool ExternalSorter::compare_tuples(const Tuple *t1, const Tuple *t2) const
{
  const auto *vl1 = dynamic_cast<const ValueListTuple *>(t1);
  const auto *vl2 = dynamic_cast<const ValueListTuple *>(t2);
  if (vl1 != nullptr && vl2 != nullptr && vl1->has_order_keys() && vl2->has_order_keys() &&
      vl1->order_keys().size() == order_by_.size() && vl2->order_keys().size() == order_by_.size()) {
    for (size_t i = 0; i < order_by_.size(); i++) {
      int cmp = vl1->order_keys()[i].compare(vl2->order_keys()[i]);
      if (cmp != 0) {
        return order_by_[i].is_asc ? (cmp > 0) : (cmp < 0);
      }
    }
  }

  for (const auto &order : order_by_) {
    Value v1, v2;
    RC    rc1 = order.expr->get_value(*t1, v1);
    RC    rc2 = order.expr->get_value(*t2, v2);

    if (rc1 != RC::SUCCESS || rc2 != RC::SUCCESS) {
      continue;
    }

    int cmp = v1.compare(v2);
    if (cmp != 0) {
      // is_asc = true 表示升序，v1 < v2时返回false（即t1 < t2）
      // is_asc = false 表示降序，v1 < v2时返回true（即t1 > t2）
      return order.is_asc ? (cmp > 0) : (cmp < 0);
    }
  }

  // 键值完全相同，再比较整行数据确保严格弱序
  int row_cmp = 0;
  RC  rc      = t1->compare(*t2, row_cmp);
  if (rc == RC::SUCCESS && row_cmp != 0) {
    return row_cmp > 0;
  }

  // 最后使用指针地址打破平局，避免容器认为完全相等
  return t1 > t2;
}

size_t ExternalSorter::estimate_tuple_memory(const Tuple *tuple) const
{
  // 简单估算：每个cell 按最大值估算
  // 更精确的估算可以使用TupleSerializer::estimate_size
  return TupleSerializer::estimate_size(tuple) + sizeof(Tuple *);
}

Tuple *ExternalSorter::flatten_tuple(const Tuple *tuple) const
{
  if (tuple == nullptr) {
    return nullptr;
  }

  int cell_num = tuple->cell_num();
  
  // 提取所有cell的值 - 必须进行深拷贝！
  // 因为RowTuple::cell_at返回的Value可能只是指向record数据的引用(own_data_=false)
  // 当JOIN操作符重用RowTuple时，这些引用会失效，导致所有tuple显示相同数据
  std::vector<Value> values;
  values.reserve(cell_num);
  for (int i = 0; i < cell_num; i++) {
    Value temp_value;
    RC    rc = tuple->cell_at(i, temp_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get cell at %d from tuple", i);
      return nullptr;
    }
    
    // 强制深拷贝：使用set_value确保数据被完全复制，拥有独立的内存
    Value deep_copied_value;
    deep_copied_value.set_value(temp_value);
    values.push_back(std::move(deep_copied_value));
  }

  // 优化：缓存 TupleCellSpec，避免重复复制
  // 所有tuple的schema是相同的，只需要提取一次
  if (!specs_cached_) {
    cached_specs_.clear();
    cached_specs_.reserve(cell_num);
    for (int i = 0; i < cell_num; i++) {
      TupleCellSpec spec;
      RC            rc = tuple->spec_at(i, spec);
      if (rc != RC::SUCCESS) {
        // 如果没有spec，使用默认的
        cached_specs_.push_back(TupleCellSpec("", "", nullptr));
      } else {
        cached_specs_.push_back(spec);
      }
    }
    specs_cached_ = true;
    LOG_DEBUG("cached %lu TupleCellSpecs for schema", cached_specs_.size());
  }

  // 创建ValueListTuple，使用缓存的specs
  auto value_list_tuple = std::make_unique<ValueListTuple>();
  value_list_tuple->set_cells(values);
  value_list_tuple->set_names(cached_specs_);  // 使用缓存的specs

  std::vector<Value> order_keys;
  order_keys.reserve(order_by_.size());
  bool keys_valid = true;
  for (const auto &order : order_by_) {
    Value key;
    RC    rc = order.expr->get_value(*tuple, key);
    if (rc != RC::SUCCESS) {
      keys_valid = false;
      break;
    }
    Value deep_key;
    deep_key.set_value(key);
    order_keys.emplace_back(std::move(deep_key));
  }
  if (keys_valid) {
    value_list_tuple->set_order_keys(std::move(order_keys));
  }

  return value_list_tuple.release();
}

// RunReader implementation
ExternalSorter::RunReader::RunReader(const std::string &filename)
    : filename_(filename), current_tuple_(nullptr), has_next_(true)
{}

ExternalSorter::RunReader::~RunReader() { close(); }

RC ExternalSorter::RunReader::open()
{
  file_.open(filename_, std::ios::binary);
  if (!file_.is_open()) {
    LOG_WARN("failed to open run file: %s", filename_.c_str());
    return RC::IOERR_OPEN;
  }

  // 读取tuple数量（但暂时不使用）
  size_t tuple_count;
  file_.read(reinterpret_cast<char *>(&tuple_count), sizeof(tuple_count));

  LOG_DEBUG("opened run file: %s, tuples: %lu", filename_.c_str(), tuple_count);
  return RC::SUCCESS;
}

RC ExternalSorter::RunReader::next(Tuple *&tuple)
{
  if (!has_next_) {
    return RC::RECORD_EOF;
  }

  tuple = nullptr;

  Tuple *new_tuple = nullptr;
  RC     rc        = TupleSerializer::deserialize(file_, new_tuple);
  if (rc == RC::RECORD_EOF) {
    has_next_ = false;
    return RC::RECORD_EOF;
  }
  if (rc != RC::SUCCESS) {
    if (new_tuple != nullptr) {
      delete new_tuple;
      new_tuple = nullptr;
    }
    LOG_WARN("failed to deserialize tuple");
    has_next_ = false;
    return rc;
  }

  current_tuple_ = new_tuple;
  tuple          = current_tuple_;
  current_tuple_ = nullptr;
  return RC::SUCCESS;
}

void ExternalSorter::RunReader::close()
{
  if (file_.is_open()) {
    file_.close();
  }
  if (current_tuple_ != nullptr) {
    delete current_tuple_;
    current_tuple_ = nullptr;
  }
  has_next_ = false;
}
