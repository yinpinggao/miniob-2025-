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
// Created by WangYunlai on 2022/11/18.
//

#include "sql/executor/sql_result.h"
#include "common/log/log.h"
#include "common/rc.h"
#include "session/session.h"
#include "storage/trx/trx.h"

SqlResult::SqlResult(Session *session) : session_(session) {}

void SqlResult::set_tuple_schema(const TupleSchema &schema) { tuple_schema_ = schema; }

RC SqlResult::open()
{
  if (nullptr == operator_) {
    return RC::INVALID_ARGUMENT;
  }

  // SqlResult 是物理计划的驱动者。算子树的 open 会自顶向下初始化扫描器、
  // 子算子和临时状态；事务也在第一次真正执行时按需启动。
  Trx *trx = session_->current_trx();
  trx->start_if_need();
  return operator_->open(trx);
}

RC SqlResult::close()
{
  if (nullptr == operator_) {
    return RC::INVALID_ARGUMENT;
  }
  RC rc = operator_->close();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to close operator. rc=%s", strrc(rc));
  }

  // close 必须释放扫描器、临时文件和内存状态；随后销毁整棵物理算子树。
  operator_.reset();

  // 非显式事务模式下，一条 SQL 就是一个事务边界：成功自动提交，失败回滚。
  // BEGIN 后 session 进入 multi-operation mode，此处不会替用户提前提交。
  if (session_ && !session_->is_trx_multi_operation_mode()) {
    if (rc == RC::SUCCESS) {
      rc = session_->current_trx()->commit();
    } else {
      RC rc2 = session_->current_trx()->rollback();
      if (rc2 != RC::SUCCESS) {
        LOG_PANIC("rollback failed. rc=%s", strrc(rc2));
      }
    }
  }
  return rc;
}

RC SqlResult::next_tuple(Tuple *&tuple)
{
  // Volcano 模型：父算子每调用一次 next，子树只生产下一条结果。
  // RC::RECORD_EOF 表示数据结束，不是执行错误。
  RC rc = operator_->next();
  if (rc != RC::SUCCESS) {
    return rc;
  }

  tuple = operator_->current_tuple();
  return rc;
}

RC SqlResult::next_chunk(Chunk &chunk)
{
  // Chunk 模式与 next_tuple 语义相同，但一次返回一批列式数据。
  RC rc = operator_->next(chunk);
  return rc;
}

void SqlResult::set_operator(std::unique_ptr<PhysicalOperator> oper)
{
  ASSERT(operator_ == nullptr, "current operator is not null. Result is not closed?");
  operator_ = std::move(oper);
  // tuple_schema 从根算子向外暴露结果列名/类型，Communicator 据此打印表头。
  operator_->tuple_schema(tuple_schema_);
}
