 /***************************************************************
  *                                                             *
  * @Author      : Codex                                        *
  * @Date        : 2025/10/26                                   *
  * @Description : DropIndexExecutor source file                *
  *                                                             *
  ***************************************************************/

#include "sql/executor/drop_index_executor.h"

#include "common/log/log.h"
#include "event/sql_event.h"
#include "sql/stmt/drop_index_stmt.h"
#include "storage/table/table.h"

RC DropIndexExecutor::execute(SQLStageEvent *sql_event)
{
  Stmt *stmt = sql_event->stmt();
  ASSERT(stmt->type() == StmtType::DROP_INDEX,
      "drop index executor can not run this command: %d",
      static_cast<int>(stmt->type()));

  auto *drop_index_stmt = static_cast<DropIndexStmt *>(stmt);
  Table *table          = drop_index_stmt->table();

  RC rc = table->drop_index(drop_index_stmt->index_name().c_str());
  LOG_TRACE("drop index finished. table=%s index=%s rc=%s", table->name(), drop_index_stmt->index_name().c_str(), strrc(rc));
  return rc;
}
