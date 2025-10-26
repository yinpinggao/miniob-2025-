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
#include "common/log/log.h"
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

OrderByPhysicalOperator::OrderByPhysicalOperator(vector<OrderBySqlNode> order_by) 
  : order_by_(std::move(order_by))
{
}

OrderByPhysicalOperator::~OrderByPhysicalOperator()
{
  cleanup_temp_files();
  if (current_output_tuple_) {
    delete current_output_tuple_;
    current_output_tuple_ = nullptr;
  }
}

RC OrderByPhysicalOperator::open(Trx *trx)
{
  if (children_.size() != 1) {
    LOG_WARN("order by operator should have exactly one child");
    return RC::INTERNAL;
  }

  RC rc = children_[0]->open(trx);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open child operator. rc=%s", strrc(rc));
    return rc;
  }

  // 创建临时目录
  rc = create_temp_dir();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to create temp directory. rc=%s", strrc(rc));
    return rc;
  }

  // Phase 1: 生成排序run文件
  rc = generate_sorted_runs();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to generate sorted runs. rc=%s", strrc(rc));
    return rc;
  }

  // Phase 2: 打开所有run文件准备归并
  rc = open_merge_phase();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open merge phase. rc=%s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::next()
{
  if (merge_heap_.empty()) {
    return RC::RECORD_EOF;
  }

  // 从堆顶取最小记录
  RunReader* min_reader = merge_heap_.top();
  merge_heap_.pop();

  // 构造输出tuple
  if (current_output_tuple_) {
    delete current_output_tuple_;
  }
  current_output_tuple_ = new ValueListTuple();
  current_output_tuple_->set_cells(min_reader->current_row);
  
  // 设置specs（从子算子的tuple获取）
  // 由于我们只需要输出数据，specs可以为空或使用简单的spec
  std::vector<TupleCellSpec> specs;
  for (size_t i = 0; i < min_reader->current_row.size(); i++) {
    specs.push_back(TupleCellSpec("", ""));  // 空的spec
  }
  current_output_tuple_->set_names(specs);

  // 读取该run的下一条记录
  RC rc = min_reader->read_next();
  if (rc == RC::SUCCESS) {
    merge_heap_.push(min_reader);
  } else if (rc != RC::RECORD_EOF) {
    LOG_WARN("failed to read next record from run. rc=%s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::close()
{
  cleanup_temp_files();
  return children_[0]->close();
}

Tuple *OrderByPhysicalOperator::current_tuple()
{
  return current_output_tuple_;
}

// ==================== Phase 1: 生成排序run ====================

RC OrderByPhysicalOperator::generate_sorted_runs()
{
  int run_id = 0;
  RC rc = RC::SUCCESS;

  while (true) {
    rc = children_[0]->next();
    if (rc == RC::RECORD_EOF) {
      break;
    }
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to get next tuple from child. rc=%s", strrc(rc));
      return rc;
    }

    Tuple* tuple = children_[0]->current_tuple();
    if (tuple == nullptr) {
      LOG_WARN("current tuple is null");
      return RC::INTERNAL;
    }

    RowData row;

    // 提取排序键
    for (auto& order_node : order_by_) {
      Value key;
      rc = order_node.expr->get_value(*tuple, key);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to get order by value. rc=%s", strrc(rc));
        return rc;
      }
      row.keys.push_back(std::move(key));
    }

    // 提取完整行
    int cell_num = tuple->cell_num();
    for (int i = 0; i < cell_num; i++) {
      Value cell;
      rc = tuple->cell_at(i, cell);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to get cell at index %d. rc=%s", i, strrc(rc));
        return rc;
      }
      row.row.push_back(std::move(cell));
    }

    size_t row_size = estimate_row_size(row.row);

    // 检查内存限制
    if (buffer_memory_usage_ + row_size > RUN_MEMORY_LIMIT && !sort_buffer_.empty()) {
      // 达到内存上限，刷写到文件
      rc = flush_sort_buffer(run_id++);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to flush sort buffer. rc=%s", strrc(rc));
        return rc;
      }

      sort_buffer_.clear();
      buffer_memory_usage_ = 0;
    }

    sort_buffer_.push_back(std::move(row));
    buffer_memory_usage_ += row_size;
  }

  // 刷写最后一批
  if (!sort_buffer_.empty()) {
    rc = flush_sort_buffer(run_id++);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to flush final sort buffer. rc=%s", strrc(rc));
      return rc;
    }
  }

  LOG_INFO("generated %d sorted runs", run_id);
  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::flush_sort_buffer(int run_id)
{
  // 排序
  std::sort(sort_buffer_.begin(), sort_buffer_.end());

  // 写入文件
  char filename[256];
  snprintf(filename, sizeof(filename), "%s/run_%06d.tmp", temp_dir_.c_str(), run_id);
  
  std::ofstream out(filename, std::ios::binary);
  if (!out.is_open()) {
    LOG_WARN("failed to open run file: %s", filename);
    return RC::IOERR_OPEN;
  }

  for (auto& row : sort_buffer_) {
    // 先写排序键，再写完整行
    RC rc = serialize_row(row.keys, out);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to serialize keys. rc=%s", strrc(rc));
      out.close();
      return rc;
    }
    
    rc = serialize_row(row.row, out);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to serialize row. rc=%s", strrc(rc));
      out.close();
      return rc;
    }
  }

  out.close();
  run_files_.push_back(filename);

  LOG_INFO("flushed run %d with %zu rows to %s", run_id, sort_buffer_.size(), filename);
  return RC::SUCCESS;
}

