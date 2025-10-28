# 视图更新规则测试用例总结

## 测试环境说明
- 测试文件：`test_view_update_comprehensive.sql`
- 测试覆盖：INSERT、UPDATE、DELETE 操作
- 视图类型：单表视图、多表视图、表达式视图、聚合视图

## 测试用例详细列表

### 测试1: 多表视图 - INSERT 操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 1.1 | `INSERT INTO create_view_v4 VALUES(120, 120, 'JFDGYF89DA');` | ❌ FAILURE | 多表视图不允许不指定字段列表的插入 |
| 1.2 | `INSERT INTO create_view_v4(id, age) VALUES(175, 175);` | ✅ SUCCESS | id 和 age 都来自同一个基表 t1 |
| 1.3 | `INSERT INTO create_view_v4(name) VALUES('NewName');` | ✅ SUCCESS | name 来自基表 t2 |
| 1.4 | `INSERT INTO create_view_v4(id, name) VALUES(200, 'MultiTable');` | ❌ FAILURE | id 来自 t1，name 来自 t2（跨表） |
| 1.5 | `INSERT INTO create_view_v4(id, age, name) VALUES(300, 30, 'AllFields');` | ❌ FAILURE | 字段来自多个基表 |

### 测试2: 单表视图（完整列）- INSERT 操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 2.1 | `INSERT INTO single_view_v1 VALUES(10, 30, 95);` | ✅ SUCCESS | 单表视图，包含所有列 |
| 2.2 | `INSERT INTO single_view_v1(id, age) VALUES(11, 31);` | ✅ SUCCESS | 单表视图，指定部分列 |

### 测试3: 单表视图（部分列）- INSERT 操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 3.1 | `INSERT INTO single_view_v2 VALUES(20, 40);` | ✅ SUCCESS | 部分列视图，其他列设为 NULL |
| 3.2 | `INSERT INTO single_view_v2(id, age) VALUES(21, 41);` | ✅ SUCCESS | 部分列视图，指定字段列表 |

### 测试4: 单表视图（含表达式）- INSERT 操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 4.1 | `INSERT INTO expr_view_v1 VALUES(30, 50, 140);` | ❌ FAILURE | 视图包含表达式字段 total |
| 4.2 | `INSERT INTO expr_view_v1(id, age) VALUES(31, 51);` | ✅ SUCCESS | 只插入基础列，不涉及表达式列 |
| 4.3 | `INSERT INTO expr_view_v1(id, total) VALUES(32, 100);` | ❌ FAILURE | total 是表达式字段，不可插入 |

### 测试5: 多表视图 - UPDATE 操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 5.1 | `UPDATE update_view_v1 SET age=99 WHERE id1=1;` | ✅ SUCCESS | 只更新 age，来自 t1 |
| 5.2 | `UPDATE update_view_v1 SET age=88, name='NewName' WHERE id1=1;` | ❌ FAILURE | age 来自 t1，name 来自 t2（跨表） |
| 5.3 | `UPDATE update_view_v1 SET id1=100, age=100 WHERE id1=2;` | ✅ SUCCESS | id1 和 age 都来自 t1 |

### 测试6: 表达式视图 - UPDATE 操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 6.1 | `UPDATE update_expr_view SET age=35 WHERE id=1;` | ✅ SUCCESS | 更新基础列 |
| 6.2 | `UPDATE update_expr_view SET double_age=100 WHERE id=1;` | ❌ FAILURE | double_age 是表达式列，不可更新 |
| 6.3 | `UPDATE update_expr_view SET age=40, double_age=80 WHERE id=1;` | ❌ FAILURE | 包含表达式列 double_age |

### 测试7: 多表视图 - DELETE 操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 7.1 | `DELETE FROM delete_view_v1 WHERE id=1;` | ❌ FAILURE | 多表视图不允许删除操作 |

### 测试8: 单表视图 - DELETE 操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 8.1 | `DELETE FROM delete_view_v2 WHERE id=10;` | ✅ SUCCESS | 单表视图允许删除 |
| 8.2 | `DELETE FROM delete_expr_view WHERE id=11;` | ✅ SUCCESS | 即使包含表达式，单表视图也允许删除 |

### 测试9: 聚合视图 - 所有操作

| 测试编号 | SQL 语句 | 期望结果 | 原因 |
|---------|---------|---------|------|
| 9.1 | `INSERT INTO agg_view VALUES(100);` | ❌ FAILURE | 聚合视图只读 |
| 9.2 | `UPDATE agg_view SET cnt=200;` | ❌ FAILURE | 聚合视图只读 |
| 9.3 | `DELETE FROM agg_view;` | ❌ FAILURE | 聚合视图只读 |

## 如何运行测试

### 方法1: 使用 obclient 命令行

```bash
cd build_debug/bin
./observer &
./obclient < ../../test_view_update_comprehensive.sql
```

### 方法2: 使用 obclient 交互模式

```bash
cd build_debug/bin
./observer &
./obclient
```

然后在交互模式中逐条执行 SQL 语句。

## 预期错误消息

根据实现，以下是各种失败情况的错误消息：

1. **多表视图无字段列表插入**：`Can not insert into join view 'db.view_name' without fields list`
2. **多表视图跨表插入**：`Can not insert into join view 'db.view_name' with fields from multiple tables`
3. **表达式字段插入**：`Column 'column_name' is not insertable`
4. **聚合视图插入**：`The target table view_name of the INSERT is not insertable-into`
5. **多表视图跨表更新**：`Can not update join view 'db.view_name' with fields from multiple tables`
6. **表达式字段更新**：`Column 'column_name' is not updateable`
7. **聚合视图更新**：`The target table view_name of the UPDATE is not updatable`
8. **多表视图删除**：`Can not delete from join view 'db.view_name'`
9. **聚合视图删除**：`The target table view_name of the DELETE is not updatable`

## 关键测试点

### ✅ 应该成功的操作
- 单表视图的所有 INSERT/UPDATE/DELETE 操作（除非包含不可变的表达式列）
- 多表视图指定单一基表字段的 INSERT/UPDATE 操作
- 部分列视图的插入（未包含的列设为 NULL）

### ❌ 应该失败的操作
- 多表视图不指定字段列表的 INSERT
- 多表视图跨基表的 INSERT/UPDATE
- 多表视图的所有 DELETE 操作
- 对表达式列的 INSERT/UPDATE 操作
- 聚合视图的所有 INSERT/UPDATE/DELETE 操作

## 测试结果验证

对于每个测试用例，验证：
1. **操作返回码**：成功或失败是否符合预期
2. **错误消息**：失败时的错误消息是否准确
3. **数据一致性**：成功操作后，基表数据是否正确更新
4. **视图数据**：通过视图查询是否能看到正确的结果

## 注意事项

1. 某些测试用例会相互影响，建议按顺序执行
2. 如果某个测试失败，可能会影响后续测试
3. 测试完成后会自动清理所有创建的视图和表
4. 如果测试中断，请手动执行清理部分的 DROP 语句







