# MiniOB 2025 赛题 9～16：从源码到面试

这篇笔记覆盖 unique、group by、简单子查询、alias、NULL、UNION、ORDER BY 和 VECTOR 基础。目标不是背 API，而是能说清楚一条 SQL 如何经过解析、绑定、计划、算子和存储层，以及当前实现在哪些边界上与标准 SQL 有差异。

阅读约定：下文的“当前实现”均指仓库当前源码；`NULL` 的比较、分组、排序等结论包含已实测结果。

## 9. unique：唯一索引

### 题目目标

支持 `CREATE UNIQUE INDEX`、多列唯一索引、插入/更新时的冲突检测，以及 `DROP INDEX`。核心要求是：索引键相同的可见记录不能共存。

### 必要原理

B+ 树普通索引可以让同一个 key 对应多个 RID；唯一性不能仅靠 B+ 树“key 是否存在”判断，因为 MVCC 下同一个 key 可能对应已删除、未提交或对当前事务不可见的版本。因此唯一检测应当是：

1. 用索引 key 找到候选 RID；
2. 读取候选记录；
3. 按当前事务的可见性规则判断是否构成冲突；
4. 有任何可见冲突才拒绝插入。

多列索引键是各列存储字节按顺序拼接而成。这样 `(a,b)` 和 `(b,a)` 是不同键，也不能把“任一列唯一”误当成“组合唯一”。

### 源码执行链

`CREATE UNIQUE INDEX t_i ON t(a, b)` 的语法在 [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y) 中将 `UNIQUE` 写入 `CreateIndexSqlNode::unique`。`CreateIndexStmt::create` 找到表和字段元数据，再创建语句对象；表层的 `Table::create_index` 创建 B+ 树、扫描已有记录并写入索引。

- [IndexMeta::init](../../../src/observer/storage/index/index_meta.cpp) 保存 `unique_` 和每个字段在组合 key 中的偏移。
- [IndexMeta::make_entry_from_record](../../../src/observer/storage/index/index_meta.h) 将多个字段字节拷贝到一个连续 key。
- [BplusTreeIndex::insert_entry](../../../src/observer/storage/index/bplus_tree_index.cpp) 在 `unique()` 时先 `get_entry`，再逐个 RID 做可见性检测。
- [Table::insert_record](../../../src/observer/storage/table/table.cpp) 先插记录、再插所有索引；索引失败后删除已插索引项和记录，做补偿回滚。
- [Table::update_record](../../../src/observer/storage/table/table.cpp) 先删旧键、插新键；插新键失败则恢复旧键；记录页更新失败则删新键、恢复旧键。

MVCC 的更新不是原地改记录：[MvccTrx::update_record](../../../src/observer/storage/trx/mvcc_trx.cpp) 先把旧版本隐藏列 `__trx_xid_end` 标成当前事务，再插入一个新版本。提交时只把版本号由负数改成提交事务号；回滚时删除新版本并恢复旧版本可见性。

### 关键数据结构/算法

- `IndexMeta`：字段列表、总 key 长度、字段 offset、`unique_` 标志。
- B+ 树：`key -> RID 列表`，即使 unique index 也保留 RID 列表，以容纳 MVCC 的旧/新版本。
- MVCC 可见性：`__trx_xid_begin/__trx_xid_end` 放在隐藏字段；`visit_record` 返回成功、不可见或并发写冲突。
- 插入/更新补偿：没有通用 WAL 事务来保证整个“记录页 + 多索引”原子提交，当前主要靠失败后的反向操作补偿。

### SQL 示例

```sql
CREATE TABLE u (a INT, b INT, c CHAR(10));
CREATE UNIQUE INDEX uk_ab ON u(a, b);

INSERT INTO u VALUES (1, 2, 'first');
INSERT INTO u VALUES (1, 2, 'second'); -- RECORD_DUPLICATE_KEY

UPDATE u SET b = 2 WHERE a = 3;         -- 若已有 (3,2)，应失败且保留旧值
DROP INDEX uk_ab ON u;
```

### 复杂度/取舍

单个 B+ 树查找通常是 `O(log_B N + K)`，其中 `K` 是相同 key 的候选 RID 数。普通唯一场景 `K` 很小；MVCC 旧版本积累、未做垃圾回收时，`K` 可能增长。

当前表层写入是“先数据、后索引、失败补偿”，实现直观，但任何补偿失败都可能留下不一致状态。成熟数据库一般会把索引维护、日志和事务恢复设计成一体。

### 实现边界

