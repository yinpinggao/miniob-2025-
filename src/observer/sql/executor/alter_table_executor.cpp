#include "sql/executor/alter_table_executor.h"

#include "event/sql_event.h"
#include "event/session_event.h"
#include "session/session.h"
#include "sql/stmt/alter_table_stmt.h"
#include "sql/stmt/stmt.h"
#include "storage/db/db.h"

RC AlterTableExecutor::execute(SQLStageEvent *sql_event)
{
  // 【赛题 19 alter】ALTER 是 DDL 直接执行路径，不生成关系算子树。Stmt 完成
  // 动作与参数校验后，由 Db::alter_table 分派到 Table 的 schema 重写实现。
  Stmt *stmt = sql_event->stmt();
  ASSERT(stmt != nullptr, "alter table executor requires a stmt");
  ASSERT(stmt->type() == StmtType::ALTER_TABLE, "unexpected stmt type: %d", static_cast<int>(stmt->type()));

  auto *alter_stmt = dynamic_cast<AlterTableStmt *>(stmt);
  ASSERT(alter_stmt != nullptr, "stmt type mismatch for alter table");

  Session *session = sql_event->session_event()->session();
  Db      *db      = session->get_current_db();
  if (db == nullptr) {
    return RC::SCHEMA_DB_NOT_EXIST;
  }

  return db->alter_table(*alter_stmt);
}
