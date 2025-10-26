/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Wangyunlai on 2021/5/13.
//

#include <limits.h>
#include <string.h>
#include <cstdio>

#include <utility>

#include "common/defs.h"
#include "common/lang/string.h"
#include "common/lang/span.h"
#include "common/lang/algorithm.h"
#include "common/log/log.h"
#include "common/global_context.h"
#include "storage/db/db.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/common/condition_filter.h"
#include "storage/common/meta_util.h"
#include "storage/index/bplus_tree_index.h"
#include "storage/index/index.h"
#include "storage/record/record_manager.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include "storage/db/db.h"
#include "storage/index/ivfflat_index.h"
#include "sql/expr/expression.h"

Table::~Table()
{
  if (record_handler_ != nullptr) {
    delete record_handler_;
    record_handler_ = nullptr;
  }

  if (data_buffer_pool_ != nullptr) {
    data_buffer_pool_->close_file();
    data_buffer_pool_ = nullptr;
  }

  for (vector<Index *>::iterator it = indexes_.begin(); it != indexes_.end(); ++it) {
    Index *index = *it;
    delete index;
  }
  indexes_.clear();

  LOG_INFO("Table has been closed: %s", name());
}

RC Table::create(Db *db, int32_t table_id, const char *path, const char *name, const char *base_dir,
    span<const AttrInfoSqlNode> attributes, StorageFormat storage_format)
{
  if (table_id < 0) {
    LOG_WARN("invalid table id. table_id=%d, table_name=%s", table_id, name);
    return RC::INVALID_ARGUMENT;
  }

  if (common::is_blank(name)) {
    LOG_WARN("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }
  LOG_INFO("Begin to create table %s:%s", base_dir, name);

  if (attributes.size() == 0) {
    LOG_WARN("Invalid arguments. table_name=%s, attribute_count=%d", name, attributes.size());
    return RC::INVALID_ARGUMENT;
  }
  for (const auto &att : attributes) {
    if (att.type == AttrType::VECTORS) {
      if (att.length > 16000 * sizeof(float) + 1) {
        return RC::INVALID_ARGUMENT;
      }
    }
  }

  RC rc = RC::SUCCESS;

  // 使用 table_name.table记录一个表的元数据
  // 判断表文件是否已经存在
  int fd = ::open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (fd < 0) {
    if (EEXIST == errno) {
      LOG_ERROR("Failed to create table file, it has been created. %s, EEXIST, %s", path, strerror(errno));
      return RC::SCHEMA_TABLE_EXIST;
    }
    LOG_ERROR("Create table file failed. filename=%s, errmsg=%d:%s", path, errno, strerror(errno));
    return RC::IOERR_OPEN;
  }

  close(fd);

  // 创建文件
  const vector<FieldMeta> *trx_fields = db->trx_kit().trx_fields();
  if ((rc = table_meta_.init(table_id, TableType::Table, true, name, trx_fields, attributes, storage_format)) !=
      RC::SUCCESS) {
    LOG_ERROR("Failed to init table meta. name:%s, ret:%d", name, rc);
    return rc;  // delete table file
  }

  fstream fs;
  fs.open(path, ios_base::out | ios_base::binary);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open file for write. file name=%s, errmsg=%s", path, strerror(errno));
    return RC::IOERR_OPEN;
  }

  // 记录元数据到文件中
  table_meta_.serialize(fs);
  fs.close();

  db_       = db;
  base_dir_ = base_dir;

  string             data_file = table_data_file(base_dir, name);
  BufferPoolManager &bpm       = db->buffer_pool_manager();
  rc                           = bpm.create_file(data_file.c_str());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create disk buffer pool of data file. file name=%s", data_file.c_str());
    return rc;
  }

  rc = init_record_handler(base_dir);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create table %s due to init record handler failed.", data_file.c_str());
    // don't need to remove the data_file
    return rc;
  }

  LOG_INFO("Successfully create table %s:%s", base_dir, name);
  return rc;
}

