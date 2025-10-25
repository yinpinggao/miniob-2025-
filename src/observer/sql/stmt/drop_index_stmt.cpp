#include "sql/stmt/drop_index_stmt.h"

#include "common/lang/string.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

using common::is_blank;
RC DropIndexStmt::create(Db *db, const DropIndexSqlNode &drop_index, Stmt *&stmt)
{
  stmt = nullptr;

  if (db == nullptr) {
    return RC::INVALID_ARGUMENT;
  }

  const char *table_name = drop_index.relation_name.c_str();
  const char *index_name = drop_index.index_name.c_str();
  if (is_blank(table_name) || is_blank(index_name)) {
    LOG_WARN("invalid drop index arguments. table or index name empty");
    return RC::INVALID_ARGUMENT;
  }

  BaseTable *base_table = db->find_table(table_name);
  if (base_table == nullptr) {
    LOG_WARN("table %s does not exist when dropping index %s", table_name, index_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  if (base_table->type() != TableType::Table) {
    LOG_WARN("drop index only supports base table. name=%s", table_name);
    return RC::UNSUPPORTED;
  }

  Table *table = static_cast<Table *>(base_table);
  Index *index = table->find_index(index_name);
  if (index == nullptr) {
    LOG_WARN("index %s does not exist on table %s", index_name, table_name);
    return RC::SCHEMA_INDEX_NOT_EXIST;
  }

  stmt = new DropIndexStmt(table, index_name);
  return RC::SUCCESS;
}

