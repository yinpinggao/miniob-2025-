#include "sql/stmt/alter_table_stmt.h"

#include <utility>

#include "common/lang/string.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

using namespace common;

AlterTableStmt::AlterTableStmt(Table *table, AlterTableSqlNode alter_info)
    : table_(table), alter_info_(std::move(alter_info))
{}

RC AlterTableStmt::create(Db *db, const AlterTableSqlNode &alter_table, Stmt *&stmt)
{
  stmt = nullptr;

  if (nullptr == db) {
    return RC::INVALID_ARGUMENT;
  }

  const char *table_name = alter_table.table_name.c_str();
  if (is_blank(table_name)) {
    LOG_WARN("invalid alter table stmt: empty table name");
    return RC::INVALID_ARGUMENT;
  }

  BaseTable *base_table = db->find_table(table_name);
  if (nullptr == base_table) {
    LOG_WARN("table does not exist. table=%s", table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  if (base_table->type() != TableType::Table) {
    LOG_WARN("alter table only supports base table. table=%s", table_name);
    return RC::UNSUPPORTED;
  }

  Table *table = static_cast<Table *>(base_table);

  switch (alter_table.alter_type) {
    case AlterType::ADD_COLUMN: {
      if (is_blank(alter_table.new_column.name.c_str())) {
        LOG_WARN("invalid alter table add column: column name empty");
        return RC::INVALID_ARGUMENT;
      }
    } break;
    case AlterType::DROP_COLUMN: {
      if (is_blank(alter_table.column_name.c_str())) {
        LOG_WARN("invalid alter table drop column: column name empty");
        return RC::INVALID_ARGUMENT;
      }
    } break;
    case AlterType::CHANGE_COLUMN: {
      if (is_blank(alter_table.column_name.c_str()) || is_blank(alter_table.new_column_name.c_str())) {
        LOG_WARN("invalid alter table change column: column name empty");
        return RC::INVALID_ARGUMENT;
      }
      if (alter_table.column_name == alter_table.new_column_name) {
        LOG_WARN("change column has identical old/new name: %s", alter_table.column_name.c_str());
        return RC::INVALID_ARGUMENT;
      }
    } break;
    case AlterType::RENAME_TABLE: {
      if (is_blank(alter_table.new_table_name.c_str())) {
        LOG_WARN("invalid alter table rename: new table name empty");
        return RC::INVALID_ARGUMENT;
      }
    } break;
    default: {
      LOG_WARN("unsupported alter table action");
      return RC::UNIMPLEMENTED;
    }
  }

  AlterTableSqlNode alter_info = alter_table;
  alter_info.table_name        = table_name;  // ensure old name kept as string copy

  stmt = new AlterTableStmt(table, std::move(alter_info));
  return RC::SUCCESS;
}