RC Table::drop()
{
  auto rc = sync();  // 刷新所有脏页
  if (rc != RC::SUCCESS) {
    return rc;
  }

  auto       table_name = name();
  error_code ec;
  auto       path = table_meta_file(base_dir_.c_str(), table_name);
  if (!filesystem::remove(path, ec)) {
    LOG_ERROR("Drop table meta fail: %s. error=%s", path.c_str(), strerror(errno));
    return RC::IOERR_WRITE;
  }

  path = table_data_file(base_dir_.c_str(), table_name);
  if (!filesystem::remove(path, ec)) {
    LOG_ERROR("Drop table data fail: %s. error=%s", path.c_str(), strerror(errno));
    return RC::IOERR_WRITE;
  }

  auto index_num = table_meta_.index_num();
  for (int i = 0; i < index_num; ++i) {
    ((BplusTreeIndex *)indexes_[i])->close();
    auto index_name = table_meta_.index(i)->name();
    path            = table_index_file(base_dir_.c_str(), table_name, index_name);
    if (!filesystem::remove(path, ec)) {
      LOG_ERROR("Drop table index data fail: %s. error=%s", path.c_str(), strerror(errno));
      return RC::IOERR_WRITE;
    }
  }

  return RC::SUCCESS;
}

RC Table::open(Db *db, const char *meta_file, const char *base_dir)
{
  // 加载元数据文件
  fstream fs;
  string  meta_file_path = string(base_dir) + common::FILE_PATH_SPLIT_STR + meta_file;
  fs.open(meta_file_path, ios_base::in | ios_base::binary);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open meta file for read. file name=%s, errmsg=%s", meta_file_path.c_str(), strerror(errno));
    return RC::IOERR_OPEN;
  }
  if (table_meta_.deserialize(fs) < 0) {
    LOG_ERROR("Failed to deserialize table meta. file name=%s", meta_file_path.c_str());
    fs.close();
    return RC::INTERNAL;
  }
  fs.close();

  db_       = db;
  base_dir_ = base_dir;

  // 加载数据文件
  RC rc = init_record_handler(base_dir);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to open table %s due to init record handler failed.", base_dir);
    // don't need to remove the data_file
    return rc;
  }

  const int index_num = table_meta_.index_num();
  for (int i = 0; i < index_num; i++) {
    const IndexMeta *index_meta = table_meta_.index(i);

    BplusTreeIndex *index      = new BplusTreeIndex();
    string          index_file = table_index_file(base_dir, name(), index_meta->name());

    rc = index->open(this, index_file.c_str(), *index_meta);
    if (rc != RC::SUCCESS) {
      delete index;
      LOG_ERROR("Failed to open index. table=%s, index=%s, file=%s, rc=%s",
                name(), index_meta->name(), index_file.c_str(), strrc(rc));
      // skip cleanup
      //  do all cleanup action in destructive Table function.
      return rc;
    }
    indexes_.push_back(index);
  }

  return rc;
}

RC Table::insert_record(Record &record)
{
  RC rc = RC::SUCCESS;
  rc    = record_handler_->insert_record(record.data(), table_meta_.record_size(), &record.rid());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Insert record failed. table name=%s, rc=%s", table_meta_.name(), strrc(rc));
    return rc;
  }

  rc = insert_entry_of_indexes(record.data(), record.rid());
  if (rc != RC::SUCCESS) {  // 可能出现了键值重复
    RC rc2 = delete_entry_of_indexes(record.data(), record.rid(), false /*error_on_not_exists*/);
    if (rc2 != RC::SUCCESS) {
      LOG_ERROR("Failed to rollback index data when insert index entries failed. table name=%s, rc=%d:%s",
                name(), rc2, strrc(rc2));
    }
    rc2 = record_handler_->delete_record(&record.rid());
    if (rc2 != RC::SUCCESS) {
      LOG_PANIC("Failed to rollback record data when insert index entries failed. table name=%s, rc=%d:%s",
                name(), rc2, strrc(rc2));
    }
  }
  return rc;
}

RC Table::visit_record(const RID &rid, const function<bool(Record &)> &visitor)
{
  return record_handler_->visit_record(rid, visitor);
}

RC Table::get_record(const RID &rid, Record &record)
{
  RC rc = record_handler_->get_record(rid, record);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to visit record. rid=%s, table=%s, rc=%s", rid.to_string().c_str(), name(), strrc(rc));
    return rc;
  }

  return rc;
}

