# 项目架构与一条 SQL 的完整执行链

## 1. 先把 MiniOB 看成六层

从数据库小白的视角，可以先把整个项目划分为六层：

```text
┌─────────────────────────────────────┐
│ 1. 接入层：网络、CLI、Session       │
├─────────────────────────────────────┤
│ 2. SQL 前端：Parser、Binder、Stmt   │
├─────────────────────────────────────┤
│ 3. 优化层：逻辑计划、规则、物理计划 │
├─────────────────────────────────────┤
│ 4. 执行层：open/next/close 算子树   │
├─────────────────────────────────────┤
│ 5. 事务层：Vacuous Trx / MVCC Trx  │
├─────────────────────────────────────┤
│ 6. 存储层：Table、Record、Index、Page│
└─────────────────────────────────────┘
```

对应目录主要是：

```text
src/observer/session                 会话和请求入口
src/observer/sql/parser              词法、语法解析和表达式绑定
src/observer/sql/stmt                不同 SQL 的语义对象
src/observer/sql/optimizer           逻辑计划、改写规则、物理计划
src/observer/sql/operator            物理执行算子
src/observer/storage/trx             事务、MVCC、事务日志
src/observer/storage/table           表、元数据和 DDL
src/observer/storage/record          页内记录管理
src/observer/storage/index           B+Tree、IVF、全文索引
src/observer/storage/buffer          Buffer Pool、Page、Disk Buffer Pool
```

## 2. 公共 SQL 执行链

一条 SQL 的主路径是：

```text
客户端/CLI 输入 SQL
    ↓
Communicator → SqlTaskHandler
    ↓
ParseStage：Lexer + Bison Parser
    ↓
ResolveStage：ParsedSqlNode → Stmt
    ↓
OptimizeStage：逻辑计划 → Rewrite → 物理计划
    ↓
SqlResult 驱动 open/next/close
    ↓
Trx → Table/Index/RecordManager/BufferPool
```

关键入口：

