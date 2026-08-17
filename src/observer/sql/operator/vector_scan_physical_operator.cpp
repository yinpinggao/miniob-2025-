/***************************************************************
 *                                                             *
 * @Author      : Koschei                                      *
 * @Email       : nitianzero@gmail.com                         *
 * @Date        : 2024/10/18                                   *
 * @Description : VectorScanPhysicalOperator source file       *
 *                                                             *
 * Copyright (c) 2024 Koschei                                  *
 * All rights reserved.                                        *
 *                                                             *
 ***************************************************************/

#include "storage/trx/trx.h"
#include "vector_scan_physical_operator.h"

VectorScanPhysicalOperator::VectorScanPhysicalOperator(Table *table, std::string table_alias, Index *index,
    std::vector<float> base_vector, size_t limit, ReadWriteMode mode)
    : table_(table),
      index_(dynamic_cast<IvfflatIndex *>(index)),
      base_vector_(std::move(base_vector)),
      limit_(limit),
      mode_(mode)
{
  tuple_.set_table_alias(table_alias);
}

RC VectorScanPhysicalOperator::open(Trx *trx)
{
  // 【赛题 18 vector-search】有 IVF 索引时，先由 ann_search 返回 top-k 候选 RID，
  // 再回表、执行普通谓词并检查 MVCC；无索引路径则是全表计算距离后 ORDER BY/LIMIT。
  if (nullptr == table_ || nullptr == index_) {
    return RC::INTERNAL;
  }

  // 这里索引只先取 limit 个候选。若之后有记录被 WHERE/MVCC 过滤，当前实现不会
  // 继续向索引补取，因此可能少于 LIMIT，这是 ANN 与关系过滤组合的典型陷阱。
  rids_ = index_->ann_search(base_vector_, limit_);

  record_handler_ = table_->record_handler();
  if (nullptr == record_handler_) {
    LOG_WARN("invalid record handler");
    return RC::INTERNAL;
  }

  tuple_.set_schema(table_, table_->table_meta().field_metas());

  trx_ = trx;
  return RC::SUCCESS;
}

RC VectorScanPhysicalOperator::next()
{
  // 索引只负责候选生成；RID 回表后的记录仍必须通过事务可见性检查。
  RC rc = RC::SUCCESS;

  if (cnt_ >= rids_.size()) {
    return RC::RECORD_EOF;
  }

  bool filter_result = false;
  while (cnt_ < rids_.size()) {
    auto rid = rids_[cnt_++];

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

RC VectorScanPhysicalOperator::close() { return RC::SUCCESS; }

Tuple *VectorScanPhysicalOperator::current_tuple()
{
  tuple_.set_record(&current_record_);
  return &tuple_;
}

void VectorScanPhysicalOperator::set_predicates(std::vector<std::unique_ptr<Expression>> &&exprs)
{
  predicates_ = std::move(exprs);
}

RC VectorScanPhysicalOperator::filter(RowTuple &tuple, bool &result)
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

std::string VectorScanPhysicalOperator::param() const
{
  return std::string(index_->index_meta().name()) + " ON " + table_->name();
}
