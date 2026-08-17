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

#include "parse_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/parser/parse.h"

using namespace common;

RC ParseStage::handle_request(SQLStageEvent *sql_event)
{
  // ParseStage 类似编译器的词法/语法分析阶段：
  // 输入是 SQL 字符串，输出是尚未绑定真实表和字段的 ParsedSqlNode。
  // 例如这里能够识别 "id + 1" 是算术表达式，但不知道 id 属于哪张表。
  RC rc = RC::SUCCESS;

  SqlResult         *sql_result = sql_event->session_event()->sql_result();
  const std::string &sql        = sql_event->sql();

  ParsedSqlResult parsed_sql_result;

  // parse 内部由 lex_sql.l 与 yacc_sql.y 生成的 lexer/parser 完成工作。
  parse(sql.c_str(), &parsed_sql_result);
  if (parsed_sql_result.sql_nodes().empty()) {
    sql_result->set_return_code(RC::SUCCESS);
    sql_result->set_state_string("");
    return RC::INTERNAL;
  }

  // Parser 可以解析多条 SQL，但当前请求处理链只执行第一条。
  if (parsed_sql_result.sql_nodes().size() > 1) {
    LOG_WARN("got multi sql commands but only 1 will be handled");
  }

  std::unique_ptr<ParsedSqlNode> sql_node = std::move(parsed_sql_result.sql_nodes().front());
  if (sql_node->flag == SCF_ERROR) {
    // set error information to event
    rc = RC::SQL_SYNTAX;
    sql_result->set_return_code(rc);
    std::string error_msg = "Failed to parse sql";
    if (!sql_node->error.error_msg.empty()) {
      error_msg += ": " + sql_node->error.error_msg;
      LOG_WARN("SQL parse error at line %d, column %d: %s", 
               sql_node->error.line, sql_node->error.column, sql_node->error.error_msg.c_str());
    }
    sql_result->set_state_string(error_msg);
    return rc;
  }

  // 所有权移入 SQLStageEvent，供 ResolveStage 消费，避免复制整棵语法树。
  sql_event->set_sql_node(std::move(sql_node));

  return RC::SUCCESS;
}
