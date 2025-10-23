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
// Created by Wangyunlai on 2023/04/24.
//

#include "storage/trx/mvcc_trx.h"

#include <ranges>
#include "storage/db/db.h"
#include "storage/field/field.h"
#include "storage/trx/mvcc_trx_log.h"
#include "common/lang/algorithm.h"

MvccTrxKit::~MvccTrxKit()
{
  vector<Trx *> tmp_trxes;
  tmp_trxes.swap(trxes_);

  for (Trx *trx : tmp_trxes) {
    delete trx;
  }
}

RC MvccTrxKit::init()
{
  // 事务使用一些特殊的字段，放到每行记录中，表示行记录的可见性。
  fields_ = vector<FieldMeta>{// field_id in trx fields is invisible.
      FieldMeta("__trx_xid_begin",
          AttrType::INTS,
          0 /*attr_offset*/,
          4 /*attr_len*/,
          false /*visible*/,
          -1 /*field_id*/,
          false),
      FieldMeta("__trx_xid_end",
          AttrType::INTS,
          0 /*attr_offset*/,
          4 /*attr_len*/,
          false /*visible*/,
          -2 /*field_id*/,
          false)};

  LOG_INFO("init mvcc trx kit done.");
  return RC::SUCCESS;
}

const vector<FieldMeta> *MvccTrxKit::trx_fields() const { return &fields_; }

int32_t MvccTrxKit::next_trx_id() { return ++current_trx_id_; }

int32_t MvccTrxKit::max_trx_id() const { return numeric_limits<int32_t>::max(); }

Trx *MvccTrxKit::create_trx(LogHandler &log_handler)
{
  Trx *trx = new MvccTrx(*this, log_handler);
  if (trx != nullptr) {
    lock_.lock();
    trxes_.push_back(trx);
    lock_.unlock();
  }
  return trx;
}

Trx *MvccTrxKit::create_trx(LogHandler &log_handler, int32_t trx_id)
{
  Trx *trx = new MvccTrx(*this, log_handler, trx_id);
  if (trx != nullptr) {
    lock_.lock();
    trxes_.push_back(trx);
    if (current_trx_id_ < trx_id) {
      current_trx_id_ = trx_id;
    }
    lock_.unlock();
  }
  return trx;
}

void MvccTrxKit::destroy_trx(Trx *trx)
{
  lock_.lock();
  for (auto iter = trxes_.begin(), itend = trxes_.end(); iter != itend; ++iter) {
    if (*iter == trx) {
      trxes_.erase(iter);
      break;
    }
  }
  lock_.unlock();

  delete trx;
}

Trx *MvccTrxKit::find_trx(int32_t trx_id)
{
  lock_.lock();
  for (Trx *trx : trxes_) {
    if (trx->id() == trx_id) {
      lock_.unlock();
      return trx;
    }
  }
  lock_.unlock();

  return nullptr;
}

void MvccTrxKit::all_trxes(vector<Trx *> &trxes)
{
  lock_.lock();
  trxes = trxes_;
  lock_.unlock();
}

LogReplayer *MvccTrxKit::create_log_replayer(Db &db, LogHandler &log_handler)
{
  return new MvccTrxLogReplayer(db, *this, log_handler);
}

////////////////////////////////////////////////////////////////////////////////

MvccTrx::MvccTrx(MvccTrxKit &kit, LogHandler &log_handler) : trx_kit_(kit), log_handler_(log_handler) {}

MvccTrx::MvccTrx(MvccTrxKit &kit, LogHandler &log_handler, int32_t trx_id)
    : trx_kit_(kit), log_handler_(log_handler), trx_id_(trx_id)
{
  started_    = true;
  recovering_ = true;
}

MvccTrx::~MvccTrx() {}

