/* Copyright (c) 2021OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include <utility>

#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "storage/table/view.h"
#include "sql/stmt/insert_stmt.h"

InsertStmt::InsertStmt(
    BaseTable *table, std::vector<std::vector<Value>> values_list, std::vector<std::vector<uint8_t>> column_masks)
    : table_(table), values_list_(std::move(values_list)), column_masks_(std::move(column_masks))
{}

RC InsertStmt::create(Db *db, const InsertSqlNode &inserts, Stmt *&stmt)
{
  const char *table_name = inserts.relation_name.c_str();
  if (nullptr == db || nullptr == table_name || inserts.values_list.empty()) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, value_num=%d",
        db, table_name, static_cast<int>(inserts.values_list.size()));
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  auto table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // 获取表的元数据（在所有检查中都需要使用）
  const TableMeta &table_meta = table->table_meta();
  
  if (table->type() == TableType::View) {
    // 检查视图是否可变（不包含聚合或 GROUP BY/HAVING）
    if (!table->is_mutable()) {
      LOG_ERROR("The target table %s of the INSERT is not insertable-into", table->name());
      return RC::READ_ONLY_VIEW_INSERT_ERROR;
    }
    
    auto view = dynamic_cast<View *>(table);
    auto field_metas = table_meta.field_metas();
    
    // 多表视图的检查
    if (view->has_join()) {
      // 多表视图没有指定字段列表时，不允许插入
      if (inserts.attr_names.empty()) {
        LOG_ERROR("Can not insert into join view '%s.%s' without fields list", db->name(), table->name());
        return RC::JOIN_VIEW_INSERT_ERROR;
      }
      
      // 确保视图已初始化
      RC rc = view->ensure_initialized();
      if (rc != RC::SUCCESS) {
        return rc;
      }
      
      // 获取字段索引映射
      auto &field_index = view->field_index();
      
      // 检查指定的字段是否都来自同一个基表
      BaseTable *target_table = nullptr;
      const int sys_field_num = table_meta.sys_field_num();
      
      for (const auto &attr_name : inserts.attr_names) {
        // 查找字段在视图中的索引
        int view_field_idx = -1;
        for (size_t i = 0; i < field_metas->size(); ++i) {
          if (strcmp((*field_metas)[i].name(), attr_name.c_str()) == 0) {
            view_field_idx = i;
            break;
          }
        }
        
        if (view_field_idx < 0) {
          LOG_WARN("Field does not exist. db=%s, table_name=%s, field_name=%s",
                    db->name(), table_name, attr_name.c_str());
          return RC::SCHEMA_FIELD_NOT_EXIST;
        }
        
        // field_index 只包含逻辑字段，需要减去系统字段的偏移
        int logical_field_idx = view_field_idx - sys_field_num;
        if (logical_field_idx < 0 || logical_field_idx >= static_cast<int>(field_index.size())) {
          LOG_ERROR("System field '%s' cannot be inserted", attr_name.c_str());
          return RC::INVALID_ARGUMENT;
        }
        
        // 通过 field_index 获取字段对应的基表
        auto &[base_table, field_id] = field_index[logical_field_idx];
        
        if (base_table == nullptr) {
          LOG_ERROR("Field '%s' is an expression field, cannot be used in insert", attr_name.c_str());
          return RC::EXPRESSION_FIELD_NOT_INSERTABLE;
        }
        
        // 检查是否所有字段都来自同一个基表
        if (target_table == nullptr) {
          target_table = base_table;
        } else if (target_table != base_table) {
          LOG_ERROR("Can not insert into join view '%s.%s' with fields from multiple tables", 
                     db->name(), table->name());
          return RC::JOIN_VIEW_INSERT_ERROR;
        }
      }
    } else {
      // 单表视图：检查视图是否包含表达式字段
      // 如果视图包含任何表达式字段（不可变字段），则完全禁止插入
      const int sys_field_num = table_meta.sys_field_num();
      for (size_t i = sys_field_num; i < field_metas->size(); ++i) {
        auto &field_meta = (*field_metas)[i];
        if (!field_meta.is_mutable()) {
          LOG_ERROR("The target table %s of the INSERT is not insertable-into because it contains expression field '%s'", 
                    table->name(), field_meta.name());
          return RC::EXPRESSION_FIELD_NOT_INSERTABLE;
        }
      }
    }
  }

  std::vector<std::vector<Value>> values_list = inserts.values_list;
  const int                       field_num   = table_meta.field_num() - table_meta.sys_field_num();

  // check the fields number
  for (auto &value_list : inserts.values_list) {
    const int value_num = static_cast<int>(value_list.size());
    if (inserts.attr_names.empty()) {
      if (field_num != value_num) {
        LOG_WARN("schema mismatch. value num=%d, field num in schema=%d", value_num, field_num);
        return RC::SCHEMA_FIELD_MISSING;
      }
    } else if (field_num < value_num || inserts.attr_names.size() != value_num) {
      LOG_WARN("schema mismatch. attr num=%d, value num=%d, field num in schema=%d", inserts.attr_names.size(), value_num, field_num);
      return RC::INVALID_ARGUMENT;
    }
  }

  if (!inserts.attr_names.empty()) {
    // 在物理算子执行阶段检查值的可为空性
    // 预处理索引
    std::unordered_set<int> field_ids;
    std::vector<int>        index(inserts.attr_names.size(), -1);
    for (size_t i = 0; i < inserts.attr_names.size(); ++i) {
      auto &attr_name  = inserts.attr_names[i];
      auto  field_meta = table_meta.field(attr_name.c_str());
      if (field_meta != nullptr) {
        // 出现两次同名列
        if (field_ids.count(field_meta->field_id())) {
          LOG_ERROR("Column '%s' specified twice", attr_name.c_str());
          return RC::INVALID_ARGUMENT;
        }
        index[i] = field_meta->field_id();
        field_ids.emplace(index[i]);
      } else {
        LOG_WARN("Field does not exist. db=%s, table_name=%s, field_name=%s",
                  db->name(), table_name, attr_name.c_str());
        return RC::SCHEMA_FIELD_NOT_EXIST;
      }
    }

    for (auto &value_list : values_list) {
      std::vector<Value> values(field_num);
      for (auto &value : values) {
        value.set_null();
      }
      for (size_t k = 0; k < value_list.size(); ++k) {
        values[index[k]] = value_list[k];
      }
      value_list = std::move(values);
    }
  }

  // everything alright
  stmt = new InsertStmt(table, std::move(values_list), {});
  return RC::SUCCESS;
}
