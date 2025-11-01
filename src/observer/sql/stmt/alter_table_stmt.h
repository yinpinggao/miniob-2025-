#pragma once

#include <string>

#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"

class Db;
class Table;

class AlterTableStmt : public Stmt
{
public:
  using ActionType = AlterType;

  AlterTableStmt(Table *table, AlterTableSqlNode alter_info);

  StmtType type() const override { return StmtType::ALTER_TABLE; }

  Table *table() const { return table_; }
  const AlterTableSqlNode &alter_info() const { return alter_info_; }
  ActionType action_type() const { return alter_info_.alter_type; }
  const AttrInfoSqlNode &new_column() const { return alter_info_.new_column; }
  const std::string &column_name() const { return alter_info_.column_name; }
  const std::string &new_column_name() const { return alter_info_.new_column_name; }
  const std::string &new_table_name() const { return alter_info_.new_table_name; }
  const std::string &table_name() const { return alter_info_.table_name; }
  const FullTextIndexConfig &fulltext_index_config() const { return alter_info_.fulltext_index_config; }

  static RC create(Db *db, const AlterTableSqlNode &alter_table, Stmt *&stmt);

private:
  Table               *table_ = nullptr;
  AlterTableSqlNode    alter_info_;
};