RC MvccTrx::insert_record(BaseTable *table, Record &record)
{
  Field begin_field;
  Field end_field;
  trx_fields(table, begin_field, end_field);

  begin_field.set_int(record, -trx_id_);
  end_field.set_int(record, trx_kit_.max_trx_id());

  RC rc = table->insert_record(record);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to insert record into table. rc=%s", strrc(rc));
    return rc;
  }

  rc = log_handler_.insert_record(trx_id_, table, record.rid());
  ASSERT(rc == RC::SUCCESS, "failed to append insert record log. trx id=%d, table id=%d, rid=%s, record len=%d, rc=%s",
         trx_id_, table->table_id(), record.rid().to_string().c_str(), record.len(), strrc(rc));

  operations_.emplace_back(Operation::Type::INSERT, table, record.rid());
  return rc;
}

RC MvccTrx::delete_record(BaseTable *table, Record &record)
{
  Field begin_field;
  Field end_field;
  trx_fields(table, begin_field, end_field);

  RC delete_result = RC::SUCCESS;

  RC rc = table->visit_record(record.rid(), [this, table, &delete_result, &end_field](Record &inplace_record) -> bool {
    RC rc = this->visit_record(table, inplace_record, ReadWriteMode::READ_WRITE);
    if (OB_FAIL(rc)) {
      delete_result = rc;
      return false;
    }

    end_field.set_int(inplace_record, -trx_id_);
    return true;
  });

  if (OB_FAIL(rc)) {
    LOG_WARN("failed to visit record. rc=%s", strrc(rc));
    return rc;
  }

  if (OB_FAIL(delete_result)) {
    LOG_TRACE("record is not visible. rid=%s, rc=%s", record.rid().to_string().c_str(), strrc(delete_result));
    return delete_result;
  }

  rc = log_handler_.delete_record(trx_id_, table, record.rid());
  ASSERT(rc == RC::SUCCESS, "failed to append delete record log. trx id=%d, table id=%d, rid=%s, record len=%d, rc=%s",
      trx_id_, table->table_id(), record.rid().to_string().c_str(), record.len(), strrc(rc));

  operations_.emplace_back(Operation::Type::DELETE, table, record.rid());

  return RC::SUCCESS;
}

RC MvccTrx::update_record(
    BaseTable *table, Record &old_record, Record &new_record)  // 涉及到update需要执行,提交前，还需要检测是否可见
{
  Field begin_field;
  Field end_field;
  trx_fields(table, begin_field, end_field);

  RC      visit_result     = RC::SUCCESS;
  int32_t original_end_xid = trx_kit_.max_trx_id();
  Record  old_version;
  Record  new_version;

  RC rc = table->visit_record(old_record.rid(),
      [this, table, &visit_result, &old_version, &end_field, &original_end_xid](Record &inplace_record) -> bool {
        RC visible_rc = this->visit_record(table, inplace_record, ReadWriteMode::READ_WRITE);
        if (OB_FAIL(visible_rc)) {
          visit_result = visible_rc;
          return false;
        }

        old_version      = inplace_record.clone();
        original_end_xid = end_field.get_int(old_version);
        end_field.set_int(inplace_record, -trx_id_);
        visit_result = RC::SUCCESS;
        return true;
      });

  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch record for update. table=%s, rid=%s, rc=%s",
        table->name(), old_record.rid().to_string().c_str(), strrc(rc));
    return rc;
  }
  if (OB_FAIL(visit_result)) {
    LOG_TRACE("record not visible for update. table=%s, rid=%s, rc=%s",
        table->name(), old_record.rid().to_string().c_str(), strrc(visit_result));
    return visit_result;
  }

  begin_field.set_int(new_record, -trx_id_);
  end_field.set_int(new_record, trx_kit_.max_trx_id());

  rc = table->insert_record(new_record);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to insert new version while update. table=%s, rid=%s, rc=%s",
        table->name(), old_record.rid().to_string().c_str(), strrc(rc));

    RC revert_rc =
        table->visit_record(old_record.rid(), [&end_field, original_end_xid](Record &inplace_record) -> bool {
          end_field.set_int(inplace_record, original_end_xid);
          return true;
        });
    if (OB_FAIL(revert_rc)) {
      LOG_ERROR("failed to revert old version after update failure. table=%s, rid=%s, rc=%s",
          table->name(), old_record.rid().to_string().c_str(), strrc(revert_rc));
      return revert_rc;
    }
    return rc;
  }

  new_version = new_record.clone();
  operations_.emplace_back(Operation::Type::UPDATE, table, old_version.rid(), old_version, new_version);
  return RC::SUCCESS;
}

