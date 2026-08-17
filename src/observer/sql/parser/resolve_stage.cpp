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

#include <string.h>
#include <string>

#include "resolve_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/stmt.h"

using namespace common;

RC ResolveStage::handle_request(SQLStageEvent *sql_event)
{
  // Resolve 阶段负责“语义”：把通用语法节点转换成 SelectStmt、UpdateStmt
  // 等具体语句对象，并检查表/字段是否存在、名字是否歧义、表达式如何绑定。
  // 注意它不是完整的静态类型系统，部分类型转换和函数参数错误仍可能执行时报出。
  RC            rc            = RC::SUCCESS;
  SessionEvent *session_event = sql_event->session_event();
  SqlResult    *sql_result    = session_event->sql_result();

  Db *db = session_event->session()->get_current_db();
  if (nullptr == db) {
    LOG_ERROR("cannot find current db");
    rc = RC::SCHEMA_DB_NOT_EXIST;
    sql_result->set_return_code(rc);
    sql_result->set_state_string("no db selected");
    return rc;
  }

  ParsedSqlNode *sql_node = sql_event->sql_node().get();
  Stmt          *stmt     = nullptr;

  // Stmt::create_stmt 根据 sql_node->flag 分派到各 XxxStmt::create。
  // Stmt 可以理解为“已完成名称绑定、可用于生成关系计划的 SQL 语义对象”。
  rc = Stmt::create_stmt(db, *sql_node, stmt);
  if (rc != RC::SUCCESS && rc != RC::UNIMPLEMENTED) {
    LOG_WARN("failed to create stmt. rc=%d:%s", rc, strrc(rc));
    sql_result->set_return_code(rc);
    return rc;
  }

  // SQLStageEvent 接管 Stmt，下一阶段根据 StmtType 生成逻辑计划。
  sql_event->set_stmt(stmt);

  return rc;
}
