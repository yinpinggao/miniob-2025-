 /***************************************************************
  *                                                             *
  * @Author      : Codex                                        *
  * @Date        : 2025/10/26                                   *
  * @Description : DropIndexStmt header file                    *
  *                                                             *
  ***************************************************************/

#pragma once

#include <string>

#include "sql/stmt/stmt.h"

struct DropIndexSqlNode;
class Table;

/**
 * @brief 删除索引的语句
 */
class DropIndexStmt : public Stmt
{
public:
  DropIndexStmt(Table *table, std::string index_name) : table_(table), index_name_(std::move(index_name)) {}
  ~DropIndexStmt() override = default;

  StmtType type() const override { return StmtType::DROP_INDEX; }

  Table              *table() const { return table_; }
  const std::string  &index_name() const { return index_name_; }

  static RC create(Db *db, const DropIndexSqlNode &drop_index, Stmt *&stmt);

private:
  Table        *table_ = nullptr;
  std::string   index_name_;
};