RC MvccTrx::visit_record(BaseTable *table, Record &record, ReadWriteMode mode)
{
  Field begin_field;
  Field end_field;
  trx_fields(table, begin_field, end_field);

  int32_t begin_xid = begin_field.get_int(record);
  int32_t end_xid   = end_field.get_int(record);

  RC rc = RC::SUCCESS;
  if (begin_xid > 0 && end_xid > 0) {
    if (trx_id_ >= begin_xid && trx_id_ <= end_xid) {
      rc = RC::SUCCESS;
    } else {
      LOG_TRACE("record invisible. trx id=%d, begin xid=%d, end xid=%d", trx_id_, begin_xid, end_xid);
      rc = RC::RECORD_INVISIBLE;
    }
  } else if (begin_xid < 0) {
    // begin xid 小于0说明是刚插入而且没有提交的数据
    if (-begin_xid == trx_id_) {
      rc = RC::SUCCESS;
    } else {
      LOG_TRACE("record invisible. someone is updating this record right now. trx id=%d, begin xid=%d, end xid=%d",
                trx_id_, begin_xid, end_xid);
      rc = RC::RECORD_INVISIBLE;
    }
  } else if (end_xid < 0) {
    // end xid 小于0 说明是正在删除但是还没有提交的数据
    if (mode == ReadWriteMode::READ_ONLY) {
      // 如果 -end_xid 就是当前事务的事务号，说明是当前事务删除的
      if (-end_xid != trx_id_) {
        rc = RC::SUCCESS;
      } else {
        LOG_TRACE("record invisible. self has deleted this record. trx id=%d, begin xid=%d, end xid=%d",
                  trx_id_, begin_xid, end_xid);
        rc = RC::RECORD_INVISIBLE;
      }
    } else {
      // 如果当前想要修改此条数据，并且不是当前事务删除的，简单的报错
      // 这是事务并发处理的一种方式，非常简单粗暴。其它的并发处理方法，可以等待，或者让客户端重试
      // 或者等事务结束后，再检测修改的数据是否有冲突
      if (-end_xid != trx_id_) {
        LOG_TRACE("concurrency conflit. someone is deleting this record right now. trx id=%d, begin xid=%d, end xid=%d",
                  trx_id_, begin_xid, end_xid);
        rc = RC::LOCKED_CONCURRENCY_CONFLICT;
      } else {
        LOG_TRACE("record invisible. self has deleted this record. trx id=%d, begin xid=%d, end xid=%d",
                  trx_id_, begin_xid, end_xid);
        rc = RC::RECORD_INVISIBLE;
      }
    }
  }
  return rc;
}

/**
 * @brief 获取指定表上的事务使用的字段
 *
 * @param table 指定的表
 * @param begin_xid_field 返回处理begin_xid的字段
 * @param end_xid_field   返回处理end_xid的字段
 */
void MvccTrx::trx_fields(BaseTable *table, Field &begin_xid_field, Field &end_xid_field) const
{
  const TableMeta      &table_meta = table->table_meta();
  span<const FieldMeta> trx_fields = table_meta.trx_fields();
  ASSERT(trx_fields.size() >= 2, "invalid trx fields number. %d", trx_fields.size());

  begin_xid_field.set_table(table);
  begin_xid_field.set_field(&trx_fields[0]);
  end_xid_field.set_table(table);
  end_xid_field.set_field(&trx_fields[1]);
}

