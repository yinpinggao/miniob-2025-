/***************************************************************
 *                                                             *
 * @Author      : Koschei                                      *
 * @Email       : nitianzero@gmail.com                         *
 * @Date        : 2024/9/17                                    *
 * @Description : UpdatePhysicalOperator source file           *
 *                                                             *
 * Copyright (c) 2024 Koschei                                  *
 * All rights reserved.                                        *
 *                                                             *
 ***************************************************************/

#include "update_physical_operator.h"
#include "storage/trx/trx.h"

RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  records_.clear();
  log_records.clear();

  std::unique_ptr<PhysicalOperator> &child = children_[0];
  RC                                 rc    = child->open(trx);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  trx_ = trx;

  while (true) {
    rc = child->next();
    if (rc == RC::RECORD_EOF) {
      rc = RC::SUCCESS;
      break;
    }
    if (OB_FAIL(rc)) {
      child->close();
      LOG_WARN("failed to iterate child operator: %s", strrc(rc));
      return rc;
    }

    Tuple *tuple = child->current_tuple();
    if (tuple == nullptr) {
      child->close();
      LOG_WARN("failed to get current tuple from child");
      return RC::INTERNAL;
    }

    auto  *row_tuple   = static_cast<RowTuple *>(tuple);
    Record record_copy = row_tuple->record().clone();
    record_copy.set_base_rids(tuple->base_rids());
    records_.emplace_back(std::move(record_copy));
  }

  child->close();

  if (records_.empty()) {
    return RC::SUCCESS;
  }

  RowTuple    tuple;
  const auto *table_fields = table_->table_meta().field_metas();
  tuple.set_schema(table_, table_fields);

  std::vector<Value> evaluated_values(values_.size());
  for (size_t idx = 0; idx < values_.size(); ++idx) {
    const FieldMeta &field_meta = field_metas_[idx];
    Expression      &value_expr = *values_[idx];
    Value            value;

    tuple.set_record(&records_.front());

    SubQueryExpr *sub_query_expr = nullptr;
    if (value_expr.type() == ExprType::SUBQUERY) {
      sub_query_expr = static_cast<SubQueryExpr *>(values_[idx].get());
      rc             = sub_query_expr->open(trx_, tuple);
      if (OB_FAIL(rc)) {
        LOG_ERROR("failed to open subquery for field %s: %s", field_meta.name(), strrc(rc));
        return rc;
      }
    }

    rc = value_expr.get_value(tuple, value);
    if (rc == RC::RECORD_EOF) {
      value.set_null();
      rc = RC::SUCCESS;
    }
    if (OB_FAIL(rc)) {
      LOG_ERROR("failed to evaluate assignment for field %s: %s", field_meta.name(), strrc(rc));
      if (sub_query_expr != nullptr) {
        sub_query_expr->close();
      }
      return rc;
    }

    if (sub_query_expr != nullptr) {
      bool has_more = sub_query_expr->has_more_row(tuple);
      sub_query_expr->close();
      if (has_more) {
        LOG_ERROR("subquery returned more than one row for field %s", field_meta.name());
        return RC::SUBQUERY_RETURNED_MULTIPLE_ROWS;
      }
    }

    if (!value.is_null() && value.attr_type() != field_meta.type()) {
      Value cast_value;
      rc = Value::cast_to(value, field_meta.type(), cast_value, false);
      if (OB_FAIL(rc)) {
        LOG_ERROR("type mismatch for field %s. expect %s but got %s",
            field_meta.name(),
            attr_type_to_string(field_meta.type()),
            attr_type_to_string(value.attr_type()));
        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
      }
      value = std::move(cast_value);
    }

    if (!value.is_null()) {
      int32_t max_len = field_meta.len() - field_meta.nullable();
      if (value.length() > max_len) {
        LOG_ERROR("value too long for field %s. length=%d, max_len=%d",
            field_meta.name(),
            value.length(),
            max_len);
        return RC::VALUE_TOO_LONG;
      }
    }

    evaluated_values[idx] = std::move(value);
  }

  for (Record &old_record : records_) {
    tuple.set_record(&old_record);

    Record new_record;
    rc = new_record.copy_data(old_record.data(), old_record.len());
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to copy record data while updating: %s", strrc(rc));
      return rc;
    }
    new_record.set_rid(old_record.rid());

    for (size_t idx = 0; idx < field_metas_.size(); ++idx) {
      const FieldMeta &field_meta = field_metas_[idx];
      const Value     &assigned   = evaluated_values[idx];

      if (field_meta.nullable()) {
        int null_offset                = field_meta.offset() + field_meta.len() - 1;
        new_record.data()[null_offset] = assigned.is_null() ? '1' : 0;
      } else if (assigned.is_null()) {
        rollback();
        return RC::NOT_NULLABLE_VALUE;
      }

      if (!assigned.is_null()) {
        rc = new_record.set_field(field_meta.offset(), field_meta.len(), assigned);
        if (OB_FAIL(rc)) {
          LOG_ERROR("failed to set field %s: %s", field_meta.name(), strrc(rc));
          return rc;
        }
      }
    }

    Record rollback_old = old_record.clone();
    Record rollback_new = new_record.clone();

    rc = trx_->update_record(table_, old_record, new_record);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to update record: %s", strrc(rc));
      rollback();
      return rc;
    }

    log_records.emplace_back(std::move(rollback_old), std::move(rollback_new));
  }

  return RC::SUCCESS;
}

void UpdatePhysicalOperator::rollback()
{
  if (trx_ == nullptr) {
    return;
  }

  RC rc = trx_->rollback();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to rollback trx in update operator. rc=%s", strrc(rc));
    return;
  }

  rc = trx_->start_if_need();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to restart trx in update operator after rollback. rc=%s", strrc(rc));
  }
}

RC UpdatePhysicalOperator::next() { return RC::RECORD_EOF; }

RC UpdatePhysicalOperator::close() { return RC::SUCCESS; }