RC Table::recover_insert_record(Record &record)
{
  RC rc = RC::SUCCESS;
  rc    = record_handler_->recover_insert_record(record.data(), table_meta_.record_size(), record.rid());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Insert record failed. table name=%s, rc=%s", table_meta_.name(), strrc(rc));
    return rc;
  }

  rc = insert_entry_of_indexes(record.data(), record.rid());
  if (rc != RC::SUCCESS) {  // 可能出现了键值重复
    RC rc2 = delete_entry_of_indexes(record.data(), record.rid(), false /*error_on_not_exists*/);
    if (rc2 != RC::SUCCESS) {
      LOG_ERROR("Failed to rollback index data when insert index entries failed. table name=%s, rc=%d:%s",
                name(), rc2, strrc(rc2));
    }
    rc2 = record_handler_->delete_record(&record.rid());
    if (rc2 != RC::SUCCESS) {
      LOG_PANIC("Failed to rollback record data when insert index entries failed. table name=%s, rc=%d:%s",
                name(), rc2, strrc(rc2));
    }
  }
  return rc;
}

RC Table::init_record_handler(const char *base_dir)
{
  string data_file = table_data_file(base_dir, table_meta_.name());

  BufferPoolManager &bpm = db_->buffer_pool_manager();
  RC                 rc  = bpm.open_file(db_->log_handler(), data_file.c_str(), data_buffer_pool_);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to open disk buffer pool for file:%s. rc=%d:%s", data_file.c_str(), rc, strrc(rc));
    return rc;
  }

  record_handler_ = new RecordFileHandler(table_meta_.storage_format());

  rc = record_handler_->init(*data_buffer_pool_, db_->log_handler(), &table_meta_);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to init record handler. rc=%s", strrc(rc));
    data_buffer_pool_->close_file();
    data_buffer_pool_ = nullptr;
    delete record_handler_;
    record_handler_ = nullptr;
    return rc;
  }

  return rc;
}

RC Table::get_record_scanner(RecordFileScanner &scanner, Trx *trx, ReadWriteMode mode)
{
  RC rc = scanner.open_scan(this, *data_buffer_pool_, trx, db_->log_handler(), mode, nullptr);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("failed to open scanner. rc=%s", strrc(rc));
  }
  return rc;
}

RC Table::get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode)
{
  RC rc = scanner.open_scan_chunk(this, *data_buffer_pool_, db_->log_handler(), mode);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("failed to open scanner. rc=%s", strrc(rc));
  }
  return rc;
}

RC Table::create_index(
    Trx *trx, IndexType index_type, const vector<FieldMeta> &field_meta, const char *index_name, bool unique)
{
  if (common::is_blank(index_name)) {
    LOG_INFO("Invalid input arguments, table name is %s, index_name is blank or attribute_name is blank", name());
    return RC::INVALID_ARGUMENT;
  }

  IndexMeta new_index_meta;

  RC rc = new_index_meta.init(index_name, index_type, field_meta, unique);
  if (rc != RC::SUCCESS) {
    LOG_INFO("Failed to init IndexMeta in table:%s, index:%s",
             name(), new_index_meta.to_string().c_str());
    return rc;
  }

  // 创建索引相关数据
  BplusTreeIndex *index      = new BplusTreeIndex();
  string          index_file = table_index_file(base_dir_.c_str(), name(), index_name);

  rc = index->create(this, index_file.c_str(), new_index_meta);
  if (rc != RC::SUCCESS) {
    delete index;
    LOG_ERROR("Failed to create bplus tree index. file name=%s, rc=%d:%s", index_file.c_str(), rc, strrc(rc));
    return rc;
  }

  // 遍历当前的所有数据，插入这个索引
  RecordFileScanner scanner;
  rc = get_record_scanner(scanner, trx, ReadWriteMode::READ_ONLY);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create scanner while creating index. table=%s, index=%s, rc=%s", 
             name(), index_name, strrc(rc));
    return rc;
  }

  Record record;
  while (OB_SUCC(rc = scanner.next(record))) {
    rc = index->insert_entry(record.data(), &record.rid());
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to insert record into index while creating index. table=%s, index=%s, rc=%s",
               name(), index_name, strrc(rc));
      return rc;
    }
  }
  if (RC::RECORD_EOF == rc) {
    rc = RC::SUCCESS;
  } else {
    LOG_WARN("failed to insert record into index while creating index. table=%s, index=%s, rc=%s",
             name(), index_name, strrc(rc));
    return rc;
  }
  scanner.close_scan();
  LOG_INFO("inserted all records into new index. table=%s, index=%s", name(), index_name);

  indexes_.push_back(index);

  /// 接下来将这个索引放到表的元数据中
  TableMeta new_table_meta(table_meta_);
  rc = new_table_meta.add_index(new_index_meta);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to add index (%s) on table (%s). error=%d:%s", index_name, name(), rc, strrc(rc));
    return rc;
  }

  /// 内存中有一份元数据，磁盘文件也有一份元数据。修改磁盘文件时，先创建一个临时文件，写入完成后再rename为正式文件
  /// 这样可以防止文件内容不完整
  // 创建元数据临时文件
  string  tmp_file = table_meta_file(base_dir_.c_str(), name()) + ".tmp";
  fstream fs;
  fs.open(tmp_file, ios_base::out | ios_base::binary | ios_base::trunc);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open file for write. file name=%s, errmsg=%s", tmp_file.c_str(), strerror(errno));
    return RC::IOERR_OPEN;  // 创建索引中途出错，要做还原操作
  }
  if (new_table_meta.serialize(fs) < 0) {
    LOG_ERROR("Failed to dump new table meta to file: %s. sys err=%d:%s", tmp_file.c_str(), errno, strerror(errno));
    return RC::IOERR_WRITE;
  }
  fs.close();

  // 覆盖原始元数据文件
  string meta_file = table_meta_file(base_dir_.c_str(), name());

  int ret = rename(tmp_file.c_str(), meta_file.c_str());
  if (ret != 0) {
    LOG_ERROR("Failed to rename tmp meta file (%s) to normal meta file (%s) while creating index (%s) on table (%s). "
              "system error=%d:%s",
              tmp_file.c_str(), meta_file.c_str(), index_name, name(), errno, strerror(errno));
    return RC::IOERR_WRITE;
  }

  table_meta_.swap(new_table_meta);

  LOG_INFO("Successfully added a new index (%s) on the table (%s)", index_name, name());
  return rc;
}

