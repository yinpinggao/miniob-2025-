#include "sql/executor/drop_index_executor.h"

#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/stmt/drop_index_stmt.h"
#include "sql/stmt/stmt.h"
#include "storage/table/table.h"

RC DropIndexExecutor::execute(SQLStageEvent *sql_event)
{
  auto *stmt = sql_event->stmt();
  ASSERT(stmt != nullptr && stmt->type() == StmtType::DROP_INDEX,
      "drop index executor can not run this command: %d",
      stmt ? static_cast<int>(stmt->type()) : -1);

  auto *drop_stmt = static_cast<DropIndexStmt *>(stmt);
  Table *table     = drop_stmt->table();
  return table->drop_index(drop_stmt->index_name().c_str());
}