- 已实测：UNIQUE **允许多个 NULL**。这不是显式实现了常见数据库的“NULL 不等于 NULL”规则，而是 B+ 树 comparator 对带 NULL 标志的字节 key 不满足严格弱序，导致同样的 NULL key 未必被定位为相同 key。不要把这个现象当成正确的 SQL NULL 语义。
- MVCC 下，`BplusTreeIndex::insert_entry` 会忽略别的事务尚未提交的同 key 新版本；如果两个事务同时插入相同唯一键，二者都可能通过检测，而提交阶段没有二次 unique 校验。这是并发唯一性漏洞。
- 题面说不考虑“已有重复数据上建 unique index”，但代码实际会扫描旧记录；若扫描时发现冲突会失败，途中创建的索引文件/内存对象清理并不完全严谨。
- NULL 标志也参与组合 key 的物理比较，且 comparator 不是类型语义比较；不要用字节比较替代 SQL 的 NULL 比较规则。

### 老师追问与参考回答

**问：为什么 unique index 不能简单禁止 B+ 树里重复 key？**

答：MVCC 更新会短时间保留同 key 的旧/新 RID，已删除版本也可能尚未物理回收。是否冲突取决于“对当前事务可见的记录”，所以需要索引定位后再做事务可见性判断。

**问：如何修复两个事务同时插同一唯一键？**

答：需要把“key 不存在/不可见”到“提交”的窗口保护起来，例如 unique-key lock、谓词锁、提交时校验，或更完整的索引并发控制。仅在 B+ 树叶子页加 latch 不能覆盖整个事务生命周期。

**问：多个 NULL 应该怎样处理？**

答：要先定义 SQL 语义并在比较器中显式实现。若采用常见 unique 语义，可让含 NULL 的键不参与重复冲突；若采用 `NULLS NOT DISTINCT`，则让 NULL 在每个对应列上相等。两种都不能依赖未定义的原始字节排序副作用。

## 10. group-by：分组、聚合和 HAVING

### 题目目标

实现 `MAX/MIN/COUNT/AVG/SUM`、单列或多列 `GROUP BY`，并支持在聚合后使用 `HAVING` 过滤。SELECT 中混入未分组普通列和聚合列应报错。

### 必要原理

逻辑执行顺序可记为：`FROM/JOIN -> WHERE -> GROUP BY + aggregate -> HAVING -> ORDER BY -> LIMIT -> SELECT 输出`。其中 WHERE 过滤原始行，HAVING 过滤“每个组产生的一行”。

聚合的 NULL 规则：`COUNT(expr)` 忽略 NULL，`COUNT(*)` 计每行；`SUM/AVG/MIN/MAX` 忽略 NULL；若一个组没有任何非 NULL 输入，后四者应为 NULL。没有 GROUP BY 的空输入仍应产生一个标量聚合结果行。

### 源码执行链

[SelectStmt::create](../../../src/observer/sql/stmt/select_stmt.cpp) 分别绑定 SELECT、GROUP BY、ORDER BY、WHERE 和 HAVING 表达式。[LogicalPlanGenerator::create_single_select_plan](../../../src/observer/sql/optimizer/logical_plan_generator.cpp) 按 `Predicate -> GroupBy -> Having Predicate -> OrderBy -> Limit -> Project` 组装逻辑树。

- [GroupByPhysicalOperator](../../../src/observer/sql/operator/group_by_physical_operator.cpp) 为每个 aggregate expression 创建一个 `Aggregator`。
- [HashGroupByPhysicalOperator::open](../../../src/observer/sql/operator/hash_group_by_physical_operator.cpp) 消耗子算子所有行，找组、累积，再统一 evaluate。
- [ScalarGroupByPhysicalOperator](../../../src/observer/sql/operator/scalar_group_by_physical_operator.cpp) 处理无 GROUP BY 的标量聚合，并专门处理空输入。
- [aggregator.h](../../../src/observer/sql/expr/aggregator.h) 实现 COUNT、AVG、SUM、MAX、MIN 的 NULL 跳过逻辑。

### 关键数据结构/算法

- `GroupValueType = (AggregatorList, CompositeTuple)`：每个组保存聚合器和该组第一条原始 tuple 的副本，再追加聚合结果 tuple。
- group key 使用 `ValueListTuple` 保存多个 group-by 表达式的值。
- `COUNT(*)` 在 binder 被改写为对常量 `1` 的 COUNT，见 [ExpressionBinder](../../../src/observer/sql/parser/expression_binder.cpp)。

名称虽然叫 `HashGroupByPhysicalOperator`，但当前并没有哈希表：`find_group` 逐个扫描 `groups_`，以 tuple compare 判断是否同组。真正的 `StandardAggregateHashTable::add_chunk` 仍是 `exit(-1)` 占位，见 [aggregate_hash_table.cpp](../../../src/observer/sql/expr/aggregate_hash_table.cpp)。

### SQL 示例