RC Table::drop_index(const char *index_name)
{
  if (common::is_blank(index_name)) {
    LOG_WARN("Invalid input arguments while dropping index. table=%s", name());
    return RC::INVALID_ARGUMENT;
  }

  int   index_pos    = -1;
  Index *target_index = nullptr;
  for (size_t i = 0; i < indexes_.size(); i++) {
    if (0 == strcmp(indexes_[i]->index_meta().name(), index_name)) {
      index_pos    = static_cast<int>(i);
      target_index = indexes_[i];
      break;
    }
  }

  if (target_index == nullptr) {
    LOG_WARN("index %s not found on table %s", index_name, name());
    return RC::SCHEMA_INDEX_NOT_EXIST;
  }

  const IndexMeta *index_meta = table_meta_.index(index_name);
  if (index_meta == nullptr) {
    LOG_WARN("missing index meta while dropping index. table=%s index=%s", name(), index_name);
    return RC::SCHEMA_INDEX_NOT_EXIST;
  }

  TableMeta new_table_meta(table_meta_);
  RC        rc = new_table_meta.remove_index(index_name);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to remove index meta. table=%s index=%s rc=%s", name(), index_name, strrc(rc));
    return rc;
  }

  auto dump_meta_to_tmp = [&](const TableMeta &meta, const std::string &tmp_file) -> RC {
    fstream fs;
    fs.open(tmp_file, ios_base::out | ios_base::binary | ios_base::trunc);
    if (!fs.is_open()) {
      LOG_ERROR("Failed to open file for write. file name=%s, errmsg=%s", tmp_file.c_str(), strerror(errno));
      return RC::IOERR_OPEN;
    }
    if (meta.serialize(fs) < 0) {
      LOG_ERROR("Failed to dump table meta to file: %s. sys err=%d:%s", tmp_file.c_str(), errno, strerror(errno));
      return RC::IOERR_WRITE;
    }
    fs.close();
    return RC::SUCCESS;
  };

  auto close_index = [&](Index *index) -> RC {
    if (index->is_vector_index()) {
      auto *vector_index = dynamic_cast<IvfflatIndex *>(index);
      return vector_index->close();
    }
    auto *btree_index = dynamic_cast<BplusTreeIndex *>(index);
    return btree_index->close();
  };

  auto reopen_index = [&](Index *index) -> RC {
    string index_file_path = table_index_file(base_dir_.c_str(), name(), index_name);
    if (index->is_vector_index()) {
      auto *vector_index = dynamic_cast<IvfflatIndex *>(index);
      ASSERT(!index_meta->fields().empty(), "vector index meta has no fields");
      return vector_index->open(this, index_file_path.c_str(), *index_meta, index_meta->fields()[0]);
    }
    auto *btree_index = dynamic_cast<BplusTreeIndex *>(index);
    return btree_index->open(this, index_file_path.c_str(), *index_meta);
  };

  string tmp_meta_file = table_meta_file(base_dir_.c_str(), name()) + ".tmp";
  rc                  = dump_meta_to_tmp(new_table_meta, tmp_meta_file);
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = close_index(target_index);
  if (OB_FAIL(rc)) {
    LOG_WARN("Failed to close index %s while dropping. table=%s rc=%s", index_name, name(), strrc(rc));
    ::remove(tmp_meta_file.c_str());
    return rc;
  }

  string index_file  = table_index_file(base_dir_.c_str(), name(), index_name);
  string backup_file = index_file + ".bak";
  if (0 != ::remove(backup_file.c_str()) && errno != ENOENT) {
    LOG_ERROR("Failed to remove stale backup index file: %s. err=%s", backup_file.c_str(), strerror(errno));
    reopen_index(target_index);
    ::remove(tmp_meta_file.c_str());
    return RC::FILE_REMOVE;
  }

  if (0 != ::rename(index_file.c_str(), backup_file.c_str())) {
    LOG_ERROR("Failed to rename index file to backup. file=%s err=%s", index_file.c_str(), strerror(errno));
    reopen_index(target_index);
    ::remove(tmp_meta_file.c_str());
    return RC::IOERR_WRITE;
  }

  string meta_file = table_meta_file(base_dir_.c_str(), name());
  if (0 != ::rename(tmp_meta_file.c_str(), meta_file.c_str())) {
    LOG_ERROR("Failed to update meta file while dropping index %s on table %s. err=%s", index_name, name(), strerror(errno));
    if (0 != ::rename(backup_file.c_str(), index_file.c_str())) {
      LOG_PANIC("Failed to restore index file during drop rollback. file=%s err=%s", index_file.c_str(), strerror(errno));
    }
    ::remove(tmp_meta_file.c_str());
    RC reopen_rc = reopen_index(target_index);
    if (OB_FAIL(reopen_rc)) {
      LOG_PANIC("Failed to reopen index after drop rollback. table=%s idx=%s rc=%s", name(), index_name, strrc(reopen_rc));
    }
    return RC::IOERR_WRITE;
  }

  table_meta_.swap(new_table_meta);

  if (0 != ::remove(backup_file.c_str())) {
    LOG_ERROR("Failed to delete index file %s after dropping index %s. err=%s", backup_file.c_str(), index_name, strerror(errno));

    RC revert_rc = dump_meta_to_tmp(new_table_meta, tmp_meta_file);
    if (OB_FAIL(revert_rc)) {
      LOG_PANIC("Failed to dump original meta while rolling back drop index. table=%s rc=%s", name(), strrc(revert_rc));
      ::remove(tmp_meta_file.c_str());
    } else {
      if (0 != ::rename(tmp_meta_file.c_str(), meta_file.c_str())) {
        LOG_PANIC("Failed to restore table meta file while rolling back drop index. table=%s err=%s", name(), strerror(errno));
      } else {
        table_meta_.swap(new_table_meta);
      }
    }

    if (0 != ::rename(backup_file.c_str(), index_file.c_str())) {
      LOG_PANIC("Failed to restore index file from backup. file=%s err=%s", index_file.c_str(), strerror(errno));
    }
    RC reopen_rc = reopen_index(target_index);
    if (OB_FAIL(reopen_rc)) {
      LOG_PANIC("Failed to reopen index after rollback. table=%s idx=%s rc=%s", name(), index_name, strrc(reopen_rc));
    }
    return RC::FILE_REMOVE;
  }

  indexes_.erase(indexes_.begin() + index_pos);
  delete target_index;

  LOG_INFO("Successfully dropped index (%s) on the table (%s)", index_name, name());
  return RC::SUCCESS;
}

