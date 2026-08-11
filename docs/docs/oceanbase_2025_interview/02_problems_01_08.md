# MiniOB-2025 赛题 1–8 源码与面试笔记

本文面向第一次接触数据库的同学。每道题都从“SQL 写出来后，系统怎样把它变成结果”开始说明。源码链接均从本目录出发；链接中的行号以当前源码为准。

## 先建立一条总路线

绝大多数 SQL 都经过下面的阶段：

```text
客户端请求
  -> SessionStage
  -> ParseStage：词法/语法树
  -> ResolveStage：表、列、表达式绑定
  -> OptimizeStage：逻辑计划、改写、物理计划
  -> ExecuteStage：命令执行器或物理算子
  -> SqlResult：open/next/close，事务提交或回滚
```

- [session_stage.cpp](../../../src/observer/session/session_stage.cpp#L80) 串起 query cache、parse、resolve、optimize、execute。
- [optimize_stage.cpp](../../../src/observer/sql/optimizer/optimize_stage.cpp#L32) 负责逻辑计划、rewrite、物理计划。
- [execute_stage.cpp](../../../src/observer/sql/executor/execute_stage.cpp#L32) 区分“有物理计划的 DML/查询”和“直接命令执行的 DDL”。
- [sql_result.cpp](../../../src/observer/sql/executor/sql_result.cpp#L25) 打开物理算子并在非显式多语句事务中自动提交。

一个适合面试的总回答是：解析只回答“用户写了什么”，绑定回答“每个名字指向什么”，逻辑计划回答“要做哪些关系运算”，物理计划回答“用什么算法做”，算子迭代器最后才真正读写记录。

## 1. basic

### 题目目标

保持 MiniOB 的基础能力：建表、插入、查询、创建索引、查看表结构等。它看似简单，却是后续所有题目的地基：如果 schema、记录格式、扫描器或事务被破坏，后续功能都会失败。

### 必要原理

数据库至少要区分三类信息：

1. 元数据：表名、字段名、类型、长度、可空性和索引定义。
2. 数据：按固定记录布局写入表文件。
3. 执行计划：把 SELECT、INSERT、DELETE 等操作组织为算子树。

查询时不能只看字段名，还要把字段绑定到具体表；多表查询中同名列必须靠表名或别名消歧。索引也不是结果本身，而是从 key 找 RID，再回表取完整记录。

### 完整源码执行链

以 `SELECT id FROM t WHERE id = 1` 为例：

1. [session_stage.cpp](../../../src/observer/session/session_stage.cpp#L80) 创建并推进 SQL 事件。
2. parser 生成语法节点，resolve 阶段调用 `SelectStmt::create`；[select_stmt.cpp](../../../src/observer/sql/stmt/select_stmt.cpp#L34) 收集 FROM 表、别名、投影表达式和 WHERE。
3. [logical_plan_generator.cpp](../../../src/observer/sql/optimizer/logical_plan_generator.cpp#L328) 生成 `TableGet -> Predicate -> Project` 的逻辑树。
4. [physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L152) 查看谓词，若发现单列等值索引则生成 IndexScan，否则生成 TableScan。
5. 扫描器逐条产生 tuple，过滤谓词，Project 输出列；结果由 [sql_result.cpp](../../../src/observer/sql/executor/sql_result.cpp#L61) 逐条拉取。

DDL 则不生成查询计划，而是在 [command_executor.cpp](../../../src/observer/sql/executor/command_executor.cpp#L41) 分派到 CreateTable、CreateIndex、Desc 等 executor，成功后同步 DB（同文件 `128-134`）。

### 关键数据结构/算法

- `TableMeta` 描述 schema；`FieldMeta` 描述字段的 offset、长度、类型和 nullable 标志。
- `Record` 是固定布局的字节串；系统事务字段通常在用户字段前面。
- `TableScanPhysicalOperator` 顺序遍历 RecordFileScanner；`IndexScanPhysicalOperator` 先用 B+ 树取得 RID，再回表取记录。
- `Tuple` 是执行层视图；多表查询由 JoinedTuple 合并左右 tuple。
- 物理算子采用 Volcano/迭代器接口：`open()`、反复 `next()`、`current_tuple()`、`close()`。

### 示例 SQL 如何走

```sql
CREATE TABLE t(id INT, name CHAR(20));
CREATE INDEX i_t_id ON t(id);
INSERT INTO t VALUES (1, 'Ada');
SELECT name FROM t WHERE id = 1;
```

建表先写表 meta 和 data 文件；插入将值按 schema 组装为 Record；创建索引扫描已有 Record 并插入 `(id, RID)`；查询被物理规划器识别为单列等值索引扫描，然后再应用谓词并投影 `name`。

### 复杂度/取舍

- 全表扫描约为 `O(N)`，空间主要是一个当前 Record。
- B+ 树等值查找约为 `O(log_B N + K)`，`K` 是命中记录数，但每个 RID 仍需回表。
- 建索引需要扫描现有数据，约 `O(N log_B N)`；优点是之后查询快，代价是写入需同时维护索引。
- 固定长度记录访问简单、更新可预测，但字符串空间可能浪费，schema 变化需要重写存储。

### 当前实现边界

- 当前普通索引规划只识别单列等值谓词；范围谓词和复合索引不能由这一入口充分利用，详见第 8 题。
- 基础执行链默认假设表、字段和执行算子已经正确绑定；错误通常在 resolve 或算子执行时返回 RC。
- `SqlResult::close()` 承担自动事务收尾，因此调用方若中途不 close，可能留下未提交事务或资源。

### 老师可能追问与参考回答

**问：为什么 SQL 要分逻辑计划和物理计划？**

答：逻辑计划只表达关系语义，例如过滤、连接、投影；物理计划才选择顺序扫描、索引扫描、Nested Loop Join 等算法。这样同一个 SQL 可以替换执行算法而不改变语义。

**问：索引为什么还要回表？**

答：B+ 树叶子通常只保存 key 和 RID，RID 是行位置，不包含所有列；除非是覆盖索引，否则必须根据 RID 读取 Record。

## 2. update

### 题目目标

实现 `UPDATE t SET col = expr [WHERE predicate]`，支持有条件和无条件更新、单字段或多字段更新，并正确维护索引和事务。

### 必要原理

更新不是简单地覆盖一段内存：

1. 先找出满足 WHERE 的旧记录。
2. 按旧行求值 SET 表达式。
3. 构造新记录，检查类型、长度、NULL/NOT NULL。
4. 删除旧索引 key、写入新版本、插入新索引 key。
5. 在事务提交时使新版本可见，失败时保持原子性。

尤其要明确 SQL 的行语义：`SET score = score + 1` 必须对每一行自己的旧 `score` 求值；同一条 UPDATE 的多个 SET 通常都基于同一行旧快照，而不是前一个 SET 的结果。

### 完整源码执行链

1. [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y#L933) 解析 UPDATE、多个 SET 子句和 WHERE。
2. [update_stmt.cpp](../../../src/observer/sql/stmt/update_stmt.cpp#L27) 找表、检查字段可更新性，为每个 SET 表达式建立 BinderContext 并绑定；`124-177` 创建 FilterStmt。
3. [logical_plan_generator.cpp](../../../src/observer/sql/optimizer/logical_plan_generator.cpp#L188) 生成 `Update -> Predicate -> TableGet(READ_WRITE)`。
4. [physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L341) 把子树变为 UpdatePhysicalOperator。
5. [update_physical_operator.cpp](../../../src/observer/sql/operator/update_physical_operator.cpp#L16) 打开子算子、复制目标记录，然后调用 `trx_->update_record`。
6. 结果关闭时由 [sql_result.cpp](../../../src/observer/sql/executor/sql_result.cpp#L36) 提交或回滚事务。

### 关键数据结构/算法

- `UpdateStmt` 保存目标表、`field_metas_`、SET 表达式和 FilterStmt。
- `TableGetLogicalOperator` 使用 `READ_WRITE`，使扫描出来的记录参与事务访问。
- UpdatePhysicalOperator 先物化目标记录副本，再构造新 Record，避免边扫描边改写导致扫描器失效。
- `Table::update_record` 负责数据文件和索引的旧/新版本维护；MVCC 事务在提交/回滚时处理版本可见性。

### 示例 SQL 如何走

```sql
UPDATE t SET score = score + 1 WHERE id = 10;
```

WHERE 被绑定为字段比较，TableGet 可能走 `id` 的单列等值索引；UpdatePhysicalOperator 收集命中的 RID 和旧记录，计算新 score，检查 FLOAT/INT 类型和长度，再将每个旧记录更新为新版本。自动提交模式下 close 时 commit；显式 `BEGIN` 下则等 COMMIT。

### 复杂度/取舍

- 找目标行是 `O(N)` 顺序扫描，或 `O(log_B N + K)` 的索引扫描。
- 物化目标记录需要 `O(K * record_size)` 额外空间；优点是避免扫描和写入相互干扰。
- 每行更新还要维护每个相关索引，约为每索引 `O(log_B N)`。
- 失败时要求事务级原子性；简单的“逐行成功、失败后人工反向更新”难以覆盖索引、日志和并发，因此应依赖事务操作记录。

### 当前实现边界（必须记住）

**多行表达式存在明确 bug。** [update_physical_operator.cpp](../../../src/observer/sql/operator/update_physical_operator.cpp#L69) 到 `87` 只用 `records_.front()` 设置 tuple 并计算一次 `evaluated_values`；随后 `136-165` 对每一个 `old_record` 复用这组值。因此：

```sql
UPDATE t SET score = score + 1;
```

命中多行时，所有行都按第一行的 score 计算，而不是逐行 `old_score + 1`。正确做法应当是对每个 `old_record` 设置 tuple、逐个 SET 表达式求值，再构造该行的新记录。

另一个事务边界问题是 [update_physical_operator.cpp](../../../src/observer/sql/operator/update_physical_operator.cpp#L184) 的 `rollback()` 直接调用整个 `trx_->rollback()`，会回滚当前显式事务中此前的其他操作，然后重新启动事务；这不等价于“只回滚本条 UPDATE”。`log_records` 被记录但没有用于定向补偿。

其他边界：类型转换和长度检查在 `109-130`；设置 NULL/违反 NOT NULL 在 `151-157`；当前 parser 只支持普通 UPDATE，不支持复杂 JOIN UPDATE 语法。

### 老师可能追问与参考回答

**问：为什么先收集记录再更新？**

答：避免扫描器正在遍历的物理记录被更新、索引改变或 MVCC 可见性改变，导致漏行、重复行或游标失效。收集 RID/旧版本后再处理更稳定。

**问：如何修复 `records_.front()` bug？**

答：把 `evaluated_values` 从“整条 UPDATE 只算一次”改成“每个 old_record 新建一组值”；每次把 tuple 指向当前 old_record，依次求所有 SET 表达式，再做类型、NULL、长度校验和 `update_record`。所有 SET 应基于当前旧记录快照。

**问：显式事务中 UPDATE 失败应该怎么办？**

答：不能无条件调用整个事务的 rollback。应让失败传播给事务控制层，由用户 ROLLBACK 整个事务；或者建立本语句 savepoint/局部操作日志，只撤销本语句产生的操作。

## 3. drop-table

### 题目目标

实现 `DROP TABLE table_name`，删除表关联的元数据、数据文件、索引文件以及内存中的表、索引和全文索引对象。

### 必要原理

表对象通常同时持有：

- meta 文件描述 schema 和索引定义；
- data 文件对应 RecordFileHandler 和 buffer pool；
- B+ 树索引文件及已打开的 index handler；
- 内存索引、全文索引和 Db 的 opened_tables map 引用。

删除时必须先停止使用这些资源、刷脏页、关闭句柄，再删除文件，最后从目录和内存对象中摘除。

### 完整源码执行链

1. parser/stmt 创建 DropTableStmt，见 [drop_table_stmt.cpp](../../../src/observer/sql/stmt/drop_table_stmt.cpp#L17)。
2. [drop_table_executor.cpp](../../../src/observer/sql/executor/drop_table_executor.cpp#L22) 调用当前 Db 的 `drop_table`。
3. [db.cpp](../../../src/observer/storage/db/db.cpp#L201) 查找 opened_tables，调用 `table->drop()`；成功后 `erase` 并 `delete table`。
4. [table.cpp](../../../src/observer/storage/table/table.cpp#L183) 执行 sync、删文件、关闭 B+ 树 index。
5. DDL 成功后 CommandExecutor 再同步数据库元数据。

### 关键数据结构/算法

- `Db::opened_tables_` 保存表名到 `BaseTable*` 的内存目录。
- `Table::indexes_` 保存打开的 Index 对象。
- `Table::fulltext_indexes_` 保存内存全文索引。
- `Table::~Table` 负责最终释放 record handler、buffer pool 和 indexes。

### 示例 SQL 如何走

```sql
DROP TABLE t;
```

DropTableExecutor 找到当前 Db，Db 找到 Table，Table 刷新脏页并尝试删除 meta/data/index 文件。成功后 Db 从 map 删除表指针并析构，后续 `find_table("t")` 返回空。

### 复杂度/取舍

- 删除文件本身近似 `O(1)`，但 `sync()` 和关闭/刷写脏页可能与表大小、索引页数相关。
- 全文索引清理是内存对象释放，通常与索引条目数相关。
- 先关闭所有句柄再 unlink 是可靠性优先；直接删除打开文件在 Linux 上看似可行，但牺牲跨平台和异常恢复能力。

### 当前实现边界

当前 [Table::drop](../../../src/observer/storage/table/table.cpp#L183) 的实际顺序是：sync → 清空全文索引 → 删除 meta → 删除 data → 遍历并关闭 B+Tree index、删除索引文件；而 `record_handler_`、data buffer pool 到 `Table::~Table()` 才处理，见 `table.cpp:79-98`。这意味着：

- 删除 data/meta 时文件仍可能被 handler/buffer pool 打开；
- 索引文件在删除前会先调用 `index->close()`，因此不存在“删索引文件时 index handler 仍打开”的问题；真正仍打开的是 data 文件相关的 `record_handler_`/buffer pool；
- 中途失败可能留下部分文件已删除、内存状态不完整；
- 并发查询持有 Table 指针时没有显式生命周期协调。

正确的工程实现应先阻止新访问、等待现有扫描结束、sync、关闭 scanner/record handler/index/buffer pool，再按 index/data/meta 删除，最后从 Db map 移除并释放对象，并对失败步骤保留可恢复信息。

### 老师可能追问与参考回答

**问：为什么不能只删除 `.table` 元数据文件？**

答：数据文件和索引文件仍占磁盘空间，重启时还可能被发现；内存中的 opened table 和 index 指针也会悬空或继续提供已删除表。

**问：删除文件和关闭句柄哪个先？**

答：先关闭句柄再删除文件。Linux 允许 unlink 打开文件，但文件空间要等最后一个句柄关闭才回收，且 Windows 通常不允许这种操作。

## 4. date

### 题目目标

支持 DATE 字段、插入日期、比较日期、打印日期和非法日期检查；日期可能早于 1970 或晚于 2038，闰年和日期合法性必须正确。

### 必要原理

不要把 DATE 存成 `time_t`：那会受到 2038 和时区影响。当前实现把 `YYYY-MM-DD` 编码为整数 `YYYYMMDD`，例如 `2024-05-20 → 20240520`。合法性由年份、月份、当月天数和 Gregorian 闰年规则判断。

### 完整源码执行链

1. [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y#L687) 将 DATE 字段长度设为 `sizeof(int)`；nullable 时再追加 NULL 标志字节。
2. 字符串字面量在 [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y#L856) 先成为 CHARS Value。
3. [base_table.cpp](../../../src/observer/storage/table/base_table.cpp#L37) 构造 Record；目标字段类型与 Value 不同时调用 `Value::cast_to(..., false)`。
4. CHARS→DATE 在 [char_type.cpp](../../../src/observer/common/type/char_type.cpp#L33) 中调用 `parse_date`；TEXT 有对应实现。
5. `DateType::compare/to_string` 在 [date_type.cpp](../../../src/observer/common/type/date_type.cpp#L20) 中按整数比较并输出 `YYYY-MM-DD`。

### 关键数据结构/算法

- DATE Value 的内部 union 成员是 `int32_t`，类型标志为 `AttrType::DATES`。
- `check_date` 使用固定月份天数数组；闰年为能被 400 整除，或能被 4 整除但不能被 100 整除。
- 复合记录中 nullable DATE 仍以最后一个字节记录 NULL 标志，日期本体仍是四字节整数。

### 示例 SQL 如何走

```sql
CREATE TABLE t(id INT, birthday DATE);
INSERT INTO t VALUES (1, '2022-10-10');
SELECT birthday FROM t WHERE birthday < '2039-01-01';
```

插入时 `'2022-10-10'` 是 CHARS，`BaseTable::make_record` 将其转成 `20221010` 写入 DATE 字段。WHERE 的字符串常量在比较/索引边界转换时也应转成 DATE，再做整数比较。

### 复杂度/取舍

- 日期解析和合法性检查是 `O(1)`。
- 整数比较是 `O(1)`，也能直接用于排序和 B+ 树 key 比较。
- 固定 4 字节存储紧凑、无需时区库；代价是不能直接表示时间、时区或更丰富的日历语义。

### 当前实现边界

- [utils.cpp](../../../src/observer/common/utils.cpp#L31) 的 `parse_date` 使用 `sscanf("%d-%d-%d")`，不检查尾随字符，因此类似 `2024-1-1abc` 可能被接受。
- parser 本身只创建 CHARS；非法日期往往在 `make_record` 的类型转换阶段才失败。
- [date_type.h](../../../src/observer/common/type/date_type.h#L27) 声明 DATE→INT 的 cast cost，但 DateType 没有实现对应 `cast_to`；而 `Value::get_int/get_float/get_boolean` 对 DATE 返回 0，不能把 DATE 当普通数值参与算术。
- DATE_FORMAT 当前还允许 CHARS 输入，见第 7 题；题面主要要求 DATE。

### 老师可能追问与参考回答

**问：为什么 2038 年不会溢出？**

答：这里不是把日期转换成 Unix 秒数，而是直接存 `YYYYMMDD` 的 int。2039、1900 等日期都在 int 范围内，且合法性单独检查。

**问：2000 和 1900 哪个是闰年？**

答：2000 是，因为能被 400 整除；1900 不是，因为虽能被 100 整除但不能被 400 整除。

## 5. join-tables

### 题目目标

实现 INNER JOIN，支持多表、多个 ON 条件，并处理隐式逗号连接与 INNER JOIN 的组合。

### 必要原理

INNER JOIN 的结果是满足连接谓词的左右行组合；不匹配的行不输出。多表连接通常先形成二表结果，再与下一表连接，逻辑上可表示为左深树。连接算法有：

- Nested Loop Join：对每个左行扫描右侧，简单但复杂度高；
- Hash Join：按等值连接键建立 hash 表，适合大数据；
- Merge Join：两边按 key 排序后同步推进，适合已排序输入。

WHERE 中只涉及单表的谓词可以下推，减少 JOIN 输入；涉及两表的谓词应保留到 JOIN 后或 JOIN 内部。

### 完整源码执行链

1. INNER grammar 在 [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y#L1053)；多个 ON 条件在 `1287-1303` 合成为 AND。
2. [select_stmt.cpp](../../../src/observer/sql/stmt/select_stmt.cpp#L44) 建立表和 alias map，绑定投影、WHERE 和 ON 条件。
3. [logical_plan_generator.cpp](../../../src/observer/sql/optimizer/logical_plan_generator.cpp#L328) 将 N 张表按左深方式构造成 JoinLogicalOperator 树；WHERE 再包成 Predicate。
4. [physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L389) 选择 Nested Loop 或 Grace Hash，并递归生成左右子算子。
5. Nested Loop 的实际迭代在 [join_physical_operator.cpp](../../../src/observer/sql/operator/join_physical_operator.cpp#L20)；Join 算子生成组合 tuple，外层 Predicate 再过滤 ON/WHERE 条件。

### 关键数据结构/算法

- JoinLogicalOperator 有两个 child，分别是左、右输入。
- `JoinedTuple` 保存左右 tuple，并合并 base RID，便于后续字段访问和更新/全文检索。
- Nested Loop Join 左表 `open` 一次，每个左 tuple 重新 `open` 右表。
- GraceHashJoin 将两边 tuple 序列化到多个临时分区；但当前实现的 hash key 有重要限制，见边界。

### 示例 SQL 如何走

```sql
SELECT t.id, t1.name
FROM t INNER JOIN t1 ON t.id = t1.id
WHERE t.id > 10;
```

parser 把 `t`、`t1` 放进 relations，把 ON 条件和 WHERE 条件绑定。逻辑树大致为 `Project -> Predicate(WHERE) -> Join -> TableGet(t), TableGet(t1)`；当前实现 ON 条件也可能在 Join 上方 Predicate 执行。Nested Loop 产生候选组合，谓词保留 id 相等且大于 10 的结果。

### 复杂度/取舍

- Nested Loop 约为 `O(N*M)`，优点是实现简单、支持任意谓词；右输入每轮重开，I/O 可能很重。
- 理想等值 Hash Join 平均约为 `O(N+M)`，额外空间 `O(min(N,M))`；Grace Hash 用磁盘分区降低内存压力，但需要临时文件和可用的 join key。
- N 表左深树会影响中间结果大小；真实优化器应基于统计信息选择连接顺序和算法。

### 当前实现边界（必须记住）

- INNER JOIN grammar 分支只接 `where group_by`，缺少 HAVING、ORDER BY、LIMIT。
- 不能解析题面要求的混合语法：

  ```sql
  SELECT * FROM t INNER JOIN t1 ON t.id=t1.id, t2 WHERE t2.id=t.id;
  ```

- INNER JOIN 分支的 `relation` 规则不含 alias，存在 `INNER JOIN t1 AS x` 的别名缺口。
- [physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L399) 按 JOIN 深度选择算法，深度至少 3 才用 Grace Hash。
- 更严重的是 [grace_hash_join_physical_operator.cpp](../../../src/observer/sql/operator/grace_hash_join_physical_operator.cpp#L205) 的 `hash_tuple()` 固定返回 0。因为 Join 算子本身拿不到真正的等值谓词，它退化为单分区外部嵌套循环，不能利用 hash key，性能接近笛卡尔连接。
- 谓词下推规则 [predicate_pushdown_rewriter.cpp](../../../src/observer/sql/optimizer/predicate_pushdown_rewriter.cpp#L21) 只处理 Predicate 直接包 TableGet 的情况；多表 Predicate 子节点是 JOIN，通常不会下推到两边。

### 老师可能追问与参考回答

**问：ON 和 WHERE 都能写连接条件吗？**

答：INNER JOIN 下结果通常等价，但语义位置不同：ON 描述连接条件，WHERE 描述连接结果过滤；对 OUTER JOIN 两者不等价。当前题目只要求 INNER JOIN，但执行器应在逻辑层保留这一区别。

**问：为什么当前 Grace Hash 不能算真正 Hash Join？**

答：Join 条件被放在外层 Predicate，GraceHash 算子没有结构化的 join key 信息，无法只按 key 分区；固定返回 0 虽保证两边不因 hash 不一致而漏结果，但退化为单分区嵌套循环。

## 6. expression

### 题目目标

在 SELECT 和 WHERE 中支持数字算术表达式，如 `col3 * 4`、`5 + col2 < col1 + 6`，并正确推断结果类型、处理 NULL 和除零。

### 必要原理

表达式是树：叶子是字段或常量，内部节点是加减乘除、比较或逻辑运算。绑定阶段把字段名换成 FieldExpr；执行阶段对每行求值。

基本类型规则：两个 INT 做加减乘得到 INT；除法通常得到 FLOAT；任一输入 FLOAT 时结果为 FLOAT。NULL 参与算术应传播为 NULL；普通 NULL 比较结果为 false，`IS NULL` 是特殊判断。

### 完整源码执行链

1. parser 创建 ArithmeticExpr/ComparisonExpr。
2. [expression_binder.cpp](../../../src/observer/sql/parser/expression_binder.cpp#L387) 递归绑定左右子表达式。
3. [expression.cpp](../../../src/observer/sql/expr/expression.cpp#L470) 的 `value_type()` 静态推断结果类型。
4. tuple 执行走 [expression.cpp](../../../src/observer/sql/expr/expression.cpp#L595) 的 `get_value`；向量化执行走 `get_column/calc_column`（同文件 `619-663`）。
5. WHERE 的 ComparisonExpr 由 Predicate/扫描器调用，投影表达式由 Project 算子输出。

### 关键数据结构/算法

- `ArithmeticExpr` 保存操作类型和左右 unique_ptr。
- `value_type()` 决定结果 Value/Column 的存储类型。
- 标量路径调用 `Value::add/subtract/multiply/divide`，实际分派到 DataType。
- 向量化路径将 Column 批量运算，减少函数调用和 tuple 开销。

### 示例 SQL 如何走

```sql
SELECT col3 * 4 FROM exp_table
WHERE 5 + col2 < col1 + 6;
```

Binder 把 `col1/col2/col3` 绑定到字段；WHERE 形成 ComparisonExpr，左右分别是 ArithmeticExpr；对每个 tuple 先计算四则运算，再比较；满足条件的行交给 Project 计算 `col3*4`。

### 复杂度/取舍

- 每行表达式求值时间为表达式树节点数 `O(E)`，额外栈/临时 Value 约 `O(E)`。
- 向量化批处理总复杂度仍为 `O(N*E)`，但缓存和函数调用更友好；需要额外处理 Column、NULL bitmap 和除零。
- 静态类型推断能避免每次运行都选择结果类型，但字段类型只有绑定后才知道。

### 当前实现边界（必须记住）

- 标量除零处理正确：`FloatType::divide` 在 [float_type.cpp](../../../src/observer/common/type/float_type.cpp#L46) 检测接近零除数，设置 NULL 并返回 SUCCESS。整数除法的结果类型是 FLOAT，因此也走这条路径。
- 向量化除零不同：`ArithmeticExpr::calc_column` 进入 [arithmetic_operator.hpp](../../../src/observer/sql/expr/arithmetic_operator.hpp#L175) 的 `DivideOperator`，直接做 `left/right`；SIMD 整数先转 float 除法再转回 int，没有标量路径的 NULL 传播，结果可能不符合题面，甚至是未定义/错误值。
- Binder 只递归绑定，不严格验证算术操作数必须是数字；非数字算术可能到运行期才返回 UNSUPPORTED。
- `ConjunctionExpr` 以 bool 处理结果，不是完整 SQL 三值逻辑。

### 老师可能追问与参考回答

**问：为什么整数除法结果推成 FLOAT？**

答：SQL 中 `/` 通常表示真除法，`1/2` 应得到 `0.5`；如果需要整数商，应使用单独的整数除法语义。MiniOB 的 `value_type` 对 DIV 特殊返回 FLOAT。

**问：怎样修复向量化除零？**

答：在批量算子中建立除数为零的 mask；对应输出写 NULL bitmap，并避免对整数做未定义的整除。或者对含零的 batch 回退到标量路径，但性能较差。

## 7. function

### 题目目标

实现 `LENGTH`、`ROUND`、`DATE_FORMAT`，这些函数既能出现在 SELECT 列表中，也能出现在 WHERE 条件中。

### 必要原理

函数调用也是表达式节点。Binder 根据名字创建 NormalFunctionExpr；执行时先计算参数，再调用 builtin。函数应明确参数个数、输入类型、NULL 行为和返回类型。

### 完整源码执行链

1. parser 生成 UnboundFunctionExpr。
2. [expression_binder.cpp](../../../src/observer/sql/parser/expression_binder.cpp#L583) 识别普通函数，递归绑定参数并构造 NormalFunctionExpr。
3. [expression.cpp](../../../src/observer/sql/expr/expression.cpp#L1145) 逐参数求值并按函数类型分派到 builtin；返回类型在 `1205-1223` 给出。
4. [builtin.cpp](../../../src/observer/sql/builtin/builtin.cpp#L26) 实际完成字符串长度、舍入、日期拆分和格式替换。

### 关键数据结构/算法

- `NormalFunctionType` 枚举 LENGTH、ROUND、DATE_FORMAT 等函数类型。
- `NormalFunctionExpr` 持有参数表达式，并把参数 Value 放入 vector。
- LENGTH 是字符串长度的 `O(L)` 操作。
- ROUND 通过 `10^decimals` 缩放后舍入。
- DATE_FORMAT 扫描格式串，遇 `%` 查表替换日期字段。

### 示例 SQL 如何走

```sql
SELECT id, LENGTH(name), ROUND(score),
       DATE_FORMAT(u_date, '%D,%M,%Y')
FROM function_table;
```

Binder 分别绑定字段和函数；每行先求 `name/score/u_date`，LENGTH 返回 INT，ROUND 返回 FLOAT，DATE_FORMAT 将 DATE 拆成年月日并输出 CHARS。

### 复杂度/取舍

- LENGTH 为 `O(name_length)`，空间 `O(1)`（不考虑结果对象）。
- ROUND 为 `O(1)`。
- DATE_FORMAT 为 `O(format_length)`，日期拆分为 `O(1)`。
- 在 builtin 运行期校验类型，代码简单灵活；代价是错误发现晚于绑定阶段。

### 当前实现边界

- LENGTH [builtin.cpp](../../../src/observer/sql/builtin/builtin.cpp#L26) 只接受 CHARS；TEXT、DATE、INT 会返回 INVALID_ARGUMENT。
- ROUND [builtin.cpp](../../../src/observer/sql/builtin/builtin.cpp#L39) 只接受 FLOAT 主参数、INT 小数位；`.5` 使用银行家舍入，而常见 MySQL ROUND 语义通常是远离零方向，存在语义差异。
- DATE_FORMAT [builtin.cpp](../../../src/observer/sql/builtin/builtin.cpp#L199) 接受 DATE 或 CHARS，虽然题目主要要求 DATE。
- 题目要求的 `%Y/%y/%m/%d/%D/%M` 已实现，同时额外实现 `%c/%e`。
- 未知 `%z/%n` 的实现是追加 `z/n`，即去掉百分号，符合题面示例；结尾孤立 `%` 会走普通字符分支，原样输出 `%`。
- `%Y` 使用 `std::to_string(year)`，年份 1 会输出 `1`，不是严格四位 `0001`；`%y` 也假定年份字符串至少四位。
- 函数参数的类型主要由 builtin 运行期检查，普通函数 binder 没有完整签名校验。

### 老师可能追问与参考回答

**问：为什么 DATE_FORMAT 未知格式不报错？**

答：题面明确要求非法格式符原样输出（示例中 `%z` 输出 `z`），所以这是兼容策略；若要严格 MySQL 兼容，则应明确区分转义、非法符号和错误返回。

**问：ROUND 为什么需要第二个参数？**

答：第二参数表示保留的小数位；负数表示在小数点左侧舍入，例如保留到十位。当前实现用 `pow(10, decimals)` 做统一处理。

## 8. multi-index

### 题目目标

支持 `CREATE INDEX idx ON t(col1, col2)`，正确建立、持久化、维护和比较多字段 B+ 树 key，并支持合理的查询/范围扫描。

### 必要原理

复合索引不是多个独立索引，而是一棵按 `(col1, col2, ...)` 字典序排序的 B+ 树。每个 key 必须包含：

```text
固定顺序的 col1 bytes | col2 bytes | ... | RID
```

RID 用于让相同列值的多行仍能形成不同的 B+ 树 key。复合索引的“最左前缀”规则是核心：`(a,b)` 能高效支持 `a = 1`，也能支持 `a = 1 AND b = 2`；只查 `b = 2` 通常不能得到连续 key 范围。

### 完整源码执行链

1. `CREATE INDEX` 的 `attr_list` 在 [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y#L574) 支持多列。
2. [create_index_stmt.cpp](../../../src/observer/sql/stmt/create_index_stmt.cpp#L24) 通过表 schema 得到 FieldMeta vector。
3. [index_meta.cpp](../../../src/observer/storage/index/index_meta.cpp#L13) 计算每列在复合 key 中的 offset 和总长度。
4. [index_meta.h](../../../src/observer/storage/index/index_meta.h#L51) 从 Record 按列顺序拷贝 key bytes。
5. [bplus_tree.cpp](../../../src/observer/storage/index/bplus_tree.cpp#L1994) 将 `fields_total_len + sizeof(RID)` 写入 B+ 树文件头；[bplus_tree.h](../../../src/observer/storage/index/bplus_tree.h#L93) 的 KeyComparator 按列顺序比较。
6. 插入、删除和唯一检查由 [bplus_tree_index.cpp](../../../src/observer/storage/index/bplus_tree_index.cpp#L89) 使用完整复合 key 完成。

### 关键数据结构/算法

- `IndexMeta::fields_` 保存列顺序；`fields_offset_` 保存拼接 key 中每列的偏移。
- nullable 字段的 NULL 标志也随字段 bytes 进入 key；KeyComparator 将 NULL 排在非 NULL 前。
- B+ 树内部节点按 KeyComparator 路由，叶子保存 key + RID；叶子链表支持顺序扫描。
- 扫描复合范围时，边界需要补齐没有指定的后缀：下界用后缀最小值，上界用后缀最大值，再配合 RID min/max。

### 示例 SQL 如何走

```sql
CREATE INDEX i_1_12 ON multi_index(col1, col2);
SELECT * FROM multi_index WHERE col1 = 1 AND col2 >= 10;
```

建索引时每行形成 `bytes(col1=1)|bytes(col2=...)`，再附加 RID 插入 B+ 树。理想查询规划应构造下界 `(1,10,RID_MIN)`、上界 `(1,MAX_COL2,RID_MAX)`，扫描叶子连续区间，再用完整谓词过滤。

### 复杂度/取舍

- 插入、删除、等值查找约为 `O(log_B N)`，key 比较成本为字段数 `F`，可写成 `O(F log_B N)`。
- 复合 key 越宽，B+ 树每页容纳的 entry 越少，树高和 I/O 可能增加；但一个索引可以同时服务多个列条件。
- 建索引需扫描所有记录并插入，约 `O(N log_B N)`；维护成本是每次写入都要更新该树。
- 复合索引比多个单列索引省一个索引入口，但只能高效支持符合前缀顺序的谓词。

### 当前实现边界（必须记住）

**复合索引可以维护，但不能用于查询。**

- [table.cpp](../../../src/observer/storage/table/table.cpp#L1626) 的 `find_index_by_field` 明确只返回 `fields.size() == 1` 的索引。
- [physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L180) 只从单个 FieldExpr + ValueExpr 等值谓词选择索引。
- [index_scan_physical_operator.cpp](../../../src/observer/sql/operator/index_scan_physical_operator.cpp#L60) 的 `build_search_key` 遇到多列索引直接返回 `UNSUPPORTED`。
- 所以复合索引能够创建、写入、删除、持久化和用于唯一 key 检查，但查询规划不会使用它，不能做左前缀、部分 key 或多列范围扫描。
- B+Tree 底层 scanner 本身要求完整的 attr key，并会补 RID min/max，见 [bplus_tree.cpp](../../../src/observer/storage/index/bplus_tree.cpp#L1801)；缺的是上层从谓词组装复合边界的逻辑。

### 老师可能追问与参考回答

**问：`(a,b)` 为什么不能只高效查 `b`？**

答：树先按 a 排序，a 不固定时，相同 b 的记录会分散在多个 a 段中，不是一个连续区间；扫描只能遍历大量 key 后过滤。

**问：`a=1 AND b BETWEEN 10 AND 20` 如何构造边界？**

答：下界为 `(1,10,RID_MIN)`，上界为 `(1,20,RID_MAX)`，再根据开闭区间决定 RID 使用 min 还是 max；扫描结果还要重新执行完整谓词，处理 NULL 和类型转换。

**问：为什么 key 里要放 RID？**

答：非唯一索引允许多行拥有相同列值；如果 key 只有列 bytes，B+ 树 comparator 会把它们视为相同，RID 作为末尾 tie-breaker 可让每一行有独立 entry。