RC MvccTrx::start_if_need()
{
  if (!started_) {
    ASSERT(operations_.empty(), "try to start a new trx while operations is not empty");
    trx_id_ = trx_kit_.next_trx_id();
    LOG_DEBUG("current thread change to new trx with %d", trx_id_);
    started_ = true;
  }
  return RC::SUCCESS;
}

RC MvccTrx::commit()
{
  int32_t commit_id = trx_kit_.next_trx_id();
  return commit_with_trx_id(commit_id);
}
// 在事务提交阶段，update操作，将就记录标记为失效，将新纪录标记为生效
RC MvccTrx::commit_with_trx_id(int32_t commit_xid)
{
  RC rc    = RC::SUCCESS;
  started_ = false;

  for (const Operation &operation : operations_) {
    BaseTable *table = operation.table();
    switch (operation.type()) {
      case Operation::Type::INSERT: {
        const RID &rid = operation.rid();

        Field begin_xid_field;
        Field end_xid_field;
        trx_fields(table, begin_xid_field, end_xid_field);

        auto record_updater = [this, &begin_xid_field, commit_xid](Record &record) -> bool {
          LOG_DEBUG("before commit insert record. trx id=%d, begin xid=%d, commit xid=%d, lbt=%s",
              trx_id_, begin_xid_field.get_int(record), commit_xid, lbt());
          ASSERT(begin_xid_field.get_int(record) == -trx_id_ && !recovering_,
              "got an invalid record while committing. begin xid=%d, trx id=%d",
              begin_xid_field.get_int(record), trx_id_);

          begin_xid_field.set_int(record, commit_xid);
          return true;
        };

        rc = table->visit_record(rid, record_updater);
        ASSERT(rc == RC::SUCCESS,
            "failed to update insert record while committing. rid=%s, rc=%s",
            rid.to_string().c_str(), strrc(rc));
        break;
      }

      case Operation::Type::DELETE: {
        const RID &rid = operation.rid();

        Field begin_xid_field;
        Field end_xid_field;
        trx_fields(table, begin_xid_field, end_xid_field);

        auto record_updater = [this, &end_xid_field, commit_xid](Record &record) -> bool {
          ASSERT(end_xid_field.get_int(record) == -trx_id_,
              "got an invalid record while committing. end xid=%d, trx id=%d",
              end_xid_field.get_int(record), trx_id_);

          end_xid_field.set_int(record, commit_xid);
          return true;
        };

        rc = table->visit_record(rid, record_updater);
        ASSERT(rc == RC::SUCCESS,
            "failed to update delete record while committing. rid=%s, rc=%s",
            rid.to_string().c_str(), strrc(rc));
        break;
      }

      case Operation::Type::UPDATE: {
        const Record &old_version = operation.old_record();
        const Record &new_version = operation.updated_record();
        const RID    &old_rid     = old_version.rid();
        const RID    &new_rid     = new_version.rid();

        Field begin_field;
        Field end_field;
        trx_fields(table, begin_field, end_field);

        auto old_updater = [this, &end_field, commit_xid](Record &record) -> bool {
          ASSERT(end_field.get_int(record) == -trx_id_,
              "invalid old version while committing. end xid=%d, trx id=%d",
              end_field.get_int(record), trx_id_);
          end_field.set_int(record, commit_xid);
          return true;
        };

        auto new_updater = [this, &begin_field, commit_xid](Record &record) -> bool {
          ASSERT(begin_field.get_int(record) == -trx_id_,
              "invalid new version while committing. begin xid=%d, trx id=%d",
              begin_field.get_int(record), trx_id_);
          begin_field.set_int(record, commit_xid);
          return true;
        };

        rc = table->visit_record(old_rid, old_updater);
        ASSERT(rc == RC::SUCCESS,
            "failed to update old version while committing. rid=%s, rc=%s",
            old_rid.to_string().c_str(), strrc(rc));

        rc = table->visit_record(new_rid, new_updater);
        ASSERT(rc == RC::SUCCESS,
            "failed to update new version while committing. rid=%s, rc=%s",
            new_rid.to_string().c_str(), strrc(rc));
        break;
      }

      default: {
        LOG_PANIC("unsupported operation type while committing. type=%d",
            static_cast<int>(operation.type()));
        break;
      }
    }
  }

  if (!recovering_) {
    rc = log_handler_.commit(trx_id_, commit_xid);
  }

  operations_.clear();
  LOG_TRACE("append trx commit log. trx id=%d, commit_xid=%d, rc=%s",
      trx_id_, commit_xid, strrc(rc));
  return rc;
}