RC Table::create_vector_index(Trx *trx, IndexType index_type, const vector<FieldMeta> &field_meta,
    const char *index_name, NormalFunctionType distance_type, const std::vector<int> &options)
{
  if (common::is_blank(index_name)) {
    LOG_INFO("Invalid input arguments, table name is %s, index_name is blank or attribute_name is blank", name());
    return RC::INVALID_ARGUMENT;
  }

  IndexMeta new_index_meta;

  RC rc = new_index_meta.init(index_name, index_type, field_meta);
  if (rc != RC::SUCCESS) {
    LOG_INFO("Failed to init IndexMeta in table:%s, index:%s",
             name(), new_index_meta.to_string().c_str());
    return rc;
  }

  // 创建索引相关数据
  auto   index      = new IvfflatIndex();
  string index_file = table_index_file(base_dir_.c_str(), name(), index_name);
  rc                = index->create(this, index_file.c_str(), new_index_meta, field_meta[0]);
  if (rc != RC::SUCCESS) {
    delete index;
    LOG_ERROR("Failed to create Ivfflat index. file name=%s, rc=%d:%s", index_file.c_str(), rc, strrc(rc));
    return rc;
  }

  // 遍历当前的所有数据，插入这个索引
  RecordFileScanner scanner;
  rc = get_record_scanner(scanner, trx, ReadWriteMode::READ_ONLY);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create scanner while creating vector index. table=%s, index=%s, rc=%s",
             name(), index_name, strrc(rc));
    return rc;
  }

  // 一次性把某向量类型列数据都读出来
  Record                                          record;
  std::vector<std::pair<std::vector<float>, RID>> data;
  while (OB_SUCC(rc = scanner.next(record))) {
    Value value;
    // 目前向量仅在一列上建立索引
    rc = record.get_field(field_meta[0], value);
    if (OB_FAIL(rc)) {
      return rc;
    }
    data.emplace_back(value.get_vector(), record.rid());
  }

  if (RC::RECORD_EOF == rc) {
    rc = RC::SUCCESS;
  } else {
    LOG_WARN("failed to get record while creating index. table=%s, index=%s, rc=%s",
             name(), index_name, strrc(rc));
    return rc;
  }

  scanner.close_scan();

  // 建立向量索引
  index->build_index(data, distance_type, options);

  LOG_INFO("inserted all records into new index. table=%s, index=%s", name(), index_name);

  indexes_.emplace_back(index);

  /// 接下来将这个索引放到表的元数据中
  TableMeta new_table_meta(table_meta_);
  rc = new_table_meta.add_index(new_index_meta);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to add index (%s) on table (%s). error=%d:%s", index_name, name(), rc, strrc(rc));
    return rc;
  }

  /// 内存中有一份元数据，磁盘文件也有一份元数据。修改磁盘文件时，先创建一个临时文件，写入完成后再rename为正式文件
  /// 这样可以防止文件内容不完整
  // 创建元数据临时文件
  string  tmp_file = table_meta_file(base_dir_.c_str(), name()) + ".tmp";
  fstream fs;
  fs.open(tmp_file, ios_base::out | ios_base::binary | ios_base::trunc);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open file for write. file name=%s, errmsg=%s", tmp_file.c_str(), strerror(errno));
    return RC::IOERR_OPEN;  // 创建索引中途出错，要做还原操作
  }
  if (new_table_meta.serialize(fs) < 0) {
    LOG_ERROR("Failed to dump new table meta to file: %s. sys err=%d:%s", tmp_file.c_str(), errno, strerror(errno));
    return RC::IOERR_WRITE;
  }
  fs.close();

  // 覆盖原始元数据文件
  string meta_file = table_meta_file(base_dir_.c_str(), name());

  int ret = rename(tmp_file.c_str(), meta_file.c_str());
  if (ret != 0) {
    LOG_ERROR("Failed to rename tmp meta file (%s) to normal meta file (%s) while creating index (%s) on table (%s). "
              "system error=%d:%s",
              tmp_file.c_str(), meta_file.c_str(), index_name, name(), errno, strerror(errno));
    return RC::IOERR_WRITE;
  }

  table_meta_.swap(new_table_meta);

  LOG_INFO("Successfully added a new index (%s) on the table (%s)", index_name, name());
  return rc;
}

