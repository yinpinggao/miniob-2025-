# 24 题源码索引与实测证据

## 1. 公共入口索引

| 层次 | 入口源码 | 作用 |
|---|---|---|
| SQL 请求 | [sql_task_handler.cpp](../../../src/observer/net/sql_task_handler.cpp#L21) | `handle_event` 读取请求，`handle_sql` 调度 SQL 各阶段 |
| Parse | [parse_stage.cpp](../../../src/observer/sql/parser/parse_stage.cpp#L30) | SQL 字符串转 ParsedSqlNode |
| Resolve | [resolve_stage.cpp](../../../src/observer/sql/parser/resolve_stage.cpp#L31) | 创建并绑定具体 Stmt |
| Optimize | [optimize_stage.cpp](../../../src/observer/sql/optimizer/optimize_stage.cpp#L32) | 逻辑/物理计划与改写 |
| Execute | [execute_stage.cpp](../../../src/observer/sql/executor/execute_stage.cpp#L32) | DDL 等直接执行入口 |
| Result | [sql_result.cpp](../../../src/observer/sql/executor/sql_result.cpp#L25) | 驱动查询算子树 |
| Logical Plan | [logical_plan_generator.cpp](../../../src/observer/sql/optimizer/logical_plan_generator.cpp) | 生成关系代数逻辑树 |
| Physical Plan | [physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp) | 选择具体执行算法 |

## 2. 24 题关键源码速查

| 题号 | 赛题 | 建议优先阅读的文件 |
|---:|---|---|
| 1 | basic | [logical_plan_generator.cpp](../../../src/observer/sql/optimizer/logical_plan_generator.cpp)、[physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp)、[table.cpp](../../../src/observer/storage/table/table.cpp) |
| 2 | update | [update_stmt.cpp](../../../src/observer/sql/stmt/update_stmt.cpp#L27)、[update_physical_operator.cpp](../../../src/observer/sql/operator/update_physical_operator.cpp#L16) |
| 3 | drop-table | [drop_table_executor.cpp](../../../src/observer/sql/executor/drop_table_executor.cpp)、[db.cpp](../../../src/observer/storage/db/db.cpp)、[table.cpp](../../../src/observer/storage/table/table.cpp) |
| 4 | date | [utils.cpp](../../../src/observer/common/utils.cpp#L31)、[char_type.cpp](../../../src/observer/common/type/char_type.cpp#L33)、[date_type.cpp](../../../src/observer/common/type/date_type.cpp#L20) |
| 5 | join-tables | [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y)、[logical_plan_generator.cpp](../../../src/observer/sql/optimizer/logical_plan_generator.cpp#L328)、[join_physical_operator.cpp](../../../src/observer/sql/operator/join_physical_operator.cpp#L20) |
| 6 | expression | [expression.cpp](../../../src/observer/sql/expr/expression.cpp)、[expression_binder.cpp](../../../src/observer/sql/parser/expression_binder.cpp)、[arithmetic_operator.hpp](../../../src/observer/sql/expr/arithmetic_operator.hpp) |
| 7 | function | [builtin.cpp](../../../src/observer/sql/builtin/builtin.cpp)、[expression_binder.cpp](../../../src/observer/sql/parser/expression_binder.cpp) |
| 8 | multi-index | [index_meta.h](../../../src/observer/storage/index/index_meta.h#L51)、[bplus_tree.h](../../../src/observer/storage/index/bplus_tree.h#L93)、[index_scan_physical_operator.cpp](../../../src/observer/sql/operator/index_scan_physical_operator.cpp) |
| 9 | unique | [bplus_tree_index.cpp](../../../src/observer/storage/index/bplus_tree_index.cpp)、[index_meta.cpp](../../../src/observer/storage/index/index_meta.cpp)、[mvcc_trx.cpp](../../../src/observer/storage/trx/mvcc_trx.cpp) |
| 10 | group-by | [aggregator.h](../../../src/observer/sql/expr/aggregator.h)、[hash_group_by_physical_operator.cpp](../../../src/observer/sql/operator/hash_group_by_physical_operator.cpp)、[scalar_group_by_physical_operator.cpp](../../../src/observer/sql/operator/scalar_group_by_physical_operator.cpp) |
| 11 | simple-sub-query | [expression.cpp](../../../src/observer/sql/expr/expression.cpp)、[expression_binder.cpp](../../../src/observer/sql/parser/expression_binder.cpp)、[physical_operator.cpp](../../../src/observer/sql/operator/physical_operator.cpp) |
| 12 | alias | [select_stmt.cpp](../../../src/observer/sql/stmt/select_stmt.cpp)、[tuple.h](../../../src/observer/sql/expr/tuple.h)、[plain_communicator.cpp](../../../src/observer/net/plain_communicator.cpp) |
| 13 | null | [base_table.cpp](../../../src/observer/storage/table/base_table.cpp)、[value.cpp](../../../src/observer/common/value.cpp)、[expression.cpp](../../../src/observer/sql/expr/expression.cpp) |
| 14 | union | [union_physical_operator.cpp](../../../src/observer/sql/operator/union_physical_operator.cpp)、[logical_plan_generator.cpp](../../../src/observer/sql/optimizer/logical_plan_generator.cpp) |
| 15 | order-by | [order_by_physical_operator.cpp](../../../src/observer/sql/operator/order_by_physical_operator.cpp)、[limit_physical_operator.cpp](../../../src/observer/sql/operator/limit_physical_operator.cpp) |
| 16 | vector-basic | [vector_type.cpp](../../../src/observer/common/type/vector_type.cpp)、[builtin.cpp](../../../src/observer/sql/builtin/builtin.cpp)、[base_table.cpp](../../../src/observer/storage/table/base_table.cpp) |
| 17 | text | [page.h](../../../src/observer/storage/buffer/page.h#L26)、[base_table.cpp](../../../src/observer/storage/table/base_table.cpp)、[text_type.cpp](../../../src/observer/common/type/text_type.cpp) |
| 18 | vector-search | [vector_index_scan_rewrite.cpp](../../../src/observer/sql/optimizer/vector_index_scan_rewrite.cpp)、[vector_scan_physical_operator.cpp](../../../src/observer/sql/operator/vector_scan_physical_operator.cpp)、[ivfflat_index.cpp](../../../src/observer/storage/index/ivfflat_index.cpp) |
| 19 | alter | [alter_table_stmt.cpp](../../../src/observer/sql/stmt/alter_table_stmt.cpp)、[alter_table_executor.cpp](../../../src/observer/sql/executor/alter_table_executor.cpp)、[table.cpp](../../../src/observer/storage/table/table.cpp#L701) |
| 20 | update-mvcc | [mvcc_trx.cpp](../../../src/observer/storage/trx/mvcc_trx.cpp#L201)、[mvcc_trx_log.cpp](../../../src/observer/storage/trx/mvcc_trx_log.cpp) |
| 21 | complex-sub-query | [expression.cpp](../../../src/observer/sql/expr/expression.cpp)、[select_stmt.cpp](../../../src/observer/sql/stmt/select_stmt.cpp)、[table_scan_physical_operator.cpp](../../../src/observer/sql/operator/table_scan_physical_operator.cpp) |
| 22 | create-view | [create_view_stmt.cpp](../../../src/observer/sql/stmt/create_view_stmt.cpp)、[view.cpp](../../../src/observer/storage/table/view.cpp)、[view_scan_physical_operator.cpp](../../../src/observer/sql/operator/view_scan_physical_operator.cpp) |
| 23 | full-text-index | [fulltext_index.cpp](../../../src/observer/storage/index/fulltext_index.cpp#L170)、[jieba_util.cpp](../../../src/observer/common/fulltext/jieba_util.cpp#L72)、[expression.cpp](../../../src/observer/sql/expr/expression.cpp#L978) |
| 24 | big-order-by | [external_sorter.cpp](../../../src/observer/sql/operator/external_sort/external_sorter.cpp#L27)、[grace_hash_join_physical_operator.cpp](../../../src/observer/sql/operator/grace_hash_join_physical_operator.cpp)、[physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L389) |

## 3. 构建与测试状态

### 完整构建

当前源码已经通过完整构建：

```text
cmake --build build_debug -j2
```

### C++ 单元测试

第一次测试受到容器内 LeakSanitizer 环境限制。设置：

```text
ASAN_OPTIONS=detect_leaks=0
```

后，确认 27 项测试通过。最后一个长耗时日志测试等待较久后被终止，因此不能写成“28/28 全部通过”，也不能据此断言最后一项失败。

### SQL Harness

已在当前容器重试完整 SQL harness，并分别在受限沙箱内外执行。两次都在进入测试套件前停于 [`miniob_test.py`](../../../test/case/miniob_test.py#L1113) 的 `os.setpgrp()`，返回 `PermissionError: [Errno 1] Operation not permitted`。因为脚本在创建 server/socket 之前就退出，本次不能再把 Unix/TCP socket 权限写成已确认的当前阻塞。

因此仍使用 `observer -P cli` 做针对性 SQL 验证。测试数据位于 `/tmp`，没有修改项目源码和仓库数据。

## 4. 代表性实测证据

### 4.1 UPDATE 表达式没有逐行求值

```sql
create table t_update(id int, c int);
insert into t_update values (1,10),(2,20),(3,30);
update t_update set c=c+1;
select * from t_update;
```

实际结果：

```text
1 | 11
2 | 11
3 | 11
```

根因是赋值表达式基于 `records_.front()` 求值一次。

### 4.2 DATE 接受尾随垃圾字符

```sql
insert into t_date values ('2024-02-29abc');
```

被接受并输出：

```text
2024-02-29
```

说明日期主体校验存在，但解析没有验证字符串完全消费。

### 4.3 INNER JOIN 混合语法不支持

```sql
SELECT *
FROM j1 INNER JOIN j2 ON j1.id=j2.id, j3
WHERE j3.id=j1.id;
```

结果：

```text
unexpected COMMA
```

### 4.4 INNER JOIN 表别名不支持

```sql
SELECT * FROM j1 x INNER JOIN j2 y ON x.id=y.id;
```

结果：

```text
unexpected INNER
```

### 4.5 复合索引没有被查询计划使用

创建复合索引后，对匹配复合条件执行 `EXPLAIN`，物理计划仍显示：

```text
TABLE_SCAN
```

证明当前只完成了索引的创建、持久化和 DML 维护，没有完成查询访问路径。

### 4.6 UNIQUE 允许多个 NULL，但实现基础不可靠

```sql
insert into t_unique values (1,null);
insert into t_unique values (2,null);
```

两条均成功。该结果不能直接解释为正确实现了 SQL 的多个 NULL 规则，因为 B+Tree comparator 对 NULL 的比较不满足严格弱序。

### 4.7 GROUP BY 把 NULL 与 0 混组

```sql
insert into t_null values (1,null),(2,null),(3,0);
select g,count(*) from t_null group by g;
```

实际结果：

```text
NULL | 3
```

正确结果应有 NULL 组和 0 组两组。

### 4.8 NOT IN(NULL) 崩溃已修复

```sql
select id
from t_update
where id not in (select g from t_null where id=1);
```

修复前，右侧首个值为 NULL 会在 `IntegerType::compare` 触发 `right type is not numeric` 断言，进程以 134 退出。修复后 `id NOT IN (NULL)` 输出空结果且 observer 继续运行；`id IN (NULL, 1)` 正常匹配 1。单元素 RHS 也有独立回归用例，避免对不可迭代的常量表达式反复求值。

### 4.9 TEXT 是定长内联

题面 TEXT 被解析为 65535 字节；项目 Page Size 为 128 KiB，所以记录可以放入一页，但短文本仍占用完整预留空间，且通常每页只能容纳一条带 TEXT 的记录。

### 4.10 全文 DELETE 后统计没有正确维护

针对性验证中，删除全文文档前后，保留文档的 score 没有按预期变化。结合源码可确认普通 DELETE 没有完整执行 `remove_document`，BM25 的 N、df、平均文档长度会失真。

## 5. 如何使用这份索引追源码

建议每次只追一条 SQL，不要从头到尾阅读整个文件。例如分析 UPDATE：

```text
1. 在 yacc_sql.y 搜 UPDATE grammar
2. 打开 UpdateStmt::create 看字段和表达式绑定
3. 在 LogicalPlanGenerator 搜 create_plan(UpdateStmt)
4. 在 PhysicalPlanGenerator 找 UpdatePhysicalOperator
5. 阅读 UpdatePhysicalOperator::open
6. 继续追 Trx::update_record
7. 区分 VacuousTrx 与 MvccTrx
8. 最后看 Table、RecordManager、Index 的物理修改
```

分析其他题也应遵循：

```text
语法 → Stmt/Binder → 逻辑计划 → 物理算子 → Trx → Table/Index/Record
```

如果某一层没有对应实现，就要判断它是否走了另一条直接执行路径，或者只是“代码存在但从未进入计划”。