// ==================== Phase 2: 归并 ====================

RC OrderByPhysicalOperator::open_merge_phase()
{
  if (run_files_.empty()) {
    LOG_INFO("no run files to merge");
    return RC::SUCCESS;
  }

  // 为每个run文件创建reader
  for (size_t i = 0; i < run_files_.size(); i++) {
    auto reader = std::make_unique<RunReader>();
    reader->run_id = i;
    reader->parent = this;
    reader->file.open(run_files_[i], std::ios::binary);
    
    if (!reader->file.is_open()) {
      LOG_WARN("failed to open run file: %s", run_files_[i].c_str());
      return RC::IOERR_OPEN;
    }

    // 读取第一条记录
    RC rc = reader->read_next();
    if (rc == RC::SUCCESS) {
      readers_.push_back(std::move(reader));
    } else if (rc != RC::RECORD_EOF) {
      LOG_WARN("failed to read first record from run %d. rc=%s", (int)i, strrc(rc));
      return rc;
    }
    // RC::RECORD_EOF 说明文件为空，跳过
  }

  // 将所有reader放入堆中
  for (auto& reader : readers_) {
    merge_heap_.push(reader.get());
  }

  LOG_INFO("opened %zu run files for merging", readers_.size());
  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::RunReader::read_next()
{
  if (!file.is_open() || file.eof()) {
    has_data = false;
    return RC::RECORD_EOF;
  }

  current_keys.clear();
  current_row.clear();

  // 先读排序键
  RC rc = parent->deserialize_row(file, current_keys);
  if (OB_FAIL(rc)) {
    if (file.eof()) {
      has_data = false;
      return RC::RECORD_EOF;
    }
    LOG_WARN("failed to deserialize keys. rc=%s", strrc(rc));
    return rc;
  }

  // 再读完整行
  rc = parent->deserialize_row(file, current_row);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to deserialize row. rc=%s", strrc(rc));
    return rc;
  }

  has_data = true;
  return RC::SUCCESS;
}

// ==================== 序列化 ====================