RC Table::delete_record(const RID &rid)
{
  RC     rc = RC::SUCCESS;
  Record record;
  rc = get_record(rid, record);
  if (OB_FAIL(rc)) {
    return rc;
  }

  return delete_record(record);
}

RC Table::delete_record(const Record &record)
{
  RC rc = RC::SUCCESS;
  for (Index *index : indexes_) {
    rc = index->delete_entry(record.data(), &record.rid());
    ASSERT(RC::SUCCESS == rc, 
           "failed to delete entry from index. table name=%s, index name=%s, rid=%s, rc=%s",
           name(), index->index_meta().name(), record.rid().to_string().c_str(), strrc(rc));
  }
  rc = record_handler_->delete_record(&record.rid());
  return rc;
}

RC Table::update_record(const Record &old_record, const Record &new_record)
{
  RC rc = RC::SUCCESS;
  // 维护索引，先删除后插入
  for (Index *index : indexes_) {
    rc = index->delete_entry(old_record.data(), &old_record.rid());
    ASSERT(RC::SUCCESS == rc,
           "failed to delete entry from index. table name=%s, index name=%s, rid=%s, rc=%s",
           name(), index->index_meta().name(), old_record.rid().to_string().c_str(), strrc(rc));
  }

  rc = insert_entry_of_indexes(new_record.data(), new_record.rid());
  if (OB_FAIL(rc)) {
    RC recover_rc = insert_entry_of_indexes(old_record.data(), old_record.rid());
    if (OB_FAIL(recover_rc)) {
      LOG_PANIC("failed to restore index data when insert new index entries failed. table=%s rc=%s", name(), strrc(recover_rc));
      return recover_rc;
    }
    return rc;
  }

  rc = record_handler_->update_record(new_record.data(), &new_record.rid());
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to update record data while updating table=%s rc=%s", name(), strrc(rc));
    RC delete_rc = delete_entry_of_indexes(new_record.data(), new_record.rid(), false);
    if (OB_FAIL(delete_rc)) {
      LOG_PANIC("failed to rollback new index entries when record update failed. table=%s rc=%s", name(), strrc(delete_rc));
      return delete_rc;
    }
    RC recover_rc = insert_entry_of_indexes(old_record.data(), old_record.rid());
    if (OB_FAIL(recover_rc)) {
      LOG_PANIC("failed to restore old index entries after record update failure. table=%s rc=%s", name(), strrc(recover_rc));
      return recover_rc;
    }
  }
  return rc;
}

