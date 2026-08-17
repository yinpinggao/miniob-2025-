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
// Created by Longda on 2021/4/13.
//

#include <sstream>
#include <string>

#include "sql/executor/execute_stage.h"

#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/executor/command_executor.h"
#include "sql/operator/calc_physical_operator.h"
#include "sql/stmt/select_stmt.h"
#include "sql/stmt/stmt.h"
#include "storage/default/default_handler.h"

using namespace std;
using namespace common;

RC ExecuteStage::handle_request(SQLStageEvent *sql_event)
{
  // ExecuteStage 是两条执行路径的分界点：
  // - SELECT/INSERT/DELETE/UPDATE 等已有物理算子树的语句交给 SqlResult；
  // - CREATE/DROP/DESC/BEGIN/COMMIT 等命令由 CommandExecutor 立即执行。
  RC rc = RC::SUCCESS;

  const unique_ptr<PhysicalOperator> &physical_operator = sql_event->physical_operator();
  if (physical_operator != nullptr) {
    return handle_request_with_physical_operator(sql_event);
  }

  SessionEvent *session_event = sql_event->session_event();

  Stmt *stmt = sql_event->stmt();
  if (stmt != nullptr) {
    // DDL 通常不产生可迭代的数据流，执行器直接修改 Db/Table 元数据或文件。
    CommandExecutor command_executor;
    rc = command_executor.execute(sql_event);
    session_event->sql_result()->set_return_code(rc);
  } else {
    return RC::INTERNAL;
  }
  return rc;
}

RC ExecuteStage::handle_request_with_physical_operator(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;

  unique_ptr<PhysicalOperator> &physical_operator = sql_event->physical_operator();
  ASSERT(physical_operator != nullptr, "physical operator should not be null");

  // 此处仅转移算子树所有权，并不立刻把查询全部执行完。
  // Communicator 输出结果时才会调用 SqlResult::open/next/close 拉取数据。
  SqlResult *sql_result = sql_event->session_event()->sql_result();
  sql_result->set_operator(std::move(physical_operator));
  return rc;
}
