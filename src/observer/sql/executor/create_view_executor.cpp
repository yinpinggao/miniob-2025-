/***************************************************************
 *                                                             *
 * @Author      : Koschei                                      *
 * @Email       : nitianzero@gmail.com                         *
 * @Date        : 2024/10/12                                   *
 * @Description : create view executor source file             *
 *                                                             *
 * Copyright (c) 2024 Koschei                                  *
 * All rights reserved.                                        *
 *                                                             *
 ***************************************************************/

#include "sql/executor/create_view_executor.h"
#include <cctype>
#include "common/log/log.h"
#include "common/lang/string.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/create_table_stmt.h"
#include "storage/db/db.h"
#include "storage/trx/trx.h"
#include "sql/stmt/create_view_stmt.h"

// 提取 AS 后的 SQL 语句
std::string extract_select_sql(const std::string &create_view_sql)
{
  if (create_view_sql.empty()) {
    return "";
  }

  auto is_identifier_char = [](char ch) {
    unsigned char c = static_cast<unsigned char>(ch);
    return std::isalnum(c) || c == '_';
  };

  std::string lower_sql = create_view_sql;
  common::str_to_lower(lower_sql);

  const std::string select_token = "select";
  size_t            select_pos   = std::string::npos;
  for (size_t i = 0; i + select_token.size() <= lower_sql.size(); ++i) {
    if (lower_sql.compare(i, select_token.size(), select_token) != 0) {
      continue;
    }
    bool left_ok  = (i == 0) || !is_identifier_char(lower_sql[i - 1]);
    bool right_ok = (i + select_token.size() >= lower_sql.size()) ||
                    !is_identifier_char(lower_sql[i + select_token.size()]);
    if (left_ok && right_ok) {
      select_pos = i;
      break;
    }
  }
  if (select_pos == std::string::npos) {
    return "";
  }

  size_t as_pos = std::string::npos;
  for (size_t i = select_pos; i > 0;) {
    --i;
    if (i + 2 > lower_sql.size()) {
      continue;
    }
    if (lower_sql.compare(i, 2, "as") != 0) {
      continue;
    }
    bool left_ok  = (i == 0) || !is_identifier_char(lower_sql[i - 1]);
    bool right_ok = (i + 2 >= lower_sql.size()) || !is_identifier_char(lower_sql[i + 2]);
    if (left_ok && right_ok) {
      as_pos = i;
      break;
    }
  }
  if (as_pos == std::string::npos) {
    return "";
  }

  size_t start_pos = as_pos + 2;
  while (start_pos < create_view_sql.size() && std::isspace(static_cast<unsigned char>(create_view_sql[start_pos]))) {
    ++start_pos;
  }

std::string result = create_view_sql.substr(start_pos);
  common::strip(result);
  return result;
}

RC CreateViewExecutor::execute(SQLStageEvent *sql_event)
{
  RC       rc;
  Stmt    *stmt    = sql_event->stmt();
  Session *session = sql_event->session_event()->session();
  ASSERT(stmt->type() == StmtType::CREATE_VIEW,
      "create view executor can not run this command: %d",
      static_cast<int>(stmt->type()));

  auto create_view_stmt = static_cast<CreateViewStmt *>(stmt);
  auto table_name       = create_view_stmt->table_name().c_str();
  auto select_stmt      = create_view_stmt->select_stmt();
  ASSERT(select_stmt != nullptr,
    "create view executor can not run this command: %d",
    static_cast<int>(stmt->type()));

  auto select_sql = extract_select_sql(sql_event->sql());
  if (select_sql.empty()) {
    return RC::SQL_SYNTAX;
  }

  rc = session->get_current_db()->create_table(table_name,
      std::move(create_view_stmt->attr_names()),
      std::move(select_sql),
      select_stmt,
      StorageFormat::ROW_FORMAT);
  if (OB_FAIL(rc)) {
    return rc;
  }

  return rc;
}
