 /***************************************************************
  *                                                             *
  * @Author      : Codex                                        *
  * @Date        : 2025/10/26                                   *
  * @Description : DropIndexStmt source file                    *
  *                                                             *
  ***************************************************************/

#include "sql/stmt/drop_index_stmt.h"

#include "common/lang/string.h"
#include "common/log/log.h"
#include "sql/parser/parse_defs.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

RC DropIndexStmt::create(Db *db, const DropIndexSqlNode &drop_index, Stmt *&stmt)
{
  stmt = nullptr;

  if (db == nullptr || drop_index.relation_name.empty() || drop_index.index_name.empty()) {
    LOG_WARN("Invalid arguments when creating drop index stmt");
    return RC::INVALID_ARGUMENT;
  }

  BaseTable *base_table = db->find_table(drop_index.relation_name.c_str());
  if (base_table == nullptr) {
    LOG_WARN("table %s not exists when dropping index %s", drop_index.relation_name.c_str(), drop_index.index_name.c_str());
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  if (base_table->type() != TableType::Table) {
    LOG_WARN("can not drop index on non-base table. table=%s", drop_index.relation_name.c_str());
    return RC::CREATE_INDEX_ON_NON_TABLE_TYPE;
  }

  Table *table = static_cast<Table *>(base_table);
  if (table->find_index(drop_index.index_name.c_str()) == nullptr) {
    LOG_WARN("index %s not found on table %s", drop_index.index_name.c_str(), drop_index.relation_name.c_str());
    return RC::SCHEMA_INDEX_NOT_EXIST;
  }

  stmt = new DropIndexStmt(table, drop_index.index_name);
  return RC::SUCCESS;
}
