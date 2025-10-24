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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

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

namespace fs = std::filesystem;

namespace {

RC build_index_meta_with_mapping(const TableMeta &new_meta,
    const IndexMeta &old_index_meta,
    IndexMeta &result,
    const std::unordered_map<std::string, std::string> &field_name_mapping)
{
  std::vector<FieldMeta> new_fields;
  new_fields.reserve(old_index_meta.fields().size());

  for (const FieldMeta &field : old_index_meta.fields()) {
    std::string lookup_name = field.name();
    auto        iter        = field_name_mapping.find(lookup_name);
    if (iter != field_name_mapping.end()) {
      lookup_name = iter->second;
    }

    const FieldMeta *new_field = new_meta.field(lookup_name.c_str());
    if (new_field == nullptr) {
      return RC::SCHEMA_FIELD_NOT_EXIST;
    }
    new_fields.push_back(*new_field);
  }

  return result.init(old_index_meta.name(), old_index_meta.index_type(), new_fields, old_index_meta.unique());
}

}  // namespace

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

RC Table::close_record_handler()
{
  if (record_handler_ != nullptr) {
    record_handler_->close();
    delete record_handler_;
    record_handler_ = nullptr;
  }

  if (data_buffer_pool_ != nullptr) {
    string data_file = table_data_file(base_dir_.c_str(), table_meta_.name());
    RC     rc        = db_->buffer_pool_manager().close_file(data_file.c_str());
    if (OB_FAIL(rc) && rc != RC::INTERNAL) {
      LOG_WARN("failed to close data file %s. rc=%s", data_file.c_str(), strrc(rc));
      return rc;
    }
    data_buffer_pool_ = nullptr;
  }

  return RC::SUCCESS;
}

RC Table::reload_record_handler()
{
  return init_record_handler(base_dir_.c_str());
}