RC Table::insert_entry_of_indexes(const char *record, const RID &rid)
{
  RC rc = RC::SUCCESS;
  for (Index *index : indexes_) {
    rc = index->insert_entry(record, &rid);
    if (rc != RC::SUCCESS) {
      break;
    }
  }
  return rc;
}

RC Table::delete_entry_of_indexes(const char *record, const RID &rid, bool error_on_not_exists)
{
  RC rc = RC::SUCCESS;
  for (Index *index : indexes_) {
    rc = index->delete_entry(record, &rid);
    if (rc != RC::SUCCESS) {
      if (rc != RC::RECORD_INVALID_KEY || !error_on_not_exists) {
        break;
      }
    }
  }
  return rc;
}

Index *Table::find_index(const char *index_name) const
{
  for (Index *index : indexes_) {
    if (0 == strcmp(index->index_meta().name(), index_name)) {
      return index;
    }
  }
  return nullptr;
}

Index *Table::find_index_by_field(const char *field_name) const
{
  for (const auto &index : indexes_) {
    if (index->index_meta().fields().size() == 1) {
      auto name = index->index_meta().fields().front().name();
      if (0 == strcmp(name, field_name)) {
        return index;
      }
    }
  }
  return nullptr;
}

// 对向量距离类型，索引字段进行检查
Index *Table::find_vector_index(NormalFunctionType distance_fn, const char *field_name) const
{
  for (const auto &index : indexes_) {
    if (index->is_vector_index()) {
      auto vector_index = dynamic_cast<IvfflatIndex *>(index);
      if (vector_index->distance_fn() == distance_fn) {
        auto name = index->index_meta().fields().front().name();
        if (0 == strcmp(name, field_name)) {
          return index;
        }
      }
    }
  }
  return nullptr;
}

RC Table::sync()
{
  RC rc = RC::SUCCESS;
  for (Index *index : indexes_) {
    rc = index->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to flush index's pages. table=%s, index=%s, rc=%d:%s",
          name(),
          index->index_meta().name(),
          rc,
          strrc(rc));
      return rc;
    }
  }

  rc = data_buffer_pool_->flush_all_pages();
  LOG_INFO("Sync table over. table=%s", name());
  return rc;
}
