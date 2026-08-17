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
// Created by Wangyunlai on 2024/01/10.
//

#include "net/sql_task_handler.h"
#include "net/communicator.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"

RC SqlTaskHandler::handle_event(Communicator *communicator)
{
  // 一次 handle_event 对应客户端的一次请求。Communicator 屏蔽了 CLI、普通 TCP、
  // MySQL 协议等接入方式的差异，上层从这里开始只处理统一的 SessionEvent。
  SessionEvent *event = nullptr;
  RC            rc    = communicator->read_event(event);
  if (OB_FAIL(rc)) {
    return rc;
  }

  if (nullptr == event) {
    return RC::SUCCESS;
  }

  // 这里只借助 SessionStage 设置线程局部的当前 Session/Request。
  // 真正的 SQL 阶段调度发生在下面的 handle_sql，而不是旧版
  // SessionStage::handle_sql；阅读主链路时不要被旧接口误导。
  session_stage_.handle_request2(event);

  // SQLStageEvent 是各 SQL 处理阶段共享的上下文容器：最初只有原始 SQL，
  // 后续依次装入 ParsedSqlNode、Stmt、Logical/PhysicalOperator 等中间结果。
  SQLStageEvent sql_event(event, event->query());

  rc = handle_sql(&sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to handle sql. rc=%s", strrc(rc));
    event->sql_result()->set_return_code(rc);
  }

  bool need_disconnect = false;

  // 查询结果并不一定已经被一次性物化。Communicator 写结果时会通过
  // SqlResult 驱动物理算子 open/next/close，逐行或逐 Chunk 拉取结果。
  rc = communicator->write_result(event, need_disconnect);
  LOG_INFO("write result return %s", strrc(rc));
  event->session()->set_current_request(nullptr);
  Session::set_current_session(nullptr);

  delete event;

  if (need_disconnect) {
    return RC::INTERNAL;
  }
  return RC::SUCCESS;
}

RC SqlTaskHandler::handle_sql(SQLStageEvent *sql_event)
{
  // 面试时可将本函数概括为 MiniOB 的 SQL 编译与执行流水线：
  // QueryCache -> Parse -> Resolve -> Optimize -> Execute。
  // 前四步主要构造和转换中间表示，真正读取/修改记录通常发生在结果输出阶段。
  RC rc = query_cache_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do query cache. rc=%s", strrc(rc));
    return rc;
  }

  // SQL 字符串 -> ParsedSqlNode。这里只检查语法形态，不确认表、字段是否存在。
  rc = parse_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do parse. rc=%s", strrc(rc));
    return rc;
  }

  // ParsedSqlNode -> 具体 Stmt，同时绑定表、字段、函数和表达式。
  rc = resolve_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do resolve. rc=%s", strrc(rc));
    return rc;
  }

  // 查询/DML 会生成逻辑计划并转换为物理算子树；DDL 通常返回 UNIMPLEMENTED，
  // 随后由 ExecuteStage 中的 CommandExecutor 直接执行。
  rc = optimize_stage_.handle_request(sql_event);
  if (rc != RC::UNIMPLEMENTED && rc != RC::SUCCESS) {
    LOG_TRACE("failed to do optimize. rc=%s", strrc(rc));
    return rc;
  }

  // 有物理计划时，将算子树交给 SqlResult；没有物理计划时直接执行 DDL/命令。
  rc = execute_stage_.handle_request(sql_event);
  if (OB_FAIL(rc)) {
    LOG_TRACE("failed to do execute. rc=%s", strrc(rc));
    return rc;
  }

  return rc;
}