RC OrderByPhysicalOperator::serialize_row(const std::vector<Value>& row, std::ostream& out)
{
  // 写入列数
  uint32_t col_count = row.size();
  out.write(reinterpret_cast<const char*>(&col_count), sizeof(col_count));

  // 写入每一列
  for (const auto& value : row) {
    RC rc = serialize_value(value, out);
    if (OB_FAIL(rc)) {
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::deserialize_row(std::istream& in, std::vector<Value>& row)
{
  // 读取列数
  uint32_t col_count = 0;
  in.read(reinterpret_cast<char*>(&col_count), sizeof(col_count));
  if (in.eof() || in.fail()) {
    return RC::RECORD_EOF;
  }

  row.clear();
  row.reserve(col_count);

  // 读取每一列
  for (uint32_t i = 0; i < col_count; i++) {
    Value value;
    RC rc = deserialize_value(in, value);
    if (OB_FAIL(rc)) {
      return rc;
    }
    row.push_back(std::move(value));
  }

  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::serialize_value(const Value& value, std::ostream& out)
{
  // 写入类型
  AttrType type = value.attr_type();
  out.write(reinterpret_cast<const char*>(&type), sizeof(type));

  // 写入是否为NULL
  bool is_null = value.is_null();
  out.write(reinterpret_cast<const char*>(&is_null), sizeof(is_null));

  if (is_null) {
    return RC::SUCCESS;
  }

  // 根据类型写入数据
  switch (type) {
    case AttrType::INTS: {
      int32_t val = value.get_int();
      out.write(reinterpret_cast<const char*>(&val), sizeof(val));
      break;
    }
    case AttrType::FLOATS: {
      float val = value.get_float();
      out.write(reinterpret_cast<const char*>(&val), sizeof(val));
      break;
    }
    case AttrType::BOOLEANS: {
      bool val = value.get_boolean();
      out.write(reinterpret_cast<const char*>(&val), sizeof(val));
      break;
    }
    case AttrType::CHARS: {
      const char* str = value.get_string().c_str();
      uint32_t len = value.get_string().length();
      out.write(reinterpret_cast<const char*>(&len), sizeof(len));
      out.write(str, len);
      break;
    }
    case AttrType::DATES: {
      int32_t val = value.get_int();  // Date stored as int
      out.write(reinterpret_cast<const char*>(&val), sizeof(val));
      break;
    }
    default: {
      LOG_WARN("unsupported attr type: %d", static_cast<int>(type));
      return RC::INTERNAL;
    }
  }

  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::deserialize_value(std::istream& in, Value& value)
{
  // 读取类型
  AttrType type;
  in.read(reinterpret_cast<char*>(&type), sizeof(type));
  if (in.eof() || in.fail()) {
    return RC::RECORD_EOF;
  }

  // 读取是否为NULL
  bool is_null;
  in.read(reinterpret_cast<char*>(&is_null), sizeof(is_null));
  if (in.fail()) {
    return RC::IOERR_READ;
  }

  if (is_null) {
    value = Value(NullValue());
    value.set_type(type);
    return RC::SUCCESS;
  }

  // 根据类型读取数据
  switch (type) {
    case AttrType::INTS: {
      int32_t val;
      in.read(reinterpret_cast<char*>(&val), sizeof(val));
      if (in.fail()) return RC::IOERR_READ;
      value = Value(val);
      break;
    }
    case AttrType::FLOATS: {
      float val;
      in.read(reinterpret_cast<char*>(&val), sizeof(val));
      if (in.fail()) return RC::IOERR_READ;
      value = Value(val);
      break;
    }
    case AttrType::BOOLEANS: {
      bool val;
      in.read(reinterpret_cast<char*>(&val), sizeof(val));
      if (in.fail()) return RC::IOERR_READ;
      value = Value(val);
      break;
    }
    case AttrType::CHARS: {
      uint32_t len;
      in.read(reinterpret_cast<char*>(&len), sizeof(len));
      if (in.fail()) return RC::IOERR_READ;
      
      std::vector<char> buffer(len + 1);
      in.read(buffer.data(), len);
      if (in.fail()) return RC::IOERR_READ;
      buffer[len] = '\0';
      
      value = Value(buffer.data());
      break;
    }
    case AttrType::DATES: {
      int32_t val;
      in.read(reinterpret_cast<char*>(&val), sizeof(val));
      if (in.fail()) return RC::IOERR_READ;
      // Date stored as int, use set_data
      value.set_type(AttrType::DATES);
      value.set_data(reinterpret_cast<char*>(&val), sizeof(val));
      break;
    }
    default: {
      LOG_WARN("unsupported attr type: %d", static_cast<int>(type));
      return RC::INTERNAL;
    }
  }

  return RC::SUCCESS;
}

// ==================== 工具方法 ====================

size_t OrderByPhysicalOperator::estimate_row_size(const std::vector<Value>& row)
{
  size_t total = 0;
  for (const auto& value : row) {
    total += sizeof(Value);  // Value对象本身
    if (value.attr_type() == AttrType::CHARS && !value.is_null()) {
      total += value.get_string().length();
    }
  }
  return total;
}

int OrderByPhysicalOperator::compare_keys(const std::vector<Value>& a, const std::vector<Value>& b) const
{
  size_t key_count = std::min(a.size(), b.size());
  
  for (size_t i = 0; i < key_count; i++) {
    int cmp = a[i].compare(b[i]);
    
    if (cmp != 0) {
      // 根据是否升序调整比较结果
      if (i < order_by_.size() && !order_by_[i].is_asc) {
        return -cmp;  // 降序
      }
      return cmp;
    }
  }
  
  return 0;
}

bool OrderByPhysicalOperator::RowData::operator<(const RowData& other) const
{
  size_t key_count = std::min(keys.size(), other.keys.size());
  
  for (size_t i = 0; i < key_count; i++) {
    int cmp = keys[i].compare(other.keys[i]);
    if (cmp != 0) {
      return cmp < 0;  // 始终升序排序，降序在比较时处理
    }
  }
  
  return false;
}

bool OrderByPhysicalOperator::RunReaderComparator::operator()(RunReader* a, RunReader* b) const
{
  if (!a->has_data) return true;
  if (!b->has_data) return false;
  
  // 注意：priority_queue是最大堆，我们需要最小堆，所以反转比较结果
  // 现在排序键已经单独存储在current_keys中
  
  size_t key_count = std::min(a->current_keys.size(), b->current_keys.size());
  
  for (size_t i = 0; i < key_count; i++) {
    int cmp = a->current_keys[i].compare(b->current_keys[i]);
    
    if (cmp != 0) {
      // 根据是否升序调整比较结果
      if (i < a->parent->order_by_.size() && !a->parent->order_by_[i].is_asc) {
        cmp = -cmp;
      }
      // priority_queue 是最大堆，返回 true 表示 a 的优先级低于 b
      return cmp > 0;
    }
  }
  
  return false;
}

RC OrderByPhysicalOperator::create_temp_dir()
{
  // 创建临时目录: /tmp/miniob_sort_<pid>_<timestamp>
  char dir_name[256];
  snprintf(dir_name, sizeof(dir_name), "/tmp/miniob_sort_%d_%ld", 
           getpid(), time(nullptr));
  
  temp_dir_ = dir_name;
  
  if (mkdir(temp_dir_.c_str(), 0755) != 0) {
    if (errno != EEXIST) {
      LOG_WARN("failed to create temp directory: %s, errno=%d", temp_dir_.c_str(), errno);
      return RC::IOERR_ACCESS;
    }
  }
  
  temp_dir_created_ = true;
  LOG_INFO("created temp directory: %s", temp_dir_.c_str());
  return RC::SUCCESS;
}

void OrderByPhysicalOperator::cleanup_temp_files()
{
  // 先清空堆（包含指向readers的指针）
  while (!merge_heap_.empty()) {
    merge_heap_.pop();
  }

  // 关闭所有打开的文件并清理readers
  for (auto& reader : readers_) {
    if (reader->file.is_open()) {
      reader->file.close();
    }
  }
  readers_.clear();

  // 删除临时文件
  for (const auto& file : run_files_) {
    unlink(file.c_str());
  }
  run_files_.clear();

  // 删除临时目录
  if (temp_dir_created_ && !temp_dir_.empty()) {
    rmdir(temp_dir_.c_str());
    temp_dir_created_ = false;
  }
}