```sql
SELECT dept, COUNT(*), AVG(score)
FROM student
WHERE score IS NOT NULL
GROUP BY dept
HAVING COUNT(*) >= 2;

SELECT COUNT(*), SUM(score), AVG(score), MIN(score), MAX(score)
FROM student
WHERE 1 = 0;
-- 0 | NULL | NULL | NULL | NULL
```

### 复杂度/取舍

当前找组是顺序查找。设输入行数为 `N`、组数为 `G`、group key 列数为 `K`，复杂度约 `O(N * G * K)`，最坏退化为 `O(N^2 * K)`；空间约 `O(G * (key + aggregate state + 首行 tuple))`。

真正 hash group by 可将平均时间降到 `O(N*K)`，代价是 hash/equality 必须严格处理类型、NULL、浮点 NaN 与内存增长；大数据还需要 spill 到磁盘或分区聚合。

### 实现边界

- 已实测：`GROUP BY nullable_col` 会把 NULL 与 `0` 混组。原因是 group key 比较最终调用 `Value::compare`，没有先判断 `is_null`；NULL 的底层数值 payload 会参与比较。
- 无 GROUP BY 的空集行为正确：COUNT 为 0，其余聚合为 NULL。带 GROUP BY 的空输入返回零行。
- SELECT 中的未分组字段会被检查；但 HAVING 中的未分组普通字段没有同等的语义检查，可能读取组内“第一行”的值，这是不标准的。
- 同一聚合同时出现在 SELECT 和 HAVING 时，当前可能创建两份聚合器，不会复用结果。

### 老师追问与参考回答

**问：为什么 HAVING 必须在聚合后？**

答：一条原始记录没有 `COUNT(*)`、`AVG(score)` 这样的组级结果；先按组累计才能判断 `HAVING COUNT(*) > 2`。WHERE 则在分组前减少输入行。

**问：如何修复 NULL 与 0 混组？**

答：定义统一的 SQL value equality/hash：先比较 `is_null`，两者皆 NULL 视为同一 group，一方 NULL 一方非 NULL 必不相等；只有两者非 NULL 时才比较类型和值。hash 和 equality 必须一致。

**问：为什么说它不是真正 hash group by？**

答：因为源码 `find_group` 遍历 vector 的每一个已有组，而不是 `unordered_map<key, state>`。名字不决定算法，要看键如何定位状态。

## 11. simple-sub-query：IN、NOT IN 和标量子查询

### 题目目标

支持 `IN/NOT IN`、子查询标量比较、子查询聚合、子查询多行时报错，以及不同类型比较。简单子查询按题意不要求与外层关联，但当前框架已具备相关子查询通路。

### 必要原理

标量子查询只能返回 0 或 1 行：0 行通常转为 NULL，多行是错误。`IN` 是“是否存在相等值”；`NOT IN` 的 NULL 规则最容易出错：若左值没有匹配项、右侧集合含 NULL，则结果为 UNKNOWN，WHERE 中不会输出该行。

相关子查询不能简单只执行一次，因为内层条件依赖每一外层行；常见执行方式是 nested-loop evaluation，性能较差但语义直观。

### 源码执行链

[SubQueryExpr::generate_select_stmt](../../../src/observer/sql/expr/expression.cpp) 调用 `SelectStmt::create` 生成子查询语句，再生成逻辑/物理算子树。[ComparisonExpr::get_value](../../../src/observer/sql/expr/expression.cpp) 每次求条件值时 open 子查询，处理 EXISTS、IN/NOT IN、标量比较，最后 close。

父查询的表 map 被传给子查询，外层 tuple 由 `set_parent_tuple` 递归传至子物理树；扫描和 predicate 用 `JoinedTuple` 组合内外 tuple，见 [PhysicalOperator](../../../src/observer/sql/operator/physical_operator.cpp) 与 [TableScanPhysicalOperator](../../../src/observer/sql/operator/table_scan_physical_operator.cpp)。所以这套设计可表达相关子查询。

### 关键数据结构/算法

- `SubQueryExpr` 持有 `SelectStmt`、逻辑算子和物理算子树。
- `ComparisonExpr` 持有左右表达式和 `CompOp`；对 `IN/NOT IN` 重复向右表达式取值。
- 标量子查询通过先取一行、再 `has_more_row` 检测多行。
- 相关子查询没有缓存、去相关或 semi-join 改写：通常是外层每行重新扫描内层。

### SQL 示例

```sql
SELECT * FROM orders
WHERE customer_id IN (SELECT id FROM customer WHERE vip = 1);

SELECT * FROM employee e
WHERE salary > (SELECT AVG(salary) FROM employee);

SELECT * FROM employee e
WHERE EXISTS (
  SELECT 1 FROM bonus b WHERE b.emp_id = e.id
);
```

### 复杂度/取舍

