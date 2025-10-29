# 视图更新规则实现说明

## 修改概要

根据提供的视图更新规则，对以下文件进行了修改：

1. **src/observer/storage/table/view.h** - 添加了 `tables()` 方法以访问视图的基表列表
2. **src/observer/sql/stmt/insert_stmt.cpp** - 完善了 INSERT 语句的视图更新规则检查
3. **src/observer/sql/stmt/update_stmt.cpp** - 完善了 UPDATE 语句的视图更新规则检查
4. **src/observer/sql/stmt/delete_stmt.cpp** - 修正了 DELETE 语句的错误消息

## 实现的规则

### 1. INSERT 操作

#### 单表视图（完整列）
- ✅ 允许插入
- 实现：无表达式字段时，允许插入

#### 单表视图（部分列）
- ✅ 允许插入指定列
- 实现：如果指定字段列表，只检查指定字段的可变性

#### 单表视图（含表达式）
- ❌ 不允许插入
- 实现：检查所有字段的可变性，如果有不可变字段（表达式列），拒绝插入
- 错误码：`RC::EXPRESSION_FIELD_NOT_INSERTABLE`

#### 多表视图（无字段列表）
- ❌ 不允许插入
- 实现：检查是否是 join 视图且没有指定字段列表
- 错误码：`RC::JOIN_VIEW_INSERT_ERROR`

#### 多表视图（有字段列表，单基表）
- ✅ 允许插入
- 实现：检查指定的字段是否都来自同一个基表，如果是则允许

#### 多表视图（有字段列表，多基表）
- ❌ 不允许插入
- 实现：检查指定的字段是否来自多个基表，如果是则拒绝
- 错误码：`RC::JOIN_VIEW_INSERT_ERROR`

### 2. UPDATE 操作

#### 单表视图
- ✅ 允许更新基础列
- ❌ 不允许更新表达式列
- 实现：检查字段的 `is_mutable()` 属性
- 错误码：`RC::EXPRESSION_FIELD_NOT_UPDATEABLE`

#### 多表视图（单基表字段）
- ✅ 允许更新
- 实现：检查所有更新的字段是否来自同一个基表

#### 多表视图（多基表字段）
- ❌ 不允许更新
- 实现：检查更新的字段是否来自多个基表，如果是则拒绝
- 错误码：`RC::JOIN_VIEW_INSERT_ERROR`（复用）

### 3. DELETE 操作

#### 单表视图
- ✅ 允许删除
- 实现：无特殊检查

#### 多表视图
- ❌ 不允许删除
- 实现：检查是否是 join 视图，如果是则拒绝
- 错误码：`RC::JOIN_VIEW_DELETE_ERROR`

### 4. 聚合视图
- ❌ 所有操作都不允许
- 实现：在视图创建时设置 `is_mutable = false`
- 错误码：`RC::READ_ONLY_VIEW_INSERT_ERROR` / `RC::READ_ONLY_VIEW_UPDATE_ERROR` / `RC::READ_ONLY_VIEW_DELETE_ERROR`

## 关键实现细节

### 视图初始化
在进行规则检查时，需要确保视图已经初始化（`init_data()` 和 `init_member()`），以便能够访问：
- 视图的基表列表（`tables_`）
- 字段到基表的映射（`field_index_`）

### 字段到基表的映射
通过在所有基表中查找字段名来确定字段所属的基表：
```cpp
for (auto &base_table : tables) {
  auto base_field_meta = base_table->table_meta().field(field_name);
  if (base_field_meta != nullptr) {
    field_base_table = base_table;
    break;
  }
}
```

### 可变性检查
- 表达式列的 `is_mutable()` 返回 `false`
- 基础列的 `is_mutable()` 继承自基表字段的可变性
- 聚合视图的 `is_mutable()` 返回 `false`

## 测试建议

### 测试用例 1: 单表视图（完整列）
```sql
CREATE TABLE t1(id INT, name VARCHAR(50), age INT);
CREATE VIEW v1 AS SELECT * FROM t1;
INSERT INTO v1 VALUES(1, 'Alice', 20);  -- 应该成功
UPDATE v1 SET name='Bob' WHERE id=1;     -- 应该成功
DELETE FROM v1 WHERE id=1;               -- 应该成功
```

