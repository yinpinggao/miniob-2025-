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
// Created by Wangyunlai on 2022/07/08.
//

#include "sql/operator/index_scan_physical_operator.h"
#include "storage/index/index.h"
#include "storage/trx/trx.h"
#include <algorithm>

IndexScanPhysicalOperator::IndexScanPhysicalOperator(Table *table, Index *index, ReadWriteMode mode,
    const Value *left_value, bool left_inclusive, const Value *right_value, bool right_inclusive)
    : table_(table), index_(index), mode_(mode), left_inclusive_(left_inclusive), right_inclusive_(right_inclusive)
{
  if (left_value) {
      //new
    has_left_value_ = true;
    left_value_ = *left_value;
  
  }
  if (right_value) {
    //new
     has_right_value_ = true;
    right_value_ = *right_value;
  }
}

IndexScanPhysicalOperator::IndexScanPhysicalOperator(Table *table, std::string table_alias, Index *index,
    ReadWriteMode mode, const Value *left_value, bool left_inclusive, const Value *right_value, bool right_inclusive)
    : table_(table), index_(index), mode_(mode), left_inclusive_(left_inclusive), right_inclusive_(right_inclusive)
{
  tuple_.set_table_alias(table_alias);
  if (left_value) {
    //new
     has_left_value_ = true;
    left_value_ = *left_value;
  }
  if (right_value) {
    //new
    has_right_value_ = true;
    right_value_ = *right_value;
  }
}
//new
RC IndexScanPhysicalOperator::build_search_key(const Value &value, std::string &buffer)
{
  if (index_ == nullptr || table_ == nullptr) {
    return RC::INTERNAL;
  }

  const auto &fields = index_->index_meta().fields();
  if (fields.size() != 1) {
    LOG_WARN("index scan currently supports single column indexes only. index=%s", index_->index_meta().name());
    return RC::UNSUPPORTED;
  }

  const FieldMeta &field = fields[0];
  std::string      record_storage(table_->table_meta().record_size(), 0);
  Value            typed_value = value;

  if (!typed_value.is_null() && typed_value.attr_type() != field.type()) {
    RC rc = Value::cast_to(value, field.type(), typed_value, false);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to cast boundary value to field type. field=%s rc=%s", field.name(), strrc(rc));
      return rc;
    }
  }

  if (typed_value.is_null()) {
    if (!field.nullable()) {
      LOG_WARN("field %s is not nullable but got null boundary", field.name());
      return RC::NOT_NULLABLE_VALUE;
    }
    record_storage[field.offset() + field.len() - 1] = '1';
  } else {
    RC rc = table_->set_value_to_record(record_storage.data(), typed_value, &field);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to set value to record buffer. field=%s rc=%s", field.name(), strrc(rc));
      return rc;
    }
    if (field.nullable()) {
      record_storage[field.offset() + field.len() - 1] = 0;
    }
  }

  buffer.assign(record_storage.data() + field.offset(), record_storage.data() + field.offset() + field.len());
  return RC::SUCCESS;
}


RC IndexScanPhysicalOperator::open(Trx *trx)
{
  if (nullptr == table_ || nullptr == index_) {
    return RC::INTERNAL;
  }

  const char *left_key  = nullptr;
  int         left_len  = 0;
  const char *right_key = nullptr;
  int         right_len = 0;
  RC          rc        = RC::SUCCESS;

  if (has_left_value_) {
    rc = build_search_key(left_value_, left_search_key_);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to build left search key. rc=%s", strrc(rc));
      return rc;
    }
    left_key = left_search_key_.data();
    left_len = static_cast<int>(left_search_key_.size());
  } else {
    left_search_key_.clear();
  }

  if (has_right_value_) {
    rc = build_search_key(right_value_, right_search_key_);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to build right search key. rc=%s", strrc(rc));
      return rc;
    }
    right_key = right_search_key_.data();
    right_len = static_cast<int>(right_search_key_.size());
  } else {
    right_search_key_.clear();
  }

  IndexScanner *index_scanner = index_->create_scanner(
      left_key, left_len, left_inclusive_, right_key, right_len, right_inclusive_);
  if (nullptr == index_scanner) {
    LOG_WARN("failed to create index scanner");
    return RC::INTERNAL;
  }

  record_handler_ = table_->record_handler();
  if (nullptr == record_handler_) {
    LOG_WARN("invalid record handler");
    index_scanner->destroy();
    return RC::INTERNAL;
  }
  index_scanner_ = index_scanner;

  tuple_.set_schema(table_, table_->table_meta().field_metas());

  trx_ = trx;
  return RC::SUCCESS;
}

RC IndexScanPhysicalOperator::next()
{
  RID rid;
  RC  rc = RC::SUCCESS;

  bool filter_result = false;
  while (RC::SUCCESS == (rc = index_scanner_->next_entry(&rid))) {
    rc = record_handler_->get_record(rid, current_record_);
    if (OB_FAIL(rc)) {
      LOG_TRACE("failed to get record. rid=%s, rc=%s", rid.to_string().c_str(), strrc(rc));
      return rc;
    }

    LOG_TRACE("got a record. rid=%s", rid.to_string().c_str());

    tuple_.set_record(&current_record_);
    rc = filter(tuple_, filter_result);
    if (OB_FAIL(rc)) {
      LOG_TRACE("failed to filter record. rc=%s", strrc(rc));
      return rc;
    }

    if (!filter_result) {
      LOG_TRACE("record filtered");
      continue;
    }

    rc = trx_->visit_record(table_, current_record_, mode_);
    if (rc == RC::RECORD_INVISIBLE) {
      LOG_TRACE("record invisible");
      continue;
    } else {
      return rc;
    }
  }

  return rc;
}

RC IndexScanPhysicalOperator::close()
{
  index_scanner_->destroy();
  index_scanner_ = nullptr;
  return RC::SUCCESS;
}

Tuple *IndexScanPhysicalOperator::current_tuple()
{
  tuple_.set_record(&current_record_);
  return &tuple_;
}

void IndexScanPhysicalOperator::set_predicates(std::vector<std::unique_ptr<Expression>> &&exprs)
{
  predicates_ = std::move(exprs);
}

RC IndexScanPhysicalOperator::filter(RowTuple &tuple, bool &result)
{
  RC    rc = RC::SUCCESS;
  Value value;
  for (std::unique_ptr<Expression> &expr : predicates_) {
    rc = expr->get_value(tuple, value);
    if (rc != RC::SUCCESS) {
      return rc;
    }

    bool tmp_result = value.get_boolean();
    if (!tmp_result) {
      result = false;
      return rc;
    }
  }

  result = true;
  return rc;
}

std::string IndexScanPhysicalOperator::param() const
{
  return std::string(index_->index_meta().name()) + " ON " + table_->name();
}