非相关 `IN` 理论上可先物化右侧集合：构建 `O(M)`，探测 `O(N)` 平均；当前实现仍会对每条外层行迭代右侧结果，近似 `O(N*M)`。

相关子查询天然近似 `O(N * inner_cost)`。优化器可将等值 `EXISTS/IN` 改写为半连接，但要谨慎保留 NULL 和重复值语义。

### 实现边界

- 已实测：`NOT IN (NULL)` 是崩溃级 bug，不只是三值语义不完整。代码只在读取“后续”右值时检查 NULL；第一个右值为 NULL 时，[`ComparisonExpr::get_value`](../../../src/observer/sql/expr/expression.cpp#L307-L318) 直接调用 `left_value.compare(right_value)`，INT 左值随后在 [`IntegerType::compare`](../../../src/observer/common/type/integer_type.cpp#L17-L20) 因右值不是数值而触发断言，observer 进程退出。正确语义应是 UNKNOWN（WHERE 中不返回）。
- 左值 NULL 被直接算作 false；对于 WHERE 过滤效果常等价 UNKNOWN，但没有完整三值逻辑。
- 标量比较会正确拒绝多行子查询，但 IN 允许多行。
- 子查询 open 时传入 `nullptr` 事务指针；MVCC 扫描遇到空事务指针会跳过可见性判断，见 [RecordFileScanner](../../../src/observer/storage/record/record_manager.cpp)。因此 MVCC 子查询可能读到未提交数据。
- 内外层允许重名 alias 的需求与 map 合并方式冲突：内层 map 用 `insert` 合并，不能覆盖父层同名 key，内层别名可能误绑定外表。

### 老师追问与参考回答

**问：为什么 NOT IN 比 NOT EXISTS 更难？**

答：NOT EXISTS 只关心有没有行；NOT IN 还要考虑右集合中的 NULL。`x NOT IN (1, NULL)` 即使 x 不是 1，也不能返回 true，因为无法证明 x 不等于未知值。

**问：如何修复 NOT IN？**

答：扫描右侧时维护 `matched` 和 `has_null`：有匹配则 false；无匹配但有 NULL 则 UNKNOWN；两者都没有才 true。执行器若只服务 WHERE，可把 UNKNOWN 当过滤；若表达式可投影，还需保留三值 Boolean。

**问：相关子查询怎样优化？**

答：先判断能否改写为 semi-join/anti-join；非相关子查询可物化并 hash；相关的等值条件可利用内层索引。优化时不能丢掉 NOT IN 的 NULL 语义和重复语义。

## 12. alias：表别名和列别名

### 题目目标

支持 `table AS t`、`column AS c`；同层表别名不能重复，内外层可重复；表别名用于字段引用、join、运算和条件，列别名仅用于结果显示。

### 必要原理

表别名是名字解析作用域的一部分，必须绑定到“本层 FROM 的某一个表实例”。同一张物理表在自连接中出现两次时，两个 alias 才能区分两个 tuple。列别名则是投影结果的显示名称，不应反过来改变 WHERE 的字段解析。

### 源码执行链

[SelectStmt::create](../../../src/observer/sql/stmt/select_stmt.cpp) 收集关系和 alias，用 `temp_map.emplace` 检查当前层别名重复，并将 alias 交给 `BinderContext`。[ExpressionBinder::bind_unbound_field_expression](../../../src/observer/sql/parser/expression_binder.cpp) 用表名/alias 查表，生成携带 table alias 的 `FieldExpr`。

物理计划为 `TableGetLogicalOperator` 保存 alias；[TableScanPhysicalOperator](../../../src/observer/sql/operator/table_scan_physical_operator.h) 和索引扫描构造时把 alias 写进 `RowTuple`；[RowTuple::find_cell](../../../src/observer/sql/expr/tuple.h) 再以 alias 过滤字段查找。输出 schema 来自 Project 的 expression name，见 [ProjectTuple](../../../src/observer/sql/expr/tuple.h)。

### 关键数据结构/算法

- `BinderContext::tables_`：本层可见的 `名字 -> BaseTable*` map。
- `tables_alias_`：FROM 各表实例的 alias，和物理 scan 顺序对应。
- `FieldExpr`：同时保存底层真实表名、字段名、可选 alias；真实表名用于定位字段，alias 用于区分表实例。

### SQL 示例

```sql
SELECT e.name AS employee, m.name AS manager
FROM employee AS e, employee AS m
WHERE e.manager_id = m.id;

SELECT t.id AS order_id
FROM orders AS t;
```

### 复杂度/取舍

绑定阶段的表/alias 查找为 hash map 平均 `O(1)`。运行阶段 alias 比较发生在 tuple 字段查找中，通常只影响表达式求值常数项；自连接真正的代价来自 join，而不是 alias 本身。

### 实现边界

- 当前层 duplicate alias 能返回 `INVALID_ALIAS`，但子查询内外重名 alias 的遮蔽不可靠，原因见第 11 的父/子 map `insert` 合并。
- `FROM t AS x` 后，当前 map 只登记 `x`，不登记 `t`；后续必须用 `x.c`，这一点接近常见 SQL 规则。
- 多表查询里未限定字段没有完善的“歧义列”检测；默认表为空时 binder 可能对空指针取字段元数据，而非返回清晰错误。
- 输出表头由 [`ProjectPhysicalOperator::tuple_schema`](../../../src/observer/sql/operator/project_physical_operator.cpp#L68-L78) 构造：有 alias 时使用 alias，否则使用 `expression->name()`。因此无 alias 的原字段名/表达式名也会正常输出，展示层在这一点是完整的。

### 老师追问与参考回答

**问：为什么 alias 不能只在打印时替换字符串？**

答：因为 `e.id = m.id` 的条件求值必须区分两个 employee 实例。alias 必须进入 binder、FieldExpr、scan tuple 和字段查找链路，只有打印替换无法支持自连接。

**问：列 alias 为什么通常不能在 WHERE 使用？**

答：WHERE 的逻辑执行早于 SELECT 投影，列 alias 尚未产生。若数据库允许某些场景使用 alias，通常是 ORDER BY 的额外解析规则，而不是把 alias 当基础列。

## 13. null：存储、运算和逻辑

### 题目目标

支持 `NULL/NOT NULL` 列定义、插入/更新 NULL、`IS NULL/IS NOT NULL`、NULL 算术与比较，并使聚合正确跳过 NULL。

### 必要原理

NULL 不是 0、空串或缺失字节，而是“未知”。标准 SQL 使用三值逻辑：TRUE/FALSE/UNKNOWN。`NULL = 1`、`NULL <> 1` 和 `NULL = NULL` 都是 UNKNOWN；WHERE 只保留 TRUE。判断 NULL 要使用 `IS NULL`。

### 源码执行链

DDL 的 nullable 解析位于 [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y)。可空列的物理长度多一个字节，最后一字节为 `'1'` 表示 NULL。

- [BaseTable::make_record](../../../src/observer/storage/table/base_table.cpp) 写入时检查 NOT NULL，并写 NULL 标志。
- [RowTuple::cell_at](../../../src/observer/sql/expr/tuple.h) 和 [Record::get_field](../../../src/observer/storage/record/record.h) 读取末尾标志，恢复 `Value::is_null`。
- [ComparisonExpr::compare_value](../../../src/observer/sql/expr/expression.cpp) 特判 `IS/IS NOT`，其余一侧 NULL 时返回 false。
- [ArithmeticExpr::calc_value](../../../src/observer/sql/expr/expression.cpp) 一侧 NULL 时产生 NULL。
- 聚合器的 NULL 跳过逻辑见第 10。

### 关键数据结构/算法

- `FieldMeta::nullable_`：列定义层的可空性。
- `Value::is_null_`：执行层的 NULL 状态；注意它与 `attr_type_` 分离，NULL 值仍可能保留原始列类型。
- 行格式：字段数据区末尾的一字节 NULL marker，而不是独立 bitmap。

### SQL 示例

```sql
CREATE TABLE n (id INT NULL, name CHAR(10) NOT NULL);
INSERT INTO n VALUES (NULL, 'alice');

SELECT * FROM n WHERE id IS NULL;
SELECT * FROM n WHERE id = NULL;       -- 不返回行
SELECT id + 1 FROM n;                  -- NULL
SELECT COUNT(id), COUNT(*) FROM n;     -- 0 | 1
```

### 复杂度/取舍

每个 nullable 列多用 1 byte，读写 NULL 判断 `O(1)`，实现简单但比 bitmap 浪费空间。bitmap 更紧凑，但需要计算位偏移、处理 schema 演进和批量执行。

### 实现边界

- 基础存储、IS 判断、算术传播、聚合忽略 NULL 已具备。
- 当前谓词实现将 NULL 比较直接压成 false，Conjunction 也只处理二值 Boolean；对 WHERE 的过滤结果常可接受，但不是完整三值逻辑。
- `Value::compare` 不统一判断 `is_null`，所以不能安全地作为 GROUP BY、UNION DISTINCT、ORDER BY 的 SQL NULL comparator。这正是 NULL 与 0 混组等问题的根源。

### 老师追问与参考回答

**问：为什么 `col = NULL` 不等价于 `col IS NULL`？**

答：`=` 是值比较，NULL 表示未知，未知与任何值比较结果仍未知；`IS NULL` 是状态判断，结果才是确定的 TRUE/FALSE。

**问：如何设计统一的 NULL 比较接口？**

答：分两个接口。谓词比较返回三值逻辑；排序/分组/去重比较返回确定的总序或等价关系，并明确两 NULL 是否相等、NULL 排前还是排后。不能让一个普通 `compare` 同时承担所有语义。

## 14. union：集合合并与去重

### 题目目标

支持多个 SELECT 的 `UNION`（整行去重）和 `UNION ALL`（不去重），从左到右结合，要求列数和对应类型兼容。

### 必要原理

`UNION ALL` 是拼接两个结果流；`UNION` 是拼接后按**整行**做 distinct。去重不是只看第一列，也不是只对两个分支间去重：同一个分支自身的重复行也应被消除。`A UNION B UNION ALL C` 的语义是 `(A UNION B) UNION ALL C`。

### 源码执行链

[SelectStmt::create](../../../src/observer/sql/stmt/select_stmt.cpp) 递归创建 set-operation 的子 SELECT，并检查列数。[LogicalPlanGenerator::create_plan](../../../src/observer/sql/optimizer/logical_plan_generator.cpp) 以累计左树方式逐个构造 `UnionLogicalOperator`，保证左结合。[PhysicalPlanGenerator](../../../src/observer/sql/optimizer/physical_plan_generator.cpp) 生成 `UnionPhysicalOperator`。

[UnionPhysicalOperator](../../../src/observer/sql/operator/union_physical_operator.cpp) 按左子树再右子树拉取 tuple；ALL 直接返回，DISTINCT 则将整行转为 `TupleDistinctKey{vector<Value>}` 并插入 `unordered_set`。

### 关键数据结构/算法

- `TupleDistinctKey`：整行 Value 数组。
- `TupleDistinctKeyHash`：按 Value 类型和值组合 hash，支持 vector 元素 hash。
- `seen_keys_`：已输出的 DISTINCT 行集合。
- `expected_cell_num_`：运行期再次检查分支列数。

### SQL 示例

```sql
SELECT id, name FROM a
UNION
SELECT id, name FROM b
UNION ALL
SELECT id, name FROM c;

-- UNION 会从 a、b 两支的所有重复行中只保留一行；c 的重复保持。
```

### 复杂度/取舍

UNION ALL 额外空间接近 `O(1)`，时间 `O(N)`。哈希 UNION DISTINCT 平均时间 `O(N * C)`，空间 `O(U * C)`，其中 `C` 是每行列/值处理成本、`U` 是不同结果行数。它能流式输出首次出现的行，但 `seen_keys_` 必须一直保留到结束。

排序去重可减少 hash 内存峰值、方便外排，但需要 `O(N log N)` 且会改变原始输出顺序。

### 实现边界

- 当前只在 statement/执行阶段检查列数，没有验证各位置类型相同；题面保证类型匹配时通常不会暴露。
- 已实测/审查风险：浮点 `Value::compare` 的相等关系与 `std::hash<float>` 的 hash 关系在 `-0/+0`、NaN 等情况下不一定满足“相等对象必须同 hash”。`unordered_set` 因而有去重不稳定风险。
- NULL hash 单独混入常量，但 equality 仍调用 `Value::compare`，未先按 `is_null` 建模。NULL 行去重正确性依赖底层 payload，不能视为严格 SQL 语义。
- DISTINCT 使用内存 `seen_keys_`，没有 spill；结果很大时会占用大量内存。

### 老师追问与参考回答

**问：为什么 UNION 去重要按整行？**

答：SQL 的集合元素是一条结果 tuple。例如 `(1,'a')` 与 `(1,'b')` 不重复；仅按 id 去重会把合法结果丢掉。

**问：hash/equality 为什么必须一致？**

答：哈希容器先以 hash 定位桶，再调用 equality。若两个“相等”对象落到不同桶，容器不会比较它们，就会留下重复项。浮点和 NULL 需要专门定义规范化规则。

## 15. order-by：多列排序与排序语义

### 题目目标

第 15 题的题面目标是支持一列或多列 `ORDER BY`、每列 ASC/DESC 和默认 ASC。`LIMIT` 组合可作为执行计划的相关边界讨论，但不是本题题面要求；外部排序属于第 24 题 `big-order-by` 的范围。

### 必要原理

多键排序是字典序：先比较第一键，只有相等才比较下一键。LIMIT 必须在排序之后，`ORDER BY score DESC LIMIT 10` 才能得到最高的十条。排序还需要预先定义 NULL 顺序（NULLS FIRST 或 NULLS LAST）和同 key 时的稳定性策略。

### 源码执行链

逻辑计划先放 ORDER BY，再把 LIMIT 包在其上，见 [LogicalPlanGenerator](../../../src/observer/sql/optimizer/logical_plan_generator.cpp)。[OrderByPhysicalOperator](../../../src/observer/sql/operator/order_by_physical_operator.cpp) 在 `open` 中打开子算子，估算规模后选择内存排序或 `ExternalSorter`。

内存路径把每行复制为 `ValueListTuple`，保存 order keys 和 sequence，再 `stable_sort`。外排路径由 [ExternalSorter](../../../src/observer/sql/operator/external_sort/external_sorter.cpp) 分批生成排序 run 文件、打开所有 run reader，并用优先队列多路归并。tuple 的字段、schema 和 order keys 由 [TupleSerializer](../../../src/observer/sql/operator/external_sort/tuple_serializer.cpp) 序列化。

### 关键数据结构/算法

- 内存 `OrderEntry{keys, tuple, sequence}`。
- 外排 `buffer_`、`run_files_`、`RunReader` 和 merge min-heap。
- `LimitPhysicalOperator` 仅维护 `pos_` 和 `limit_`，见 [limit_physical_operator.h](../../../src/observer/sql/operator/limit_physical_operator.h)。
- 当前选择器用固定估算：表扫描约 1000 行、索引扫描约 500 行，阈值固定 50 MB。

### SQL 示例

```sql
SELECT id, score
FROM exam
ORDER BY score DESC, id ASC
LIMIT 10;
```

### 复杂度/取舍

内存排序时间 `O(N log N)`、空间 `O(N * row_size)`，简单且通常最快。外排仍约 `O(N log N)` 比较，但增加读写 `O(N)` 级磁盘 I/O；其内存可受 run buffer 控制，代价是临时文件和归并管理。

当前外排每个 run 最多 1000 tuple，即使 5 MB buffer 未满也会 flush；大输入会产生许多 run，并在归并时同时打开所有文件，存在文件描述符耗尽风险。

### 实现边界

- NULL 没有显式排序策略，key 比较直接走 `Value::compare`。因此 NULL 顺序不可靠，也会受 NULL payload 影响。
- 以源码为准：内存路径虽调用 `stable_sort`，但当 ORDER keys 相等时继续比较整行，最后才看 sequence；不同记录但同 key 不保证保持输入顺序，因而这不是真正的稳定 ORDER BY。当前预编译二进制的小样本实测中同 key 行恰好保持了输入序，但这只是观测结果，不是实现保证。
- 外排路径用 `std::sort`，且平局时用 tuple 指针地址打破平局，跨 run 更不稳定。
- `LimitPhysicalOperator::open` 和 `close` 不重置 `pos_`；同一物理计划对象重复 open 时，LIMIT 可能立即 EOF 或少返回行。
- 外排切换依据估算而非实际内存计数。真实大单表会被估成 1000 行，仍可能误走内存排序并造成内存压力。
- 临时文件名是 `/tmp/run_pid_counter.tmp`，非原子创建且全局计数无并发保护，见 [TempFileManager](../../../src/observer/sql/operator/external_sort/temp_file_manager.cpp)。

### 老师追问与参考回答

**问：为什么 LIMIT 要放在 ORDER BY 之后？**

答：先 LIMIT 会得到“输入流前十条”再排序，不是“全表最高十条”。除非优化器能证明使用索引有序扫描或 top-K 算法等价，否则逻辑上必须先排序。

**问：如何实现稳定的外部排序？**

答：为每行保存全局输入序号，run 内和 merge heap 的比较器在所有排序键相等时只比较该序号；不能使用指针地址。这样跨 run 仍保持同 key 的输入顺序。

**问：如何更可靠地决定外排？**

答：优先使用统计信息和实际累计内存。开始可在内存中收集，达到预算就 flush 成 run，避免完全依赖“表扫描固定 1000 行”之类的启发式。

## 16. vector-basic：向量类型、转换和距离

### 题目目标

支持 `VECTOR(n)` 列、插入向量、`STRING_TO_VECTOR`、`VECTOR_TO_STRING` 和 `DISTANCE(v1,v2,'COSINE'/'EUCLIDEAN'/'DOT')`。

### 必要原理

向量在行内通常存为连续 float 数组；列维度 `n` 是 schema 属性，插入时必须验证输入向量维度一致。距离计算要先验证两向量长度相同：

- 欧氏距离：`sqrt(sum((a_i-b_i)^2))`；
- cosine distance：`1 - dot(a,b) / (|a|*|b|)`；
- dot：`sum(a_i*b_i)`。

向量转字符串需要固定且可测试的格式；题目示例要求科学计数法、五位小数。

### 源码执行链

`VECTOR(n)` 在 [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y) 解析成 `n * sizeof(float)` 的字段长度；nullable 再额外加 NULL 标志字节。建表时 [Table::create](../../../src/observer/storage/table/table.cpp) 限制最大约 16000 维。

插入时 [BaseTable::set_value_to_record](../../../src/observer/storage/table/base_table.cpp) 比较列维度和 value 的 float 个数，长度不一致返回 `VECTOR_DIM_MISMATCH`。读取向量时 [Record::get_field](../../../src/observer/storage/record/record.h) 以 float 数组构造 Value。

`DISTANCE` 在 [ExpressionBinder](../../../src/observer/sql/parser/expression_binder.cpp) 校验三个参数、把字面 metric 映射为内部函数类型，再去掉第三参数；[NormalFunctionExpr](../../../src/observer/sql/expr/expression.cpp) 调 [builtin.cpp](../../../src/observer/sql/builtin/builtin.cpp) 计算。`VECTOR_TO_STRING` 最终由 [VectorType::to_string](../../../src/observer/common/type/vector_type.cpp) 输出科学计数法。

### 关键数据结构/算法

- `Value`：`AttrType::VECTORS` 时持有 `float*`、字节长度、所有权标志，见 [value.h](../../../src/observer/common/value.h)。
- `VectorType`：负责 vector compare、cast、序列化和向量四则运算。
- `parse_vector_from_string`：方括号字符串按逗号切分并解析 float，见 [utils.cpp](../../../src/observer/common/utils.cpp)。
- 运行时距离函数先 cast 两参数为 VECTORS，比较维度，再线性循环计算。

### SQL 示例

```sql
CREATE TABLE vec_test (id INT, embedding VECTOR(3));
INSERT INTO vec_test VALUES (1, STRING_TO_VECTOR('[1, 2, 3]'));

SELECT VECTOR_TO_STRING(embedding) AS vec,
       DISTANCE(embedding, STRING_TO_VECTOR('[1, 2, 3]'), 'EUCLIDEAN') AS d
FROM vec_test;

SELECT DISTANCE(STRING_TO_VECTOR('[1,2]'),
                STRING_TO_VECTOR('[2,3]'), 'COSINE');
```

### 复杂度/取舍

行内存储的读写空间是 `O(d)`，任一精确距离计算是 `O(d)` 时间、`O(1)` 额外计算空间。它适合本题的基础功能和后续精确检索；大规模向量检索再需要 IVF/HNSW 等近似索引来避免 `O(rows * d)` 全扫描。

当前 `Value` 对 vector 使用堆内存和深拷贝，生命周期安全较直观，但排序、UNION、外排中大量复制向量会产生明显内存与分配成本。

### 实现边界

- 空向量 `[]` 被解析器拒绝；字符串必须以 `[` 开始、`]` 结束。但 token 只要求能读出一个 float，类似 `[1x]` 可能被接受，缺少“完整 token 消费”、有限值检查和严格错误报告。
- 表列长度与插入 vector 长度不一致会报 `VECTOR_DIM_MISMATCH`；两个距离参数长度不一致返回 `VECTOR_LENGTG_ARE_INCONSISTENT`。
- `STRING_TO_VECTOR(NULL)` 与 `VECTOR_TO_STRING(NULL)` 返回 NULL；距离任一参数 NULL 也返回 NULL。
- cosine 距离的查询实现遇零向量返回 NULL；向量索引辅助版本遇零分母返回 `1.0`。二者语义不一致，后续若用索引重写 ORDER BY distance，可能得到不同结果。
- `Value(const char*)` 会尝试把形如 `[... ]` 的任意字符串自动当 vector 解析，见 [value.cpp](../../../src/observer/common/value.cpp)。这让字符串字面量和向量字面量类型推断耦合，不是严格的 SQL 类型系统设计。
- 向量输出使用 scientific、precision(5)、逗号无空格，符合题目示例，但浮点舍入和 NaN/Inf 格式没有额外规范。

### 老师追问与参考回答

**问：为什么插入和距离计算都要检查维度？**

答：列定义保证数据模型一致，距离定义保证逐维运算有数学意义。只在计算时发现长度不同会把错误数据延迟到查询期，也会破坏索引构建。

**问：DOT 为什么有时不适合直接叫“距离”？**

答：点积越大通常表示越相似，不满足距离的非负、对称、三角不等式等性质。若用于“最近优先”的 ORDER BY/向量索引，要明确是按点积升序、降序，还是转换成负内积。

**问：如何让字符串解析更健壮？**

答：用有限状态机或严格 token parser：允许空白但要求每个 token 被完整消费，拒绝空 token、尾随垃圾、NaN/Inf（若产品不允许），限制最大维度，并在失败时释放已分配内存。

## 复习主线

这八题反复出现同一条工程主线：**语义先统一，再让存储、比较器、hash、算子和事务共享这套语义**。当前代码中最值得面试时主动指出的缺口，正是 NULL/浮点比较器不统一、MVCC unique 提交窗口、NOT IN 的三值逻辑、名为 HashGroupBy 的顺序查找，以及外排/Limit 的状态管理。