RC MvccTrx::rollback()
{
  RC rc    = RC::SUCCESS;
  started_ = false;

  for (auto &operation : std::ranges::reverse_view(operations_)) {
    BaseTable *table = operation.table();
    switch (operation.type()) {
      case Operation::Type::INSERT: {
        const RID &rid = operation.rid();
        if (recovering_) {
          Record record;
          rc = table->get_record(rid, record);
          if (OB_SUCC(rc)) {
            Field begin_xid_field;
            Field end_xid_field;
            trx_fields(table, begin_xid_field, end_xid_field);
            if (begin_xid_field.get_int(record) != -trx_id_) {
              continue;
            }
          } else if (rc == RC::RECORD_NOT_EXIST) {
            rc = RC::SUCCESS;
            continue;
          } else {
            LOG_WARN("failed to get record while rollback. table=%s, rid=%s, rc=%s",
                table->name(), rid.to_string().c_str(), strrc(rc));
            return rc;
          }
        }

        rc = table->delete_record(rid);
        ASSERT(rc == RC::SUCCESS,
            "failed to delete insert record while rollback. rid=%s, rc=%s",
            rid.to_string().c_str(), strrc(rc));
        break;
      }

      case Operation::Type::DELETE: {
        const RID &rid = operation.rid();

        Field begin_xid_field;
        Field end_xid_field;
        trx_fields(table, begin_xid_field, end_xid_field);

        auto record_updater = [this, &end_xid_field](Record &record) -> bool {
          if (recovering_ && end_xid_field.get_int(record) != -trx_id_) {
            return false;
          }

          ASSERT(end_xid_field.get_int(record) == -trx_id_,
              "got an invalid record while rollback. end xid=%d, trx id=%d",
              end_xid_field.get_int(record), trx_id_);

          end_xid_field.set_int(record, trx_kit_.max_trx_id());
          return true;
        };

        rc = table->visit_record(rid, record_updater);
        ASSERT(rc == RC::SUCCESS,
            "failed to restore delete record while rollback. rid=%s, rc=%s",
            rid.to_string().c_str(), strrc(rc));
        break;
      }

      case Operation::Type::UPDATE: {  // 回滚阶段，将旧版本的 end_xid 从负值转为提交ID（正式标记失效），将新版本的
                                       // begin_xid 从负值转为提交ID（正式生效）
        const Record &old_version     = operation.old_record();
        const Record &new_version     = operation.updated_record();
        const RID    &old_rid         = old_version.rid();
        const RID    &new_rid         = new_version.rid();
        bool          need_remove_new = true;

        if (recovering_) {
          Record record;
          rc = table->get_record(new_rid, record);
          if (OB_SUCC(rc)) {
            Field begin_field;
            Field end_field;
            trx_fields(table, begin_field, end_field);
            if (begin_field.get_int(record) != -trx_id_) {
              need_remove_new = false;
            }
          } else if (rc == RC::RECORD_NOT_EXIST) {
            need_remove_new = false;
            rc              = RC::SUCCESS;
          } else {
            LOG_WARN("failed to get new version while rollback. table=%s, rid=%s, rc=%s",
                table->name(), new_rid.to_string().c_str(), strrc(rc));
            return rc;
          }
        }

        if (need_remove_new) {
          rc = table->delete_record(new_version);
          ASSERT(rc == RC::SUCCESS,
              "failed to delete new version while rollback. rid=%s, rc=%s",
              new_rid.to_string().c_str(), strrc(rc));
        }

        Field begin_field;
        Field end_field;
        trx_fields(table, begin_field, end_field);

        auto recoverer = [this, &end_field](Record &record) -> bool {
          if (recovering_ && end_field.get_int(record) != -trx_id_) {
            return false;
          }
          end_field.set_int(record, trx_kit_.max_trx_id());
          return true;
        };

        rc = table->visit_record(old_rid, recoverer);
        ASSERT(rc == RC::SUCCESS,
            "failed to restore old version while rollback. rid=%s, rc=%s",
            old_rid.to_string().c_str(), strrc(rc));
        break;
      }

      default: {
        LOG_PANIC("unsupported operation type while rollback. type=%d",
            static_cast<int>(operation.type()));
        break;
      }
    }
  }

  operations_.clear();

  if (!recovering_) {
    rc = log_handler_.rollback(trx_id_);
  }
  LOG_TRACE("append trx rollback log. trx id=%d, rc=%s", trx_id_, strrc(rc));
  return rc;
}