### 测试用例 2: 单表视图（部分列）
```sql
CREATE TABLE t2(id INT, name VARCHAR(50), age INT);
CREATE VIEW v2(id, name) AS SELECT id, name FROM t2;
INSERT INTO v2(id, name) VALUES(1, 'Alice');  -- 应该成功
INSERT INTO v2 VALUES(1, 'Alice');             -- 应该成功（age 设为 NULL）
UPDATE v2 SET name='Bob' WHERE id=1;           -- 应该成功
DELETE FROM v2 WHERE id=1;                     -- 应该成功
```

### 测试用例 3: 单表视图（含表达式）
```sql
CREATE TABLE t3(id INT, age INT);
CREATE VIEW v3 AS SELECT id, id+age AS data FROM t3;
INSERT INTO v3 VALUES(1, 30);              -- 应该失败（含表达式列）
UPDATE v3 SET id=2 WHERE id=1;             -- 应该成功（更新基础列）
UPDATE v3 SET data=30 WHERE id=1;          -- 应该失败（更新表达式列）
DELETE FROM v3 WHERE id=1;                 -- 应该成功
```

### 测试用例 4: 多表视图（无字段列表）
```sql
CREATE TABLE t4(id INT, name VARCHAR(50));
CREATE TABLE t5(id INT, age INT);
CREATE VIEW v4 AS SELECT t4.id, t4.name, t5.age FROM t4, t5 WHERE t4.id=t5.id;
INSERT INTO v4 VALUES(1, 'Alice', 20);     -- 应该失败（join 视图无字段列表）
DELETE FROM v4 WHERE id=1;                 -- 应该失败（join 视图）
```

### 测试用例 5: 多表视图（单基表字段）
```sql
CREATE TABLE t6(id INT, name VARCHAR(50));
CREATE TABLE t7(id INT, age INT);
CREATE VIEW v5 AS SELECT t6.id, t6.name, t7.age FROM t6, t7 WHERE t6.id=t7.id;
INSERT INTO v5(id, name) VALUES(1, 'Alice');  -- 应该成功（只涉及 t6）
UPDATE v5 SET name='Bob' WHERE id=1;          -- 应该成功（只更新 t6）
```

### 测试用例 6: 多表视图（多基表字段）
```sql
CREATE TABLE t8(id INT, name VARCHAR(50));
CREATE TABLE t9(id INT, age INT);
CREATE VIEW v6 AS SELECT t8.id, t8.name, t9.age FROM t8, t9 WHERE t8.id=t9.id;
INSERT INTO v6(id, name, age) VALUES(1, 'Alice', 20);  -- 应该失败（涉及 t8 和 t9）
UPDATE v6 SET name='Bob', age=21 WHERE id=1;           -- 应该失败（涉及 t8 和 t9）
```

### 测试用例 7: 聚合视图
```sql
CREATE TABLE t10(id INT, score INT);
CREATE VIEW v7 AS SELECT COUNT(*) AS cnt FROM t10;
INSERT INTO v7 VALUES(10);                 -- 应该失败（聚合视图）
UPDATE v7 SET cnt=10;                      -- 应该失败（聚合视图）
DELETE FROM v7;                            -- 应该失败（聚合视图）
```

## 注意事项

1. **视图初始化**：在进行规则检查时，必须确保视图已经初始化，否则无法访问基表信息
2. **错误码**：目前多表视图的 UPDATE 操作复用了 `RC::JOIN_VIEW_INSERT_ERROR`，可以考虑添加 `RC::JOIN_VIEW_UPDATE_ERROR`
3. **性能**：每次操作都会调用 `init_member()`，可能需要优化以避免重复初始化
4. **嵌套视图**：当前实现未特别处理嵌套视图，但由于嵌套视图的可变性继承自源视图，应该能够正确工作

## 错误码使用

- `RC::READ_ONLY_VIEW_INSERT_ERROR` - 只读视图（聚合、GROUP BY、HAVING）不允许插入
- `RC::READ_ONLY_VIEW_UPDATE_ERROR` - 只读视图不允许更新
- `RC::READ_ONLY_VIEW_DELETE_ERROR` - 只读视图不允许删除
- `RC::JOIN_VIEW_INSERT_ERROR` - join 视图插入错误（无字段列表或字段来自多个基表）
- `RC::JOIN_VIEW_DELETE_ERROR` - join 视图不允许删除
- `RC::EXPRESSION_FIELD_NOT_INSERTABLE` - 表达式字段不允许插入
- `RC::EXPRESSION_FIELD_NOT_UPDATEABLE` - 表达式字段不允许更新