- [SqlTaskHandler](../../../src/observer/net/sql_task_handler.cpp#L21)：`handle_event` 读取请求，`handle_sql` 依次调用 QueryCache/Parse/Resolve/Optimize/Execute。
- [ParseStage](../../../src/observer/sql/parser/parse_stage.cpp#L30)：把 SQL 字符串解析成 `ParsedSqlNode`。
- [ResolveStage](../../../src/observer/sql/parser/resolve_stage.cpp#L31)：做名称和语义解析，创建具体 `Stmt`。
- [OptimizeStage](../../../src/observer/sql/optimizer/optimize_stage.cpp#L32)：创建逻辑/物理计划并运行改写规则。
- [ExecuteStage](../../../src/observer/sql/executor/execute_stage.cpp#L32)：处理部分直接执行的 SQL。
- [SqlResult](../../../src/observer/sql/executor/sql_result.cpp#L25)：驱动物理查询计划并输出结果。

`SessionStage::handle_request/handle_sql` 是保留的旧路径，源码已标注 `TODO remove me`，全仓没有实际调用者。当前网络请求到 parser 的真实入口是 [`SqlTaskHandler::handle_sql`](../../../src/observer/net/sql_task_handler.cpp#L58)，不是 `SessionStage::handle_sql`。

## 3. Parser：先判断“这句话长什么样”

Parser 解决的是语法问题。例如：

```sql
select id, score + 10
from student
where class_id = 1;
```

它会识别：

- 这是一条 SELECT；
- SELECT 列表中有字段 `id` 和算术表达式 `score + 10`；
- FROM 中有表 `student`；
- WHERE 中有比较表达式 `class_id = 1`。

这一步还没有确认 `student` 是否真的存在，也没有确认 `score` 是不是数字类型。它主要负责把字符串转成结构化的语法节点。

语法定义主要位于 [yacc_sql.y](../../../src/observer/sql/parser/yacc_sql.y)。

面试类比：Parser 像编译器前端的语法分析器，只负责确认句子是否符合语法，并建立抽象语法树。

## 4. Resolver/Binder：确认名字和类型到底指什么

Resolver 和 ExpressionBinder 处理语义问题：

- `student` 是否存在；
- `student.id` 是否存在；
- 没写表名前缀时，字段是否有歧义；
- `score + 10` 两边的类型是否能做加法；
- 子查询中的字段来自内层表还是外层表；
- 函数名和参数如何绑定。

解析结果会从通用的 `ParsedSqlNode` 变成：

- `SelectStmt`
- `InsertStmt`
- `UpdateStmt`
- `DeleteStmt`
- `CreateTableStmt`
- `AlterTableStmt`
- 其他具体语句对象

这里可以把 `Stmt` 理解为“已经完成名称绑定和基本语义检查的 SQL”。

表达式绑定入口见 [expression_binder.cpp](../../../src/observer/sql/parser/expression_binder.cpp)。

## 5. 逻辑计划：描述要做什么

逻辑计划使用关系代数表达查询含义，不关心具体使用哪一种算法。

例如：

```sql
select name
from student
where score > 90
order by name
limit 10;
```

逻辑树可以表示为：

```text
Project(name)
  └── Limit(10)
        └── OrderBy(name)
              └── Predicate(score > 90)
                    └── TableGet(student)
```

逻辑算子的含义是：

- `TableGet`：读取一个关系；
- `Predicate`：过滤行；
- `Project`：选择输出列；
- `Join`：连接两个关系；
- `GroupBy`：分组聚合；
- `OrderBy`：排序；
- `Limit`：只保留指定数量。

入口见 [logical_plan_generator.cpp](../../../src/observer/sql/optimizer/logical_plan_generator.cpp)。

## 6. Rewrite 和物理计划：决定具体怎么做

同一个逻辑操作可能有多种物理算法：

```text
TableGet
  → TableScan
  → IndexScan

Join
  → NestedLoopJoin
  → HashJoin

OrderBy
  → 内存排序
  → External Sort
```

优化器需要判断选哪一种。当前 MiniOB 主要依赖规则和启发式，而不是成熟的 Cost-Based Optimizer。

例如等值条件：

```sql
select * from student where id = 10;
```

如果 `id` 上有可识别的单列索引，物理计划可能使用 `IndexScanPhysicalOperator`；否则使用 `TableScanPhysicalOperator`。

物理计划生成入口见 [physical_plan_generator.cpp](../../../src/observer/sql/optimizer/physical_plan_generator.cpp)。

当前优化器的典型简化包括：

- 主要识别单字段等值索引；
- 复合索引不能构造搜索 key；
- 缺少可靠的表行数、NDV、选择率等统计信息；
- 连接算法选择依赖树深度等启发式；
- 没有系统地枚举 join order。

## 7. 火山模型：查询不是一次性执行完

SELECT 通常不是在 `ExecuteStage` 中一次性跑完，而是把物理算子树交给 `SqlResult`。

上层通过统一接口拉取数据：

```cpp
operator.open(trx);
while (operator.next() == RC::SUCCESS) {
  Tuple *tuple = operator.current_tuple();
  // 输出 tuple
}
operator.close();
```

这叫 Volcano Iterator Model。

每个算子的职责：

- `open`：初始化资源并打开孩子；
- `next`：产生下一条结果；
- `current_tuple`：返回当前结果；
- `close`：关闭孩子并释放文件、内存等资源。

以 Predicate 为例：

```text
Predicate.next()
    ↓
反复调用 child.next()
    ↓
对当前 tuple 计算 WHERE 表达式
    ↓
条件为真时，把该 tuple 返回上层
```

优点：

- 算子接口统一；
- 能够流式处理；
- 算子容易组合。

缺点：

- 一行一次的函数调用开销较大；
- 阻塞算子仍需收集大量数据，如排序和聚合；
- 需要严格重置 `open/close` 状态，否则重复执行会出错。

## 8. Tuple、Record、RID 和 Page 的区别

这四个概念经常被混淆：

### Tuple

执行器看到的逻辑行，可以由多个表拼接，也可以包含表达式和聚合结果。

### Record

存储层的一条物理记录，是一段按照 TableMeta 布局排列的定长字节。

### RID

物理记录地址，一般是：

```text
(page_num, slot_num)
```

索引叶子节点保存的最终是 RID，而不是整条记录。

### Page

Buffer Pool 和磁盘之间进行 I/O 的基本单位。RecordManager 在页内维护 slot，BufferPool 决定哪些页暂时驻留内存。

一次索引回表可以理解为：

```text
index key → RID → page_num → BufferPool 取页 → slot_num 取 Record
```

## 9. Table、RecordManager、Index 和 BufferPool

存储层分工：

- `Table`：知道表的 schema、索引和数据文件。
- `RecordFileHandler/Scanner`：负责插入、读取、扫描和更新页内 Record。
- `Index`：维护字段值到 RID 的映射。
- `DiskBufferPool`：把磁盘页装入内存并负责刷盘。
- `Frame`：Buffer Pool 中某个页对应的内存帧。

INSERT 的底层思路是：

```text
字段 Value
→ 按 FieldMeta 编码成 Record bytes
→ RecordManager 找有空槽的 Page
→ 写入 Record 并得到 RID
→ 向每个普通/全文/向量索引添加 entry
```

如果索引插入失败，普通模式下会尝试删除已经添加的索引项和记录，这是一种补偿式原子性。

## 10. 事务层：Vacuous 和 MVCC

项目中至少需要区分两种事务思路：

### Vacuous Transaction

更接近直接修改物理记录，事务能力很弱，适合教学和单用户功能测试。

### MVCC Transaction

记录包含隐藏的：

```text
__trx_xid_begin
__trx_xid_end
```

扫描到记录后，事务根据版本区间判断当前快照是否可见。UPDATE 不覆盖旧记录，而是让旧版本结束、插入一个新版本。

关键入口见 [mvcc_trx.cpp](../../../src/observer/storage/trx/mvcc_trx.cpp)。

需要注意：索引扫描找到 RID 不代表记录一定可见。索引中可能保留历史版本的 entry，必须回表后通过 `visit_record` 做 MVCC 判断。

## 11. DDL 和 SELECT 为什么走法不同

SELECT 天然适合物理算子树，因为它会不断产生结果行。

而 CREATE TABLE、DROP TABLE、CREATE INDEX 等 DDL 通常不产生行，更常见的链路是：

```text
Stmt
→ 对应 Executor
→ Db/Table 的 DDL 方法
→ 修改 metadata 和文件
```

因此不能看到 `ExecuteStage` 很短，就认为数据库没有执行 SQL。查询结果主要由 `SqlResult` 拉取，DDL 则更多由专门 Executor 直接完成。

## 12. 用一条 SELECT 串起整个过程

SQL：

```sql
select s.name, round(s.score, 1) as rounded_score
from student s
where s.class_id = 1
order by s.score desc
limit 5;
```

执行过程：

```text
1. Parser 识别 SELECT/FROM/WHERE/ORDER/LIMIT 和函数调用
2. Binder 把 s 绑定到 student，把字段绑定到 FieldMeta
3. round 从未绑定函数变成 NormalFunctionExpr
4. Logical Planner 生成 TableGet→Predicate→Order→Limit→Project
5. Physical Planner 选择 TableScan 或 IndexScan，以及排序算子
6. SqlResult 调用根算子的 open
7. Predicate 从扫描算子逐行拉取并过滤 class_id=1
8. OrderBy 收集并排序满足条件的 tuple
9. Limit 只向上提供 5 行
10. Project 计算 name 和 round(score,1)
11. Communicator 根据输出 schema 打印结果
```

## 13. 用一条 UPDATE 串起事务和存储

SQL：

```sql
update student
set score = score + 1
where class_id = 1;
```

主路径：

```text
Parser
→ UpdateStmt 绑定目标列和表达式
→ TableGet(READ_WRITE)
→ Predicate(class_id=1)
→ UpdatePhysicalOperator 先收集目标 Record
→ 对每条记录构造新字段值
→ Trx::update_record
→ Table/RecordManager/Index
```

先收集目标记录用于避免 Halloween Problem；但当前实现错误地只按第一条记录求值一次，这将在赛题 2 中详细分析。

## 14. 面试必答：这个架构的优点和不足

可以这样回答：

> 项目的优点是 SQL 前端、逻辑计划、物理算子、事务和页式存储边界比较清晰，算子使用统一的 open-next-close 接口，便于添加新功能。主要不足是优化器缺少统计信息和成本模型，很多 SQL 语义采用比赛型简化；DDL、视图 DML、全文和向量索引的事务恢复也没有形成完整闭环。

老师继续追问“你最先改什么”，可以回答：

1. 统一 `Value` 的 NULL、比较、hash 和排序语义。
2. 修复 UPDATE 逐行表达式求值并增加 statement rollback。
3. 补充统计信息和复合索引范围构造。
4. 完善已有 Hash Group By 和 Grace Hash Join：前者当前仍顺序查找 group，后者因没有 join key 而退化；同时补充子查询去相关。
5. 完善 MVCC 的日志、唯一键并发控制和版本垃圾回收。