RC find_table(Db *db, const LogEntry &log_entry, BaseTable *&table)
{
  auto *trx_log_header = reinterpret_cast<const MvccTrxLogHeader *>(log_entry.data());
  switch (MvccTrxLogOperation(trx_log_header->operation_type).type()) {
    case MvccTrxLogOperation::Type::INSERT_RECORD:
    case MvccTrxLogOperation::Type::DELETE_RECORD: {
      auto *trx_log_record = reinterpret_cast<const MvccTrxRecordLogEntry *>(log_entry.data());
      table                = db->find_table(trx_log_record->table_id);
      if (nullptr == table) {
        LOG_WARN("no such table to redo. log record=%s", trx_log_record->to_string().c_str());
        return RC::SCHEMA_TABLE_NOT_EXIST;
      }
    } break;
    default: {
      // do nothing
    } break;
  }
  return RC::SUCCESS;
}

RC MvccTrx::redo(Db *db, const LogEntry &log_entry)
{
  auto      *trx_log_header = reinterpret_cast<const MvccTrxLogHeader *>(log_entry.data());
  BaseTable *base_table     = nullptr;
  RC         rc             = find_table(db, log_entry, base_table);
  if (OB_FAIL(rc)) {
    return rc;
  }

  ASSERT(base_table->type() == TableType::Table, "Only tables support MVCC. The provided base_table is not of type Table.");
  Table *table = static_cast<Table *>(base_table);
  switch (MvccTrxLogOperation(trx_log_header->operation_type).type()) {
    case MvccTrxLogOperation::Type::INSERT_RECORD: {
      auto *trx_log_record = reinterpret_cast<const MvccTrxRecordLogEntry *>(log_entry.data());
      operations_.push_back(Operation(Operation::Type::INSERT, table, trx_log_record->rid));
    } break;

    case MvccTrxLogOperation::Type::DELETE_RECORD: {
      auto *trx_log_record = reinterpret_cast<const MvccTrxRecordLogEntry *>(log_entry.data());
      operations_.push_back(Operation(Operation::Type::DELETE, table, trx_log_record->rid));
    } break;

    case MvccTrxLogOperation::Type::COMMIT: {
      // auto *trx_log_record = reinterpret_cast<const MvccTrxCommitLogEntry *>(log_entry.data());
      // commit_with_trx_id(trx_log_record->commit_trx_id);
      // 遇到了提交日志，说明前面的记录都已经提交成功了
    } break;

    case MvccTrxLogOperation::Type::ROLLBACK: {
      // do nothing
      // 遇到了回滚日志，前面的回滚操作也都执行完成了
    } break;

    default: {
      ASSERT(false, "unsupported redo log. log_record=%s", log_entry.to_string().c_str());
      return RC::INTERNAL;
    } break;
  }

  return RC::SUCCESS;
}
