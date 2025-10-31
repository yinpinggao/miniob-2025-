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
// Created by WangYunlai on 2021/6/9.
//

#include "sql/operator/table_scan_physical_operator.h"
#include "event/sql_debug.h"
#include "storage/table/table.h"

using namespace std;

RC TableScanPhysicalOperator::open(Trx *trx)
{
  RC rc = table_->get_record_scanner(record_scanner_, trx, mode_);
  if (rc == RC::SUCCESS) {
    tuple_.set_schema(table_, table_->table_meta().field_metas());
  }
  trx_ = trx;
  return rc;
}

RC TableScanPhysicalOperator::next()
{
  RC rc = RC::SUCCESS;

  bool filter_result = false;
  while (OB_SUCC(rc = record_scanner_.next(current_record_))) {
    LOG_TRACE("got a record. rid=%s", current_record_.rid().to_string().c_str());

    // 存储记录来自哪个表和 rid
    tuple_.reset();
    tuple_.append_base_rids(table_, current_record_.rid());
    tuple_.set_record(&current_record_);
    rc = filter(tuple_, filter_result);
    if (rc != RC::SUCCESS) {
      LOG_TRACE("record filtered failed=%s", strrc(rc));
      return rc;
    }

    if (filter_result) {
      sql_debug("get a tuple: %s", tuple_.to_string().c_str());
      break;
    } else {
      sql_debug("a tuple is filtered: %s", tuple_.to_string().c_str());
    }
  }
  return rc;
}

RC TableScanPhysicalOperator::close() { return record_scanner_.close_scan(); }

Tuple *TableScanPhysicalOperator::current_tuple()
{
  tuple_.set_record(&current_record_);
  return &tuple_;
}

RC TableScanPhysicalOperator::tuple_schema(TupleSchema &schema) const
{
  schema = TupleSchema();
  if (table_ == nullptr) {
    return RC::INTERNAL;
  }

  const TableMeta &table_meta = table_->table_meta();
  const int        sys_fields = table_meta.sys_field_num();
  const int        field_num  = table_meta.field_num();

  for (int i = sys_fields; i < field_num; i++) {
    const FieldMeta *field_meta = table_meta.field(i);
    if (field_meta == nullptr) {
      continue;
    }
    if (!field_meta->visible()) {
      continue;
    }
    const char *alias = table_alias_.empty() ? nullptr : table_alias_.c_str();
    schema.append_cell(TupleCellSpec(table_->name(), field_meta->name(), alias));
  }

  if (schema.cell_num() == 0) {
    return RC::UNIMPLEMENTED;
  }
  return RC::SUCCESS;
}

string TableScanPhysicalOperator::param() const { return table_->name(); }

void TableScanPhysicalOperator::set_predicates(vector<unique_ptr<Expression>> &&exprs)
{
  predicates_ = std::move(exprs);
}

RC TableScanPhysicalOperator::filter(RowTuple &tuple, bool &result)
{
  RC          rc = RC::SUCCESS;
  Value       value;
  Tuple      *tp = &tuple;
  JoinedTuple jt;
  jt.set_left(&tuple);
  jt.set_right(const_cast<Tuple *>(parent_tuple_));
  if (parent_tuple_) {
    tp = &jt;
  }
  for (unique_ptr<Expression> &expr : predicates_) {
    rc = expr->get_value(*tp, value);
    if (rc != RC::SUCCESS) {
      close();
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
