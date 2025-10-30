#include "sql/executor/alter_table_executor.h"

#include "event/sql_event.h"
#include "event/session_event.h"
#include "session/session.h"
#include "sql/stmt/alter_table_stmt.h"
#include "sql/stmt/stmt.h"
#include "storage/db/db.h"

RC AlterTableExecutor::execute(SQLStageEvent *sql_event)
{
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
