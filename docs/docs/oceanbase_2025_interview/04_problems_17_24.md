# MiniOB 2025 赛题 17--24：从源码到面试

本文面向刚开始学习数据库的同学。每题先说明“要解决什么”，再从 SQL 到存储引擎走一遍实际调用链；最后区分“已经做到了什么”和“源码仍有什么边界”。源码链接均相对于本文所在目录。

## 17. text：大文本字段

### 目标

支持 `TEXT` 列，最大输入为 65535 **字节**。题目的难点不是语法，而是如何让一行中有很长的文本仍能被页式存储引擎读写。

### 必要原理

MiniOB 的磁盘页是 128 KB：[`page.h`](../../../src/observer/storage/buffer/page.h#L26)。常见数据库的 TEXT/LOB 实现会在普通行中放一个 locator（长度、首溢出页号等），真实文本放到溢出页或独立 LOB 文件；这样普通 record 仍小，长文本可跨页。

本仓库当前的方案不是 locator，而是把 TEXT 当作 65535 字节的定长内联字段。因此它能放进一个 128 KB 页的数据区，但加上页头、slot bitmap、其它列和对齐后，通常一页只能放一行；它没有“跨页大对象”的能力。

### 源码链路与数据结构

`CREATE TABLE t(a TEXT)` 经 parser 将字段长度设为 65535：[`yacc_sql.y`](../../../src/observer/sql/parser/yacc_sql.y#L702)。`TableMeta` 将所有字段长度相加，得到固定 `record_size`：[`table_meta.cpp`](../../../src/observer/storage/table/table_meta.cpp#L92-L111)。

插入时，`BaseTable::make_record` 一次分配整个 record，检查 value 长度不超过字段长度，然后把 bytes 内联复制进去：[`base_table.cpp`](../../../src/observer/storage/table/base_table.cpp#L37-L103)。最后由 `RecordFileHandler` 找空闲页或申请新页，`RowRecordPageHandler` 以固定 slot 大小写入：[`record_manager.cpp`](../../../src/observer/storage/record/record_manager.cpp#L47-L52) 和 [`record_manager.cpp`](../../../src/observer/storage/record/record_manager.cpp#L550-L613)。

内存中的 `Value::set_text` 自己持有一份 C-string：[`value.cpp`](../../../src/observer/common/value.cpp#L273-L292)；比较和类型转换由 [`text_type.cpp`](../../../src/observer/common/type/text_type.cpp#L19-L105) 处理。

### SQL 如何执行

```sql
CREATE TABLE article(id INT, body TEXT);
INSERT INTO article VALUES (1, '...');
SELECT body FROM article WHERE id = 1;
```

INSERT 先构造 65535-byte 的逻辑字段空间，再写一个固定长度 record；SELECT 从该 record 的字段 offset 取回 TEXT。大多数短文本也会占用这块预留空间，这是此实现的主要代价。

### 复杂度与取舍

- 单行读写仍是固定 record 的 O(record_size)，不是只与真实文本长度相关。
- 优点是实现简单、按 offset 取字段快。
- 缺点是短文本浪费空间，页利用率低；也无法支持真正超过一页的文本。

### 边界与缺陷

- 题面要求“扩展 record manager，支持超过一页的数据”，当前实现没有 overflow page/LOB locator；只是利用 128 KB 页面容纳了 65535-byte 的定长字段。
- `Value::set_text` 使用 `strnlen/strlen`，所以内嵌 `\0` 会被视为结尾：[`value.cpp`](../../../src/observer/common/value.cpp#L281-L290)。这不是二进制安全的 TEXT。
- `TextType::to_string` 也按 C-string 输出：[`text_type.cpp`](../../../src/observer/common/type/text_type.cpp#L93-L99)。
- 枚举注释仍写“4096 字节”，与实际 parser 的 65535 不一致：[`attr_type.h`](../../../src/observer/common/type/attr_type.h#L26)。

### 老师追问与参考回答

**问：为什么不把 TEXT 直接做成一个更大的页？**  
答：大页会放大 buffer pool、WAL、锁和 I/O 放大的成本。更常见的做法是普通页保留小 locator，真实大对象放溢出页链；这样热点行仍很小。

**问：如何保证更新长文本时不泄漏旧溢出页？**  
答：把新 locator 写入新版本/redo，提交后旧版本由 MVCC GC 或引用计数回收；回滚删除新链。不能在写新内容前立即释放旧链。

## 18. vector-search：精确向量检索与 IVF

### 目标

题目要求在**没有向量索引**时支持精确 top-k，例如：

```sql
SELECT id FROM tab_vec
ORDER BY DISTANCE(embedding, STRING_TO_VECTOR('[10,0,5]'), 'EUCLIDEAN')
LIMIT 1;
```

精确检索（exact search）和 IVF（近似最近邻索引）必须分开理解：前者扫描所有向量，后者只扫描少量桶，速度更快但可能漏掉真正最近邻。

### 必要原理

- L2：`sqrt(sum((a[i]-b[i])^2))`，越小越近。
- cosine distance：`1-cos(a,b)`，越小越近。
- inner product/dot：通常越**大**越相似；不能沿用“越小越好”的距离排序规则。

### 源码链路与数据结构

无索引 SQL 会保留普通 `TABLE_SCAN`。`DISTANCE` 在 binder 中转换为 L2/COSINE/INNER_PRODUCT 函数：[`expression_binder.cpp`](../../../src/observer/sql/parser/expression_binder.cpp#L560-L579)，每行通过 `NormalFunctionExpr` 计算距离：[`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L1166-L1168)，随后 `OrderByPhysicalOperator` 排序并由 LIMIT 截断：[`order_by_physical_operator.cpp`](../../../src/observer/sql/operator/order_by_physical_operator.cpp#L34-L85)。这就是精确路径。

有索引时，optimizer 只识别 `ORDER BY 一个向量距离 ASC LIMIT n`：[`vector_index_scan_rewrite.cpp`](../../../src/observer/sql/optimizer/vector_index_scan_rewrite.cpp#L20-L105)。它把参数写入 `TableGetLogicalOperator`：[`table_get_logical_operator.h`](../../../src/observer/sql/operator/table_get_logical_operator.h#L42-L67)，物理计划改为 `VectorScanPhysicalOperator`：[`physical_plan_generator.cpp`](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L152-L178)。

IVF 的数据结构是中心点 `centroids_` 和每个中心的 `(vector, RID)` bucket：[`ivfflat_index.cpp`](../../../src/observer/storage/index/ivfflat_index.cpp#L120-L168)。搜索只选 `probes_` 个中心，再排序候选：[`ivfflat_index.cpp`](../../../src/observer/storage/index/ivfflat_index.cpp#L171-L210)。

### SQL 如何执行

- 无 IVF：扫描 N 行，N 次距离计算，排序后取 k；结果精确。
- 有 IVF：先找最近的 `probes` 个中心，只比这些桶里的候选；结果是 ANN。

`VectorScanPhysicalOperator` 再按 RID 回表并做普通 predicate/MVCC 检查：[`vector_scan_physical_operator.cpp`](../../../src/observer/sql/operator/vector_scan_physical_operator.cpp#L27-L85)。

### 复杂度与取舍

- Exact：距离计算 O(ND)，全排序 O(N log N)；可用大小为 k 的 heap 改为 O(N log k)。
- IVF：大约 O(lists·D + candidates·D + candidates log candidates)，以 recall 换延迟。
- 当数据量很小或必须 100% recall 时应选 exact；IVF 适合大量高维数据。

### 边界与缺陷

- IVF 对 inner product 仍以升序排序：[`ivfflat_index.cpp`](../../../src/observer/storage/index/ivfflat_index.cpp#L181-L202)，方向错误。
- `probes_` 没有 clamp 到中心数，`probes_ > centroids_.size()` 会访问越界：[`ivfflat_index.cpp`](../../../src/observer/storage/index/ivfflat_index.cpp#L190-L193)。
- 空表建索引后中心为空，第一次 `insert_entry` 会按空中心找 bucket，存在越界风险：[`ivfflat_index.cpp`](../../../src/observer/storage/index/ivfflat_index.cpp#L224-L229)。
- IVF `open` 没有恢复数据，且 `delete_entry` 是空实现：[`ivfflat_index.cpp`](../../../src/observer/storage/index/ivfflat_index.cpp#L86-L118) 与 [`ivfflat_index.cpp`](../../../src/observer/storage/index/ivfflat_index.cpp#L232)。重启问题更严重：[`Table::open`](../../../src/observer/storage/table/table.cpp#L327-L342) 只对全文索引特别分支，其他索引一律构造 `BplusTreeIndex` 并尝试打开索引文件。IVF 没有持久化该文件，因此重启后不是“索引数据丢失但表可用”，而是整张表打开失败。删除、UPDATE 的索引一致性也仍不可靠。

### 老师追问与参考回答

**问：top-k exact 为什么不必完整排序？**  
答：维护一个大小为 k 的最大堆；扫描每个距离，若比堆顶更优就替换。总复杂度 O(ND+N log k)。

**问：IVF 怎样提高 recall？**  
答：增大 `probes`，但候选和延迟增加；还要处理删除 tombstone、重建中心，以及空索引的首批数据。

## 19. alter：表重写与失败回滚

### 目标

实现 ADD/DROP/CHANGE COLUMN 和 RENAME TABLE，并保证原有行和单列索引与新 schema 一致。

### 必要原理

固定长度 row format 下，删除或新增中间字段会改变后续 offset。因此安全思路是“写新文件”：用新 schema 扫描旧表、逐行转换写入临时 data 文件、重建索引、最后原子切换 metadata/data。

### 源码链路与数据结构

`AlterTableStmt` 做语义检查：[`alter_table_stmt.cpp`](../../../src/observer/sql/stmt/alter_table_stmt.cpp#L16-L96)，executor 转给 `Db::alter_table`：[`alter_table_executor.cpp`](../../../src/observer/sql/executor/alter_table_executor.cpp#L10-L25) 和 [`db.cpp`](../../../src/observer/storage/db/db.cpp#L225-L269)。

ADD/DROP 会复制 `TableMeta` 后调用 `rewrite_table_storage`：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L832-L912)。它由 `copy_record_to_new_layout` 根据 field name mapping 拷贝老字段；新 nullable 字段会写 null flag：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L496-L544)。数据写进 `.data.tmp`，之后重建普通索引：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L701-L830)。

CHANGE 只改字段名和索引 metadata（offset 不变）：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L914-L970)。RENAME 改 data/meta/index 文件名：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L973-L1057)。

### SQL 如何执行

```sql
ALTER TABLE t ADD COLUMN age INT;
ALTER TABLE t DROP COLUMN age;
ALTER TABLE t CHANGE COLUMN old_name new_name INT;
ALTER TABLE t RENAME TO t2;
```

ADD 后老行的新 nullable 列是 NULL；DROP 后引用该列的索引 metadata 被移除；CHANGE 不改 bytes，只改 schema 和 index field name。

### 复杂度与取舍

- ADD/DROP：扫描并重写全部数据，O(N·record_size)，再 O(N log N) 重建 B+ 树索引。
- CHANGE：字段 offset 不变时主要是 metadata/index reopen，成本低。
- 这是典型 copy-on-write DDL，但只有在“最终切换”是事务性的前提下才安全。

### 边界与缺陷

- 当前 ALTER **非原子**。`rewrite_table_storage` 在最终 data rename 前已执行 `drop_all_indexes(true)` 和关闭旧 handler：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L780-L787)。后续文件 rename、meta persist、reload 或 rebuild 失败都没有完整恢复旧文件/内存状态。
- 旧 data 先改名为 `.bak`，新 data 升级后就删除 `.bak`，但 metadata 随后才持久化：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L790-L824)。因此可能得到新 data + 旧 meta。
- 通用 `rebuild_indexes` 的 factory 仅支持 B+ tree/IVF：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L574-L585)。全文索引的 ALTER 迁移不能可靠重建。
- RENAME 同样先销毁运行期索引/handler，局部文件回退不等于对象和 meta 回退：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L989-L1055)。

### 老师追问与参考回答

**问：怎样做到 ALTER 原子？**  
答：把新 data、新 indexes、新 meta 全部写到临时名字并 fsync；写一条 DDL phase journal；最后按可恢复顺序 rename。崩溃恢复根据 journal 要么全部 promote，要么删除新文件并保留旧文件。

**问：新增 NOT NULL、无默认值列怎么迁移旧行？**  
答：应拒绝该 DDL，或要求默认值；不能悄悄写零值，因为零不等于 SQL 默认/NULL。

## 20. update-mvcc：多版本 UPDATE

### 目标

在多连接、串行测试下，让 UPDATE 具备未提交不可见、提交生效、回滚恢复的 MVCC 语义，同时不破坏索引。

### 必要原理

每行有隐藏列 `__trx_xid_begin/__trx_xid_end`：一个已提交版本对事务 snapshot 可见；UPDATE 通常不是原地覆盖，而是“旧版本结束 + 新版本开始”。这叫 version chain；之后需要 GC 清理再也不可见的旧版本。

### 源码链路与数据结构

MVCC kit 给每表增加 `__trx_xid_begin/__trx_xid_end`：[`mvcc_trx.cpp`](../../../src/observer/storage/trx/mvcc_trx.cpp#L33-L56)。UPDATE 的 SQL 计划为 READ_WRITE table-get 加 predicate，再交给 `UpdatePhysicalOperator`：[`logical_plan_generator.cpp`](../../../src/observer/sql/optimizer/logical_plan_generator.cpp#L188-L214)。

operator 先收集匹配行、复制 record、写入 SET 值：[`update_physical_operator.cpp`](../../../src/observer/sql/operator/update_physical_operator.cpp#L16-L181)。`MvccTrx::update_record` 检查旧版可写，把 old.end 设为负 trx id，插入 new.begin 为负 trx id 的新 record：[`mvcc_trx.cpp`](../../../src/observer/storage/trx/mvcc_trx.cpp#L201-L263)。

可见性判断在 [`mvcc_trx.cpp`](../../../src/observer/storage/trx/mvcc_trx.cpp#L265-L317)。commit 将 old.end/new.begin 改为 commit xid：[`mvcc_trx.cpp`](../../../src/observer/storage/trx/mvcc_trx.cpp#L412-L447)；rollback 删除新版本并恢复旧版：[`mvcc_trx.cpp`](../../../src/observer/storage/trx/mvcc_trx.cpp#L532-L583)。

### SQL 如何执行

```sql
BEGIN;
UPDATE t SET score = score + 1 WHERE id = 1;
-- 本事务看到新版本；其它事务不能看到未提交新版本
COMMIT;  -- 新版本 __trx_xid_begin、旧版本 __trx_xid_end 变为 commit xid
```

普通表扫描和 B+tree index scan 都会在取到物理 record 后调用 `trx->visit_record`，从而跳过不可见版本：[`record_manager.cpp`](../../../src/observer/storage/record/record_manager.cpp#L831-L844)、[`index_scan_physical_operator.cpp`](../../../src/observer/sql/operator/index_scan_physical_operator.cpp#L169-L196)。

### 复杂度与取舍

- UPDATE 额外插入一个完整 record，写放大 O(record_size)，并保留旧版本直到 GC。
- 当前索引保留新旧版本 entry，索引扫描后再做可见性过滤；写快一些，但读放大和索引膨胀严重。

### 边界与缺陷

- 没有 UPDATE transaction log。日志操作枚举只有 INSERT/DELETE/COMMIT/ROLLBACK：[`mvcc_trx_log.cpp`](../../../src/observer/storage/trx/mvcc_trx_log.cpp#L26-L35)；`MvccTrx::update_record` 也不追加 update log。崩溃恢复不能完整地 redo/undo UPDATE。
- MVCC UPDATE 调 `table->insert_record(new_record)`，并未移除旧版普通索引 entry：[`mvcc_trx.cpp`](../../../src/observer/storage/trx/mvcc_trx.cpp#L239-L261)。正确性主要靠读时过滤，且没有 GC：[`mvcc_trx.h`](../../../src/observer/storage/trx/mvcc_trx.h#L62-L67)。
- SET 表达式只以 `records_.front()` 计算一次：[`update_physical_operator.cpp`](../../../src/observer/sql/operator/update_physical_operator.cpp#L69-L134)。因此 `SET c=c+1` 可能把所有命中行设成第一行计算出的同一值，而不是逐行求值。
- operator 先 materialize 所有候选 record：[`update_physical_operator.cpp`](../../../src/observer/sql/operator/update_physical_operator.cpp#L34-L59)，大 UPDATE 有内存压力。
- 没有 statement savepoint；出错时是 `trx_->rollback()` 整个事务：[`update_physical_operator.cpp`](../../../src/observer/sql/operator/update_physical_operator.cpp#L184-L200)。

### 老师追问与参考回答

**问：二级索引怎样支持 MVCC？**  
答：索引 entry 应带可见版本信息，或指向 version chain；不能只靠永久堆积旧 entry。查询用 snapshot 判断 entry 对应版本是否可见，GC 在无活跃 snapshot 后清理。

**问：如何避免 `UPDATE t SET c=c+1` 的 Halloween 问题？**  
答：先固定候选 RID 集或使用稳定 snapshot，再逐行计算/写新版本；不能一边依赖会被自己更新的扫描条件一边继续扫描。

## 21. complex-sub-query：相关子查询与 EXISTS

### 目标

支持子查询引用外层行，例如 `EXISTS`、`IN`、标量子查询，以及子查询中的聚合。

### 必要原理

不相关子查询可只执行一次；相关子查询需要对每个外层 tuple 重新绑定“当前外层值”。最朴素的实现是 nested-loop apply：外层 N 行，每行执行一次内层计划。优化器后续可把 EXISTS/IN 改为 semi join。

### 源码链路与数据结构

`SelectStmt::create` 将 parent table map 合入当前 `table_map`：[`select_stmt.cpp`](../../../src/observer/sql/stmt/select_stmt.cpp#L34-L87)。binder 为 `SubQueryExpr` 生成 SelectStmt、logical plan、physical plan：[`expression_binder.cpp`](../../../src/observer/sql/parser/expression_binder.cpp#L607-L624)。

比较表达式发现子查询后，每个外层 tuple 都 open/close 子计划：[`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L221-L256)。`SubQueryExpr::open` 通过 `set_parent_tuple` 将外行向下传递：[`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L841-L847)；物理算子递归传播：[`physical_operator.cpp`](../../../src/observer/sql/operator/physical_operator.cpp#L49-L54)。表扫描/predicate 将 inner 和 outer 拼成 `JoinedTuple` 后计算条件：[`table_scan_physical_operator.cpp`](../../../src/observer/sql/operator/table_scan_physical_operator.cpp#L103-L129)。

EXISTS/NOT EXISTS 只探测第一行：[`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L258-L278)；标量子查询会检查第二行并报多行错误：[`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L288-L290) 与 [`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L341-L343)。

### SQL 如何执行

```sql
SELECT * FROM t1
WHERE EXISTS (SELECT 1 FROM t2 WHERE t2.id = t1.id);
```

扫描 `t1` 的一行后，把该行作为 parent tuple 执行 `t2` 的 plan；t2 找到首行就返回 true。嵌套子查询会继续携带外层 table map。

### 复杂度与取舍

- 朴素相关 EXISTS：最坏 O(|t1|·|t2|)。内层有 id index 时可接近 O(|t1| log |t2|)。
- 优点是实现直接，天然支持深层相关条件。
- 缺点是重复扫描；应优化为 semi join、anti join、subquery cache 或 parameterized index scan。

### 边界与缺陷

- `ComparisonExpr` 用 `subquery_expr->open(nullptr, tuple)`：[`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L231-L239)。内层 scan 因 trx 为 null 而不检查 MVCC 可见性：[`record_manager.cpp`](../../../src/observer/storage/record/record_manager.cpp#L831-L838)。
- `IN/NOT IN` 的 NULL 三值逻辑不完整；代码只在后续 RHS 行显式处理 NULL，首个 RHS NULL 会走普通 compare：[`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L293-L331)。
- `SubQueryExpr` 中的 `res_query/visited_index` 没有驱动实际执行：[`expression.h`](../../../src/observer/sql/expr/expression.h#L560-L572)。
- 子查询放到投影而非 comparison 时，没有统一的 open 生命周期保证；当前主要路径依赖 `ComparisonExpr` 显式 open。

### 老师追问与参考回答

**问：NOT IN 为什么比 NOT EXISTS 难？**  
答：只要子查询结果含 NULL，且左值未匹配任何非 NULL 值，NOT IN 的结果是 UNKNOWN，不是 true；WHERE 会过滤 UNKNOWN。NOT EXISTS 没有这个三值陷阱。

**问：怎样把 EXISTS 优化成 join？**  
答：把相关等值条件提取为 join key，改为 semi join；只判断是否存在匹配，不能输出或重复外行。

## 22. create-view：视图查询与 DML 路由

### 目标

视图是存储 SELECT 定义的虚表。单表基础列视图可以路由 INSERT/UPDATE/DELETE 到基表；含聚合、表达式或多表时必须按可更新规则拒绝或限制。

### 必要原理

可更新的核心条件是：一次 DML 能无歧义映射到一张基表的一行/一组基础列。视图扫描还必须保留每个输出行来自哪些 `(base table, RID)`，否则 UPDATE/DELETE 不知道要改谁。

### 源码链路与数据结构

CREATE VIEW 先构造 SelectStmt：[`create_view_stmt.cpp`](../../../src/observer/sql/stmt/create_view_stmt.cpp#L17-L42)，executor 从原 SQL 中提取 SELECT 并创建 View：[`create_view_executor.cpp`](../../../src/observer/sql/executor/create_view_executor.cpp#L26-L119)。`View::create` 保存 select SQL，并建立 `field_index_ = (base_table, field_id)`：[`view.cpp`](../../../src/observer/storage/table/view.cpp#L20-L151)。

查询 View 时 `ViewScanPhysicalOperator` 重解析保存的 SELECT、建子计划、再把子 tuple 变成 view record；同时复制 base RIDs：[`view_scan_physical_operator.cpp`](../../../src/observer/sql/operator/view_scan_physical_operator.cpp#L22-L96) 和 [`view_scan_physical_operator.cpp`](../../../src/observer/sql/operator/view_scan_physical_operator.cpp#L192-L214)。

INSERT 的前置可更新检查在 [`insert_stmt.cpp`](../../../src/observer/sql/stmt/insert_stmt.cpp#L47-L130)，路由实现在 [`view.cpp`](../../../src/observer/storage/table/view.cpp#L173-L240)。UPDATE 的同基表检查在 [`update_stmt.cpp`](../../../src/observer/sql/stmt/update_stmt.cpp#L53-L163)，路由在 [`view.cpp`](../../../src/observer/storage/table/view.cpp#L255-L299)。DELETE 对 join view 直接拒绝：[`delete_stmt.cpp`](../../../src/observer/sql/stmt/delete_stmt.cpp#L47-L57)。

### SQL 如何执行

```sql
CREATE VIEW v AS SELECT id, age FROM t;
INSERT INTO v(id, age) VALUES(1, 18);
UPDATE v SET age=19 WHERE id=1;
DELETE FROM v WHERE id=1;
```

理想情况下，view record 的 `field_index_` 把 `id/age` 定位到 `t` 的字段；view scan 保留 `t` 的 RID，UPDATE/DELETE 便可精确下推。

### 复杂度与取舍

- 查询复杂度就是其定义 SELECT 的复杂度，当前每次 scan 会重新 parse/plan view SQL。
- 单表基础列 DML 应近似基表 DML。
- 多表视图若允许 DML，需要跨表事务和更严格的字段归属判断；本仓库用“同一基表字段”作有限检查。

### 边界与缺陷

- `View::insert_record` 直接调用基表 `insert_record`，绕过 `trx->insert_record`：[`view.cpp`](../../../src/observer/storage/table/view.cpp#L225-L235)。因此 MVCC begin/end、事务日志、跨表原子回滚都被绕开。
- MVCC DELETE/UPDATE 对 View 不可靠：`View::get_record/visit_record` 只是返回 SUCCESS，未提供真实行：[`view.cpp`](../../../src/observer/storage/table/view.cpp#L301-L303)。而 MVCC 的通用 DML 依赖这些函数。
- `View::update_record` 会遍历所有 base RIDs 并更新，不仅是 SET 所属基表：[`view.cpp`](../../../src/observer/storage/table/view.cpp#L260-L296)。
- 它遍历含系统列的 `field_metas`，却按逻辑字段下标访问 `field_index_`：[`view.cpp`](../../../src/observer/storage/table/view.cpp#L270-L275)，存在索引约定错误/越界风险。
- 重启时 `init_member` 只按字段名在基表中找第一个匹配，忽略 table alias；两表都有 `id` 时会错映射：[`view.cpp`](../../../src/observer/storage/table/view.cpp#L431-L453)。
- 多表 INSERT 即使前置检查通过，若后续某基表插入失败，前面已插的表不会补偿：[`view.cpp`](../../../src/observer/storage/table/view.cpp#L181-L239)。

### 老师追问与参考回答

**问：什么视图不能 UPDATE？**  
答：聚合、GROUP BY、含不可写表达式、无法确定唯一基表/基表行的 join view 都应拒绝。单表基础列视图可写，但缺失 NOT NULL 无默认值列时 INSERT 仍应失败。

**问：怎样保存映射避免重启后猜字段？**  
答：在 view metadata 中持久化每个输出列的 source table id、source field id、表达式标记，以及 view 定义版本；不要只用列名搜索。

## 23. full-text-index：jieba、BM25 与增量维护

### 目标

使用 jieba 分词，建立倒排索引，并让 `MATCH(col) AGAINST(query)` 返回 BM25 分数。

### 必要原理

倒排索引是 `term -> posting list`；posting 包含 `(RID, term frequency)`。还需保存每篇文档的 token 数，才能计算平均文档长度。

本实现的 BM25Okapi 形式为：

`score(D,Q) = Σ IDF(q) * tf(q,D)*(k1+1)/(tf(q,D)+k1*(1-b+b*|D|/avgdl))`

其中 `k1=1.5`，`b=0.75`；IDF 为 `log((N-df+0.5)/(df+0.5))`，对负 IDF 用 `0.25*average_idf` 替换，以对齐 rank_bm25 的 epsilon 规则。

### 源码链路与数据结构

`ALTER TABLE ... ADD FULLTEXT` 从 [`db.cpp`](../../../src/observer/storage/db/db.cpp#L261-L264) 进入 `Table::create_fulltext_index`，扫描已有 record 并 `add_document`：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L1284-L1418)。`FullTextIndex` 的核心成员是 `unordered_map<string, PostingList>`、`doc_stats_` 和 `total_tokens_`：[`fulltext_index.h`](../../../src/observer/storage/index/fulltext_index.h#L24-L157)。

`add_document` 分词、统计 TF、更新 posting 和文档长度：[`fulltext_index.cpp`](../../../src/observer/storage/index/fulltext_index.cpp#L19-L76)。BM25 计算在 [`fulltext_index.cpp`](../../../src/observer/storage/index/fulltext_index.cpp#L170-L253)，IDF/TF normalization 在 [`fulltext_index.cpp`](../../../src/observer/storage/index/fulltext_index.cpp#L323-L374)。

`JiebaUtil` 用 `Jieba::Cut` 后访问 cppjieba 的 stopword 集删除停用词：[`jieba_util.cpp`](../../../src/observer/common/fulltext/jieba_util.cpp#L72-L97)。MATCH 每行取得 RID 后直接调用 `calculate_bm25`：[`expression.cpp`](../../../src/observer/sql/expr/expression.cpp#L978-L1107)。

### SQL 如何执行

```sql
ALTER TABLE docs ADD FULLTEXT INDEX ft_content(content) WITH PARSER jieba;
SELECT id, MATCH(content) AGAINST('数据库 索引') AS score
FROM docs
WHERE MATCH(content) AGAINST('数据库 索引') > 0
ORDER BY MATCH(content) AGAINST('数据库 索引') DESC, id ASC;
```

当前查询计划仍是**全表扫描**：每个扫描到的 row 取其 RID，再在内存倒排表中算该 RID 的 BM25。它没有由 posting list 先产生候选 RID 的 fulltext scan 算子，因此功能正确路径不是高效倒排检索。

DML 时，普通插入会经 `insert_entry_of_indexes` 调用 `add_document`；普通 UPDATE 插入同一 RID 的新文档时，`add_document` 内部会先移除该 RID 的旧内容。但普通 `Table::delete_record` 只遍历 `indexes_` 中的普通索引，没有调用包含全文清理逻辑的 `delete_entry_of_indexes`，所以普通 DELETE 不会执行 `remove_document`：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L1492-L1503) 和 [`table.cpp`](../../../src/observer/storage/table/table.cpp#L1544-L1613)。重启时 `Table::open` 会扫描记录重建内存倒排索引：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L251-L325)。

### 复杂度与取舍

- 建索引/重启重建：O(total tokens)。
- 当前 MATCH：表扫描 O(N) 加每行查 posting；没有发挥倒排索引的候选过滤优势。
- 正统查询应分词 query 后合并 postings、算候选 top-k，复杂度主要与命中 postings 数有关。

### 边界与缺陷

- `remove_document` 不删除空 posting 的 term：[`fulltext_index.cpp`](../../../src/observer/storage/index/fulltext_index.cpp#L79-L109)。`calculate_average_idf` 又把这些 df=0 的 stale term 纳入平均值：[`fulltext_index.cpp`](../../../src/observer/storage/index/fulltext_index.cpp#L356-L374)，DELETE/UPDATE 后 epsilon BM25 偏离 rank_bm25。
- 普通 `Table::delete_record` 没有调用全文索引的 `remove_document`，被删文档仍留在内存倒排结构中，N、df 和平均文档长度继续包含它。
- `search_with_scores` 过滤掉 `score <= 0`：[`fulltext_index.cpp`](../../../src/observer/storage/index/fulltext_index.cpp#L255-L295)，如果这条路径被使用，负 IDF epsilon 的合法结果会被丢弃。但全仓当前没有该函数的查询调用者，现行 MATCH 路径直接调用 `calculate_bm25`，因此这是死代码风险，不是当前查询路径的现行 bug。
- MVCC UPDATE 新插一个版本但不会 remove 旧版本的全文文档；旧版本继续污染 N、df、avgdl。重启重建又以 null trx 扫描所有物理版本：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L277-L323)。
- 通用 DROP INDEX 只从普通 `indexes_` 找索引，全文索引只在 `fulltext_indexes_`，因此 fulltext drop 路径不完整：[`table.cpp`](../../../src/observer/storage/table/table.cpp#L1421-L1468)。
- jieba 用默认构造和 `__FILE__` 寻找字典，未显式固定题目部署词典；又用 private accessor 取私有 stopword 成员：[`jieba_util.cpp`](../../../src/observer/common/fulltext/jieba_util.cpp#L13-L40) 和 [`jieba_util.cpp`](../../../src/observer/common/fulltext/jieba_util.cpp#L80-L90)。这对 cppjieba 版本/路径非常脆弱。

### 老师追问与参考回答

**问：为何 BM25 不能只记录命中文档？**  
答：BM25 的 IDF 依赖总文档数 N 和 df，长度归一化依赖所有文档的 avgdl；删除、MVCC 版本、空文档都会影响统计。

**问：怎样避免全文索引被 MVCC 历史版本污染？**  
答：posting 要带版本可见性，或只在 commit 时发布新文档/撤销旧文档，并按 snapshot 计算统计；GC 后清理不可见版本。

## 24. big-order-by：外部排序与退化 GraceHashJoin

### 目标

在内存约束下处理四表笛卡尔积后的大 ORDER BY。需要把中间结果分批落盘、分别排序、再多路归并；连接端也需要避免把全部中间结果留在内存。

### 必要原理

外部排序分两阶段：

1. run generation：缓冲区满时排序，写出一个有序 run；
2. k-way merge：每个 run 读一个头 tuple，最小堆选下一个输出。

Grace Hash Join 应从等值 ON 条件提取 join key，左右按相同 hash(key) 分区；每次把一个 build partition 放内存，以 hash table probe 对应的另一侧分区。若某 partition 仍太大，需递归分区或 fallback。

### 源码链路与数据结构

`OrderByPhysicalOperator::open` 用行数/字段数/join depth 估算决定是否 external sort：[`order_by_physical_operator.cpp`](../../../src/observer/sql/operator/order_by_physical_operator.cpp#L88-L145)。外排给 `ExternalSorter` 5 MB buffer：[`order_by_physical_operator.cpp`](../../../src/observer/sql/operator/order_by_physical_operator.cpp#L213-L243)。它每 1000 行或内存阈值 flush 一个 run：[`external_sorter.cpp`](../../../src/observer/sql/operator/external_sort/external_sorter.cpp#L55-L156)，再用 priority queue 多路归并：[`external_sorter.cpp`](../../../src/observer/sql/operator/external_sort/external_sorter.cpp#L159-L223)。run tuple 由 [`tuple_serializer.cpp`](../../../src/observer/sql/operator/external_sort/tuple_serializer.cpp#L21-L235) 序列化。

join depth 至少 3 时 planner 选 GraceHashJoin：[`physical_plan_generator.cpp`](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L390-L460)。它把左右 tuple 写到 partition files：[`grace_hash_join_physical_operator.cpp`](../../../src/observer/sql/operator/grace_hash_join_physical_operator.cpp#L82-L203)，再逐 partition 读左侧并与右侧组合：[`grace_hash_join_physical_operator.cpp`](../../../src/observer/sql/operator/grace_hash_join_physical_operator.cpp#L266-L400)。

### SQL 如何执行

```sql
SELECT * FROM t1, t2, t3, t4
ORDER BY t1.c1, t2.c1, t3.c1, t4.c1;
```

当前 planner 对深 join 选择 Grace 算子，随后 ORDER BY 倾向 external sort。外排能减少“最终排序数组”峰值，但 join 的实现质量决定了前面会不会先耗尽内存或时间。

### 复杂度与取舍

- 外排：run 内排序总计约 O(N log M)，多路归并 O(N log R)，M 是缓冲区容量、R 是 run 数；代价是临时磁盘 I/O。
- 真正 Grace Hash Join：partition O(|L|+|R|)，每分区 hash join 近似 O(|Li|+|Ri|)。
- 当前 Grace 实际退化为外部 nested loop，最坏 O(|L|·|R|)。

### 边界与缺陷

- NULL comparator 错误：`NullType::compare` 对任何右值都返回 -1，连 `NULL` 对 `NULL` 也是 -1：[`null_type.cpp`](../../../src/observer/common/type/null_type.cpp#L17-L21)。排序 comparator 使用 `Value::compare`：[`external_sorter.cpp`](../../../src/observer/sql/operator/external_sort/external_sorter.cpp#L260-L300)。这违反严格弱序，`std::sort` 行为未定义；应先定义 NULLS FIRST/LAST，再让 NULL==NULL。
- serializer 不支持 `TEXTS`，default 返回 `UNSUPPORTED`：[`tuple_serializer.cpp`](../../../src/observer/sql/operator/external_sort/tuple_serializer.cpp#L433-L500)。外排包含 TEXT 会失败。
- NULL 反序列化为无类型 `NullValue`，丢失原字段类型：[`tuple_serializer.cpp`](../../../src/observer/sql/operator/external_sort/tuple_serializer.cpp#L503-L516)。
- run 数没有 fan-in 限制，所有 run readers 同时打开：[`external_sorter.cpp`](../../../src/observer/sql/operator/external_sort/external_sorter.cpp#L167-L195)；内存估算也没含 schema/spec/container 等完整开销：[`tuple_serializer.cpp`](../../../src/observer/sql/operator/external_sort/tuple_serializer.cpp#L392-L430)。
- `GraceHashJoinPhysicalOperator::hash_tuple` 恒返回 0：[`grace_hash_join_physical_operator.cpp`](../../../src/observer/sql/operator/grace_hash_join_physical_operator.cpp#L205-L216)。全部数据进入一个 partition，没有 join key。
- 它不建 hash table，而是将整个左 partition 读入 `build_tuples_`：[`grace_hash_join_physical_operator.cpp`](../../../src/observer/sql/operator/grace_hash_join_physical_operator.cpp#L360-L385)，再对每个右 tuple 遍历所有左 tuple：[`grace_hash_join_physical_operator.cpp`](../../../src/observer/sql/operator/grace_hash_join_physical_operator.cpp#L293-L342)。
- `memory_limit_` 只保存、没有用于控制 partition 或 build side：[`grace_hash_join_physical_operator.h`](../../../src/observer/sql/operator/grace_hash_join_physical_operator.h#L83-L117)。因此深 join 仍可能超内存。
- planner 只看 join depth，不看是否有等值 key、数据量和选择率：[`physical_plan_generator.cpp`](../../../src/observer/sql/optimizer/physical_plan_generator.cpp#L399-L445)。

### 老师追问与参考回答

**问：为什么 hash join 不能 hash 整行？**  
答：左右 join 的整行通常不同，即使 key 相等也会落到不同 partition，产生漏结果。必须从等值谓词提取同一逻辑 key，并对两侧使用一致的 hash/NULL 规则。

**问：单个 hash partition 超内存怎么办？**  
答：用新的 hash seed 递归 repartition；若数据倾斜导致始终超限，改为 block nested-loop 或 external sort-merge join。内存预算必须真实计入 tuple、hash bucket、I/O buffer。

**问：NULL 排序如何实现才安全？**  
答：先判断双方是否 NULL：两者都 NULL 返回相等；只有一侧 NULL 按 NULLS FIRST/LAST 返回；两侧非 NULL 才调用类型 compare。这样 comparator 才满足严格弱序。