RC Table::persist_table_meta(const TableMeta &meta, const std::string &table_name)
{
  string meta_file = table_meta_file(base_dir_.c_str(), table_name.c_str());
  string tmp_file  = meta_file + ".tmp";

  std::error_code ec;
  fs::remove(tmp_file, ec);

  std::fstream fs;
  fs.open(tmp_file, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
  if (!fs.is_open()) {
    LOG_ERROR("failed to open tmp meta file %s for write", tmp_file.c_str());
    return RC::IOERR_OPEN;
  }

  if (meta.serialize(fs) < 0) {
    LOG_ERROR("failed to serialize table meta to %s", tmp_file.c_str());
    fs.close();
    return RC::IOERR_WRITE;
  }
  fs.close();

  if (rename(tmp_file.c_str(), meta_file.c_str()) != 0) {
    LOG_ERROR("failed to rename meta tmp file %s to %s. err=%s", tmp_file.c_str(), meta_file.c_str(), strerror(errno));
    return RC::IOERR_WRITE;
  }

  return RC::SUCCESS;
}

RC Table::copy_record_to_new_layout(const TableMeta &src_meta,
    const Record &src_record,
    char *dest_buffer,
    const TableMeta &dest_meta,
    const std::unordered_map<std::string, std::string> &field_mapping)
{
  memset(dest_buffer, 0, dest_meta.record_size());

  for (int i = 0; i < dest_meta.field_num(); ++i) {
    const FieldMeta *dest_field = dest_meta.field(i);
    if (dest_field == nullptr) {
      continue;
    }

    std::string lookup_name;
    if (i < dest_meta.sys_field_num()) {
      lookup_name = dest_field->name();
    } else {
      auto iter = field_mapping.find(dest_field->name());
      if (iter != field_mapping.end()) {
        lookup_name = iter->second;
      } else {
        lookup_name = dest_field->name();
      }
    }

    const FieldMeta *src_field = nullptr;
    if (!lookup_name.empty()) {
      src_field = src_meta.field(lookup_name.c_str());
    }

    char *dest_ptr = dest_buffer + dest_field->offset();
    if (src_field != nullptr) {
      const char *src_ptr = src_record.data() + src_field->offset();
      int         copy_len = std::min(dest_field->len(), src_field->len());
      memcpy(dest_ptr, src_ptr, copy_len);
      if (dest_field->len() > src_field->len()) {
        memset(dest_ptr + src_field->len(), 0, dest_field->len() - src_field->len());
      }
    } else {
      memset(dest_ptr, 0, dest_field->len());
      if (dest_field->nullable()) {
        dest_ptr[dest_field->len() - 1] = '1';
      }
    }
  }

  return RC::SUCCESS;
}

RC Table::drop_all_indexes(bool remove_files)
{
  for (Index *index : indexes_) {
    if (index != nullptr) {
      index->close();
      delete index;
    }
  }
  indexes_.clear();

  if (remove_files) {
    for (int i = 0; i < table_meta_.index_num(); ++i) {
      const IndexMeta *index_meta = table_meta_.index(i);
      if (index_meta == nullptr) {
        continue;
      }
      string index_file = table_index_file(base_dir_.c_str(), table_meta_.name(), index_meta->name());
      std::error_code ec;
      fs::remove(index_file, ec);
      if (ec) {
        LOG_WARN("failed to remove index file %s: %s", index_file.c_str(), ec.message().c_str());
      }
    }
  }

  return RC::SUCCESS;
}

static std::unique_ptr<Index> create_index_instance(IndexType type)
{
  switch (type) {
    case IndexType::BPlusTreeIndex: return std::make_unique<BplusTreeIndex>();
    default: return nullptr;
  }
}

RC Table::rebuild_indexes(const std::vector<IndexMeta> &index_metas)
{
  indexes_.clear();

  for (const IndexMeta &index_meta : index_metas) {
    auto index = create_index_instance(index_meta.index_type());
    if (!index) {
      LOG_ERROR("unsupported index type when rebuilding index: %d", static_cast<int>(index_meta.index_type()));
      return RC::UNSUPPORTED;
    }

    string index_file = table_index_file(base_dir_.c_str(), table_meta_.name(), index_meta.name());
    std::error_code ec;
    fs::remove(index_file, ec);

    RC rc = index->create(this, index_file.c_str(), index_meta);
    if (OB_FAIL(rc)) {
      LOG_ERROR("failed to create index %s while rebuilding. rc=%s", index_meta.name(), strrc(rc));
      return rc;
    }

    RecordFileScanner scanner;
    rc = get_record_scanner(scanner, nullptr, ReadWriteMode::READ_ONLY);
    if (OB_FAIL(rc)) {
      index->close();
      return rc;
    }

    Record record;
    while ((rc = scanner.next(record)) == RC::SUCCESS) {
      rc = index->insert_entry(record.data(), &record.rid());
      if (OB_FAIL(rc)) {
        LOG_ERROR("failed to insert record into index %s while rebuilding. rc=%s", index_meta.name(), strrc(rc));
        scanner.close_scan();
        index->close();
        return rc;
      }
    }
    if (rc != RC::RECORD_EOF) {
      LOG_WARN("scanner aborted while rebuilding index %s. rc=%s", index_meta.name(), strrc(rc));
      scanner.close_scan();
      index->close();
      return rc;
    }
    scanner.close_scan();

    indexes_.push_back(index.release());
  }

  return RC::SUCCESS;
}

RC Table::reload_existing_indexes(const std::vector<IndexMeta> &index_metas)
{
  for (Index *index : indexes_) {
    if (index != nullptr) {
      index->close();
      delete index;
    }
  }
  indexes_.clear();

  for (const IndexMeta &index_meta : index_metas) {
    auto index = create_index_instance(index_meta.index_type());
    if (!index) {
      LOG_ERROR("unsupported index type when reloading index: %d", static_cast<int>(index_meta.index_type()));
      return RC::UNSUPPORTED;
    }

    string index_file = table_index_file(base_dir_.c_str(), table_meta_.name(), index_meta.name());
    RC     rc         = index->open(this, index_file.c_str(), index_meta);
    if (OB_FAIL(rc)) {
      LOG_ERROR("failed to reopen index %s. rc=%s", index_meta.name(), strrc(rc));
      return rc;
    }

    indexes_.push_back(index.release());
  }

  return RC::SUCCESS;
}

RC Table::rewrite_table_storage(TableMeta &new_meta,
    const std::unordered_map<std::string, std::string> &field_mapping,
    const std::vector<IndexMeta> &new_index_metas)
{
  string data_file     = table_data_file(base_dir_.c_str(), table_meta_.name());
  string tmp_data_file = data_file + ".tmp";
  string backup_file   = data_file + ".bak";

  BufferPoolManager &bpm = db_->buffer_pool_manager();
  std::error_code    ec;
  fs::remove(tmp_data_file, ec);

  RC rc = bpm.create_file(tmp_data_file.c_str());
  if (OB_FAIL(rc)) {
    LOG_ERROR("failed to create tmp data file %s. rc=%s", tmp_data_file.c_str(), strrc(rc));
    return rc;
  }

  DiskBufferPool *tmp_pool = nullptr;
  rc                       = bpm.open_file(db_->log_handler(), tmp_data_file.c_str(), tmp_pool);
  if (OB_FAIL(rc)) {
    LOG_ERROR("failed to open tmp data file %s. rc=%s", tmp_data_file.c_str(), strrc(rc));
    return rc;
  }

  TableMeta          temp_meta = new_meta;
  RecordFileHandler  tmp_handler(temp_meta.storage_format());
  rc = tmp_handler.init(*tmp_pool, db_->log_handler(), &temp_meta);
  if (OB_FAIL(rc)) {
    LOG_ERROR("failed to init tmp record handler. rc=%s", strrc(rc));
    bpm.close_file(tmp_data_file.c_str());
    return rc;
  }

  RecordFileScanner scanner;
  rc = get_record_scanner(scanner, nullptr, ReadWriteMode::READ_ONLY);
  if (OB_FAIL(rc)) {
    LOG_ERROR("failed to open scanner on old data. rc=%s", strrc(rc));
    tmp_handler.close();
    bpm.close_file(tmp_data_file.c_str());
    return rc;
  }

  std::vector<char> new_record(new_meta.record_size());
  Record             record;

  while ((rc = scanner.next(record)) == RC::SUCCESS) {
    rc = copy_record_to_new_layout(table_meta_, record, new_record.data(), new_meta, field_mapping);
    if (OB_FAIL(rc)) {
      LOG_ERROR("failed to transform record during rewrite. rc=%s", strrc(rc));
      scanner.close_scan();
      tmp_handler.close();
      bpm.close_file(tmp_data_file.c_str());
      return rc;
    }
    RID rid;
    rc = tmp_handler.insert_record(new_record.data(), new_meta.record_size(), &rid);
    if (OB_FAIL(rc)) {
      LOG_ERROR("failed to insert record to tmp file. rc=%s", strrc(rc));
      scanner.close_scan();
      tmp_handler.close();
      bpm.close_file(tmp_data_file.c_str());
      return rc;
    }
  }

  if (rc != RC::RECORD_EOF) {
    LOG_ERROR("unexpected rc while scanning old data: %s", strrc(rc));
    scanner.close_scan();
    tmp_handler.close();
    bpm.close_file(tmp_data_file.c_str());
    return rc;
  }
  scanner.close_scan();

  tmp_handler.close();
  tmp_pool->flush_all_pages();
  bpm.close_file(tmp_data_file.c_str());

  rc = drop_all_indexes(true);
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = close_record_handler();
  if (OB_FAIL(rc)) {
    return rc;
  }

  fs::remove(backup_file, ec);
  fs::rename(data_file, backup_file, ec);
  if (ec) {
    LOG_ERROR("failed to backup old data file %s: %s", data_file.c_str(), ec.message().c_str());
    return RC::IOERR_WRITE;
  }

  fs::rename(tmp_data_file, data_file, ec);
  if (ec) {
    LOG_ERROR("failed to promote tmp data file %s: %s", tmp_data_file.c_str(), ec.message().c_str());
    fs::rename(backup_file, data_file, ec);
    return RC::IOERR_WRITE;
  }

  fs::remove(backup_file, ec);

  new_meta.set_indexes(new_index_metas);
  table_meta_.swap(new_meta);

  rc = persist_table_meta(table_meta_, table_meta_.name());
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = reload_record_handler();
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = rebuild_indexes(new_index_metas);
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = persist_table_meta(table_meta_, table_meta_.name());
  if (OB_FAIL(rc)) {
    return rc;
  }

  return RC::SUCCESS;
}

RC Table::alter_add_column(const AttrInfoSqlNode &attr_info)
{
  if (table_meta_.field(attr_info.name.c_str()) != nullptr) {
    return RC::SCHEMA_FIELD_EXIST;
  }

  TableMeta new_meta(table_meta_);
  RC        rc = new_meta.append_field(attr_info);
  if (OB_FAIL(rc)) {
    return rc;
  }

  std::vector<IndexMeta> new_indexes;
  new_indexes.reserve(table_meta_.index_num());
  std::unordered_map<std::string, std::string> identity;

  for (int i = 0; i < table_meta_.index_num(); ++i) {
    const IndexMeta *old_index = table_meta_.index(i);
    IndexMeta        rebuilt_index;
    rc = build_index_meta_with_mapping(new_meta, *old_index, rebuilt_index, identity);
    if (OB_FAIL(rc)) {
      return rc;
    }
    new_indexes.push_back(std::move(rebuilt_index));
  }

  rc = rewrite_table_storage(new_meta, identity, new_indexes);
  return rc;
}

RC Table::alter_drop_column(const std::string &column_name)
{
  if (table_meta_.field(column_name.c_str()) == nullptr) {
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  for (int i = 0; i < table_meta_.sys_field_num(); ++i) {
    if (0 == strcmp(table_meta_.field(i)->name(), column_name.c_str())) {
      return RC::INVALID_ARGUMENT;
    }
  }

  TableMeta new_meta(table_meta_);
  RC        rc = new_meta.remove_field(column_name);
  if (OB_FAIL(rc)) {
    return rc;
  }

  if (new_meta.field_num() - new_meta.sys_field_num() <= 0) {
    return RC::INVALID_ARGUMENT;
  }

  std::vector<IndexMeta> new_indexes;
  std::unordered_map<std::string, std::string> identity;

  for (int i = 0; i < table_meta_.index_num(); ++i) {
    const IndexMeta *old_index = table_meta_.index(i);
    bool             contains  = false;
    for (const FieldMeta &idx_field : old_index->fields()) {
      if (column_name == idx_field.name()) {
        contains = true;
        break;
      }
    }
    if (contains) {
      continue;
    }

    IndexMeta rebuilt_index;
    rc = build_index_meta_with_mapping(new_meta, *old_index, rebuilt_index, identity);
    if (OB_FAIL(rc)) {
      return rc;
    }
    new_indexes.push_back(std::move(rebuilt_index));
  }

  rc = rewrite_table_storage(new_meta, identity, new_indexes);
  return rc;
}

RC Table::alter_change_column(const std::string &old_name, const std::string &new_name)
{
  if (old_name == new_name) {
    return RC::SUCCESS;
  }

  if (table_meta_.field(old_name.c_str()) == nullptr) {
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  if (table_meta_.field(new_name.c_str()) != nullptr) {
    return RC::SCHEMA_FIELD_EXIST;
  }

  for (int i = 0; i < table_meta_.sys_field_num(); ++i) {
    if (0 == strcmp(table_meta_.field(i)->name(), old_name.c_str())) {
      return RC::INVALID_ARGUMENT;
    }
  }

  TableMeta new_meta(table_meta_);
  RC        rc = new_meta.rename_field(old_name, new_name);
  if (OB_FAIL(rc)) {
    return rc;
  }

  std::unordered_map<std::string, std::string> rename_map;
  rename_map[old_name] = new_name;

  std::vector<IndexMeta> new_indexes;
  new_indexes.reserve(table_meta_.index_num());

  for (int i = 0; i < table_meta_.index_num(); ++i) {
    const IndexMeta *old_index = table_meta_.index(i);
    IndexMeta        rebuilt_index;
    rc = build_index_meta_with_mapping(new_meta, *old_index, rebuilt_index, rename_map);
    if (OB_FAIL(rc)) {
      return rc;
    }
    new_indexes.push_back(std::move(rebuilt_index));
  }

  new_meta.set_indexes(new_indexes);
  table_meta_.swap(new_meta);
  table_meta_.set_indexes(new_indexes);

  rc = persist_table_meta(table_meta_, table_meta_.name());
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = reload_existing_indexes(new_indexes);
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = persist_table_meta(table_meta_, table_meta_.name());
  return rc;
}

RC Table::alter_rename_table(const std::string &new_name)
{
  if (table_meta_.name() == new_name) {
    return RC::SUCCESS;
  }

  if (common::is_blank(new_name.c_str())) {
    return RC::INVALID_ARGUMENT;
  }

  std::vector<IndexMeta> index_metas;
  index_metas.reserve(table_meta_.index_num());
  for (int i = 0; i < table_meta_.index_num(); ++i) {
    index_metas.push_back(*table_meta_.index(i));
  }

  RC rc = drop_all_indexes(false);
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = close_record_handler();
  if (OB_FAIL(rc)) {
    return rc;
  }

  string old_name      = table_meta_.name();
  string old_meta_file = table_meta_file(base_dir_.c_str(), old_name.c_str());
  string new_meta_file = table_meta_file(base_dir_.c_str(), new_name.c_str());
  string old_data_file = table_data_file(base_dir_.c_str(), old_name.c_str());
  string new_data_file = table_data_file(base_dir_.c_str(), new_name.c_str());

  std::error_code ec;
  fs::rename(old_data_file, new_data_file, ec);
  if (ec) {
    LOG_ERROR("failed to rename data file to %s: %s", new_data_file.c_str(), ec.message().c_str());
    return RC::IOERR_WRITE;
  }

  fs::rename(old_meta_file, new_meta_file, ec);
  if (ec) {
    LOG_ERROR("failed to rename meta file to %s: %s", new_meta_file.c_str(), ec.message().c_str());
    fs::rename(new_data_file, old_data_file, ec);
    return RC::IOERR_WRITE;
  }

  std::vector<std::pair<std::string, std::string>> renamed_index_files;
  renamed_index_files.reserve(index_metas.size());

  for (const IndexMeta &index_meta : index_metas) {
    string old_index = table_index_file(base_dir_.c_str(), old_name.c_str(), index_meta.name());
    string new_index = table_index_file(base_dir_.c_str(), new_name.c_str(), index_meta.name());
    fs::rename(old_index, new_index, ec);
    if (ec) {
      LOG_ERROR("failed to rename index file %s: %s", old_index.c_str(), ec.message().c_str());
      for (const auto &entry : renamed_index_files) {
        std::error_code revert_ec;
        fs::rename(entry.second, entry.first, revert_ec);
      }
      fs::rename(new_meta_file, old_meta_file, ec);
      fs::rename(new_data_file, old_data_file, ec);
      return RC::IOERR_WRITE;
    }
    renamed_index_files.emplace_back(std::move(old_index), std::move(new_index));
  }

  table_meta_.set_name(new_name);
  table_meta_.set_indexes(index_metas);

  rc = persist_table_meta(table_meta_, new_name);
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = reload_record_handler();
  if (OB_FAIL(rc)) {
    return rc;
  }

  rc = reload_existing_indexes(index_metas);
  if (OB_FAIL(rc)) {
    return rc;
  }

  return RC::SUCCESS;
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

  // 尝试插入
  rc = insert_entry_of_indexes(new_record.data(), new_record.rid());
  // 出现重复键
  if (rc != RC::SUCCESS) {
    // 因为有些索引还没有插入，删除失败不应该报错
    RC delete_entry_of_indexes_rc = delete_entry_of_indexes(new_record.data(), new_record.rid(), false);
    if (RC::SUCCESS != delete_entry_of_indexes_rc) {
      LOG_WARN("failed to rollback index data when insert index entries failed. table name=%s, rc=%s", name(), strrc(delete_entry_of_indexes_rc));
      return delete_entry_of_indexes_rc;
    }
    return rc;
  }

  // 最后更新记录
  rc = record_handler_->update_record(new_record.data(), &new_record.rid());
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
