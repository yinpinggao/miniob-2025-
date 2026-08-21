# 数据库内核全景：从一张表到一个数据库系统

## 本章导读

这是整套教材的第 0 章，也是唯一一章"不讲细节"的章。后面 12 章会逐层拆解数据库内核的每个模块：Buffer Pool、记录布局、B+ 树、SQL 前端、执行器、优化器、事务、日志……但在钻进任何一层之前，你需要先回答三个最朴素的问题：

1. 数据库到底解决了什么问题，为什么用 Excel 和文件不行？
2. 一个数据库系统内部有哪些部件，它们各自负责什么？
3. 一条 SQL 从敲下回车到返回结果，究竟经历了什么？

本章用生活类比、一张具体的小表和两条 SQL（一条 SELECT、一条 INSERT），把这三个问题讲清楚，最后给出全教材 13 章与 MiniOB 2025 全部 24 道赛题之间的学习路线图。读完本章，你应该能对着一张分层图说出每一层的职责，能粗略描述一条 SQL 的旅程，并且知道面试时"什么是数据库内核"这类开放题该怎么组织答案。

阅读前建议先浏览官方架构文档 [MiniOB 架构](../../design/miniob-architecture.md) 和本系列的 [项目架构与一条 SQL 的完整执行链](../01_project_architecture.md)；本章是它们的"零基础前置版"，细节会指回这两篇和后续各章，不重复展开。

## 1. 为什么需要数据库：从 Excel 的崩溃现场说起

### 1.1 直觉：用文件存数据会发生什么

假设你是教务处的志愿者，要用最朴素的工具管理全校学生的成绩。最自然的做法：建一个 Excel，一行一个学生，列是学号、姓名、班级、成绩。或者更"程序员"一点：写一个 CSV 文件，再写个 Python 脚本读写它。

第一天很好用。然后麻烦接踵而至：

- **场景一：两个人同时改。** 你和另一位老师同时打开文件，各自改了一个学生的成绩，先后保存。后保存的把先保存的覆盖掉了——一次修改凭空消失。你们约定"改之前先吼一声"，但人一多就乱。
- **场景二：改到一半断电。** 你在保存一个有 10 万行的文件，写到一半机房断电。文件只剩半截，前半截是新数据，后半截是旧数据，还混着一段乱码。哪半截是新的？没人知道。
- **场景三：换个问法就得重写程序。** 领导今天问"各班平均分"，明天问"每班前五名"，后天问"成绩在 85 到 90 之间、按班级分组的人数"。每换一种问法，你的 Python 脚本就要改一遍，而且一改就容易写错。
- **场景四：文件格式升级。** 你想给学生加一个"邮箱"列。所有读写这个文件的脚本都得跟着改，漏改一个就出乱码。

这四类麻烦，恰好对应数据库系统（DBMS, Database Management System）存在的四个理由：

| 文件/Excel 的痛点 | 数据库的回答 | 涉及的内核机制 |
|---|---|---|
| 并发修改互相覆盖、互相看见半成品 | **并发控制**：多个事务同时执行但互不干扰 | 事务、MVCC（[第 09 章](09_transaction_mvcc.md)） |
| 写到一半崩溃，数据自相矛盾 | **崩溃恢复**：断电后能把数据库恢复到一个正确的状态 | WAL 日志、redo（[第 10 章](10_logging_recovery.md)） |
| 每换一种查询就要重写程序 | **声明式查询**：你只描述"要什么"，系统决定"怎么做" | SQL 前端、优化器、执行器（第 04–08 章） |
| 改存储格式要动所有程序 | **数据独立性**：逻辑视图与物理存储解耦 | 元数据、抽象执行接口（贯穿全书） |

可以把 DBMS 类比成一家**银行的金库加账房**：钱（数据）不是随便堆在仓库里，而是由出纳（接入层）接待、账房先生（SQL 层）按规矩记账、库房（存储层）按固定格式码放，还有一本流水账（日志）保证停电了也能对得上账。你自己用文件存数据，等于把钱堆在自己床底下——不是不行，是规模一大、人一多、一出意外就完蛋。

### 1.2 严格说法

一个数据库管理系统是在**操作系统文件之上**的一层软件，它为数据的存取提供：

- **数据独立性（Data Independence）**：应用程序面对的是"表"这样的逻辑结构，不用关心数据在磁盘上怎么摆。加索引、换存储格式，SQL 一句不用改。这又分为逻辑独立性（改表结构不影响视图）和物理独立性（改物理组织不影响表）。
- **并发控制（Concurrency Control）**：让成百上千个事务同时读写同一份数据，效果上却像是一个一个排队执行的。
- **崩溃恢复（Crash Recovery）**：任何时刻断电、进程被杀，重启后数据库都能恢复到"已提交的事务都在、未提交的事务都不在"的状态。
- **声明式查询（Declarative Query）**：用 SQL 描述想要的结果集，由系统生成并选择执行方案。

这四条是贯穿全书的主线。后面每一章你都会看到它们如何落到具体的代码机制上。

### 1.3 术语辨析：数据库、DBMS、数据库系统

三个词在日常对话里混用，面试时最好分清：

- **数据库（Database, DB）**：有组织的数据集合本身——那些表和记录。
- **数据库管理系统（DBMS）**：管理数据库的**软件**，如 MySQL、PostgreSQL、OceanBase、MiniOB。本教材说的"数据库内核"就是 DBMS 的核心部分。
- **数据库系统（Database System, DBS）**：DB + DBMS + 应用程序 + 数据库管理员（DBA）的整体。

所以严谨的说法是"用 MiniOB 这个 DBMS 管理一个数据库"，而不是"用 MiniOB 数据库管理系统管理一个数据库管理系统"。

## 2. 关系模型速成

数据库里装的不是"一堆数据"，而是按**关系模型（Relational Model）**组织的数据。这是 Codd 在 1970 年提出的模型，也是 MySQL、PostgreSQL、OceanBase、MiniOB 共同的理论地基。

### 2.1 关系、元组、属性

先给一张具体的表，后面所有例子都围着它转：

`student` 表：

| id（学号） | name（姓名） | class_id（班级） | score（成绩） |
|---:|---|---:|---:|
| 1 | Alice | 101 | 90 |
| 2 | Bob | 101 | 85 |
| 3 | Carol | 102 | 92 |

严格概念一句话版：

- **关系（Relation）**：一张表。`student` 就是一个关系。
- **元组（Tuple）**：表里的一行。`(1, Alice, 101, 90)` 是一个元组。
- **属性（Attribute）**：表里的一列。`id`、`name` 都是属性，每个属性有名字和类型（如 `id` 是 INT，`name` 是字符串）。
- **模式（Schema）**：表的"表头"——有哪些属性、各是什么类型。关系是模式的一个实例。

类比：表是 Excel 工作表，元组是一行，属性是一列，模式是首行表头加上每列的格式约定。区别在于关系模型里**行与行没有顺序**（它是一个集合），列里的值必须是原子值（不能一个单元格里再塞一张小表）。

### 2.2 主键：每一行的身份证

**主键（Primary Key）**是能唯一标识一个元组的一组属性。在 `student` 里，`id` 是主键：任意两行不能有相同的学号。

为什么需要它？因为没有主键，"把 Bob 的成绩改成 88"这句话就是有歧义的——可能有两个 Bob。主键给了每一行一个全局唯一的"身份证号"，让 UPDATE、DELETE 能精确点名。工程上主键通常还承担两个角色：唯一性约束由唯一索引强制保证（赛题 9 unique，原理见 [第 03 章](03_bplus_tree.md)），以及作为其他表引用本表的锚点（外键，MiniOB 未实现，了解概念即可）。

### 2.3 SQL：说"要什么"，不说"怎么做"

SQL（Structured Query Language）是关系模型的标准查询语言，它最大的特点是**声明式**。看这条查询：

```sql
select name
from student
where score > 88
order by score desc;
```

它说的是"我要成绩大于 88 的学生的名字，按成绩从高到低排"，而**没有说**：

- 是从头到尾扫一遍 `student`，还是走某个索引；
- 是先过滤再排序，还是边扫边维护一个堆；
- 排序用快排、堆排还是外部归并。

这些"怎么做"的决定权交给了数据库内部的**优化器**。对上面这张三行的表，答案是 `(Carol), (Alice)`——跟着算一遍：`score > 88` 留下 Alice(90) 和 Carol(92)，降序排后 Carol 在前。

声明式带来两个深远后果：

1. **同一条 SQL 有无数种等价执行方式，代价天差地别**——这就是优化器存在的理由（见第 7 节的数字演算和 [第 06 章](06_query_optimizer.md)）。
2. **SQL 的正确性可以独立于执行方式讨论**——不管用哪种算法执行，结果必须一样。这让"换更快的算法"成为纯粹的内部优化，用户无感知。

作为对照，你在 Python 里手写 `for row in rows: if row.score > 88: ...` 是**命令式**：你把怎么做一步步写死了，换来的是每次换需求都要重写的苦役。

## 3. ACID 一句话版

事务（Transaction）是把一组读写操作打包成一个"要么全做、要么全不做"的单元。事务必须满足四个性质，合称 ACID。用教科书级例子"账户 A 给账户 B 转账 100 元"来讲：

| 性质 | 一句话 | 转账例子里它保证什么 |
|---|---|---|
| **A**tomicity 原子性 | 事务里的操作要么全部生效，要么全部不生效 | 不会出现"A 扣了 100 但 B 没收到"的中间态落库 |
| **C**onsistency 一致性 | 事务把数据库从一个满足约束的状态带到另一个 | 转账前后，A+B 的总金额不变（约束由应用和数据库共同维护） |
| **I**solation 隔离性 | 并发事务互相看不见对方的中间结果 | 别人查不到"A 已扣、B 未加"那一瞬间的账 |
| **D**urability 持久性 | 一旦提交成功，断电崩溃也不丢 | 银行说转账成功，机房停电后钱也不能凭空消失 |

记忆口径：**A、I、D 是数据库提供的手段，C 是它们合力达成的目的**——一致性最终是业务层面的不变式，数据库用原子性、隔离性、持久性来守护它。这四条每一条背后都是一整块内核子系统：A 靠日志与回滚，I 靠并发控制（MiniOB 用 MVCC，[第 09 章](09_transaction_mvcc.md)），D 靠 WAL（[第 10 章](10_logging_recovery.md)）。

## 4. 一个数据库系统的分层架构

现在把视角从"一张表"拉大到"一个数据库系统"。几乎所有关系型数据库，内部都可以切成下面七层。每层一句话职责，外加一个餐厅类比（把 SQL 当点菜）：

```text
┌──────────────────────────────────────────────────────────┐
│ 接入层    网络收发、连接与会话管理                          │ ← 餐厅前台：接单、上菜
├──────────────────────────────────────────────────────────┤
│ SQL 前端  词法/语法解析 + 语义绑定：SQL 文本 → 结构化语句    │ ← 看懂菜单：确认菜名存在
├──────────────────────────────────────────────────────────┤
│ 优化器    生成候选执行方案并选一个：逻辑计划 → 物理计划       │ ← 后厨调度：先切菜还是先热油
├──────────────────────────────────────────────────────────┤
│ 执行器    按物理计划真正动手，逐行产出结果                   │ ← 厨师炒菜
├──────────────────────────────────────────────────────────┤
│ 事务层    并发控制与提交/回滚，保证 ACID 中的 A 和 I         │ ← 多位厨师共用灶台不打架
├──────────────────────────────────────────────────────────┤
│ 存储层    表、记录、索引在内存与磁盘上的组织                 │ ← 仓库与货架
├──────────────────────────────────────────────────────────┤
│ 日志恢复  先写日志再改数据，崩溃后重演日志恢复               │ ← 流水账：停电照账重做
└──────────────────────────────────────────────────────────┘
```

各层职责再展开一句：

- **接入层**：监听端口、接受连接、把客户端的字节流拆成一条条 SQL 请求，执行完再把结果按协议写回去。
- **SQL 前端**：先做词法/语法分析确认"这句话合不合法"，把字符串变成语法树；再做语义绑定，确认"提到的表、字段、函数真实存在"，产出类型明确的语句对象。
- **优化器**：把语句翻译成关系代数式的**逻辑计划**（要做什么），做等价改写，再选定**物理计划**（具体怎么做：全表扫还是索引扫、哪种连接算法）。
- **执行器**：驱动物理算子树运行。主流执行模型是**火山模型**——每个算子提供 `open/next/close` 接口，上层像拉动火山口一样逐行向下"拉"数据。
- **事务层**：给每个事务发号、决定它能看到哪些版本的数据（隔离），并在提交或回滚时收尾。
- **存储层**：以**页（Page）**为单位组织磁盘文件；用**缓冲池（Buffer Pool）**在内存里缓存热页；记录按行或列摆放在页内；索引（常见 B+ 树）维护"键值 → 记录位置"的映射。
- **日志恢复层**：所有修改先顺序追加到日志（WAL, Write-Ahead Logging），再异步刷数据页；崩溃后重演（redo）日志，把已提交事务的修改补回来。

这个架构的重要特征是**单向依赖**：上层只调用下层的接口，下层不知道 SQL 的存在。执行器不知道页怎么刷盘，存储层不知道什么叫 JOIN。正是这种分层，让你可以只改优化器就获得性能提升，或者只换存储引擎就支持新硬件。

## 5. 一条 SELECT 穿过哪些层

用一条具体 SQL 把分层图走一遍：

```sql
select name, score + 10
from student
where id = 2;
```

概览级旅程（每一步的细节都留给后续章节）：

1. **接入层**：网络线程收到这段文本，包装成一个请求事件，交给 SQL 流水线。
2. **SQL 前端**：词法分析把文本切成 `select`、`name`、`,`……一串 token；语法分析按语法规则组装成语法树——"这是一条 SELECT，列表里有 `name` 列和 `score + 10` 表达式，FROM 是 `student`，WHERE 是 `id = 2`"。语义绑定接着确认：`student` 表存在，`id/name/score` 是它的字段，`+` 两边是数字。产出 `SelectStmt`。
3. **优化器**：生成逻辑计划 `Project(name, score+10) → Predicate(id=2) → TableGet(student)`；物理计划阶段检查 `id` 上有没有可用索引，决定用索引扫描还是全表扫描。
4. **执行器**：`open` 打开算子树；每次 `next`：扫描算子定位到 `id = 2` 的记录 → 过滤算子放行 → 投影算子算出 `score + 10 = 95`，把 `(Bob, 95)` 交给上层。
5. **事务层**：扫描到记录时，事务判断这个版本对当前事务**可见不可见**（MVCC），不可见就跳过。
6. **存储层**：如果走索引，B+ 树从根页向下找到叶子页里的键 `2`，取出记录地址（RID，页号+槽号），再到数据页里把整行读出来；涉及的页都在缓冲池里换入换出。
7. **日志恢复层**：SELECT 是只读的，不写日志——但它能看到的数据恰恰是日志和事务层共同保证过的。

注意 SELECT 的一个特点：**结果不是一次性算完的**，而是客户端每要一行，算子树就被 `next` 拉一次。一张亿行的表查 `limit 10`，可能只做了极少的工作——这是火山模型流式执行的好处，细节见 [第 05 章](05_execution_model.md)。

## 6. 一条 INSERT 穿过哪些层

```sql
insert into student values (4, 'Dave', 102, 78);
```

INSERT 不产生结果集，但它穿过的层比 SELECT 更深——要一直走到日志和磁盘：

1. **接入层 / SQL 前端**：同上，解析绑定后产出 `InsertStmt`，四个值各自转成对应的内部类型（INT、字符串、INT、INT）。
2. **优化器/执行器**：INSERT 通常不生成复杂物理计划，直接由执行器调用存储接口。
3. **事务层**：为这条插入开启（或复用）一个事务，分配事务号；新记录被打上"由哪个事务创建"的标记。
4. **存储层**：按表的模式把四个值编码成一段定长字节（一条物理记录）→ 在数据文件里找一个有空槽的页 → 写入记录，得到它的 RID → 向该表的每一个索引（包括主键索引）插入"键 → RID"的项。主键冲突会在索引插入时被发现并报错。
5. **日志恢复层**：在数据页落盘**之前**，先把"插入了什么"这条 redo 日志顺序写入日志文件。事务提交时保证日志已刷盘——这就是 WAL 的铁律：**先日志，后数据**。
6. 之后的某个时刻，缓冲池里被改脏的数据页才异步刷回磁盘；为防止"半页写"（一页只写了一半就断电），工业实现和 MiniOB 都会借助 double write 之类的机制（[第 01 章](01_buffer_pool.md)）。

把两条 SQL 对比着记：**SELECT 的深度到"读页"为止，INSERT 的深度到"日志+脏页"为止**；它们共享前五层流水线，分歧在执行器之下。

## 7. 复杂度速览：索引和优化器为什么值回票价

为什么分层架构里要专门养一个优化器？用数字算一遍就懂了。

假设 `student` 表有 100 万行，执行 `select * from student where id = 500000`。

**方案 A：全表扫描。** MiniOB 的页大小是 128KB——`src/observer/storage/buffer/page.h:26` 定义 `BP_PAGE_SIZE = (1 << 17)`，即 131072 字节，扣掉页头后数据区 `BP_PAGE_DATA_SIZE` 为 131056 字节。假设每行记录约 100 字节，一页能装约 1300 行，100 万行约 770 页。全表扫描要把这些页全部读一遍，外加 100 万次逐行比较。

**方案 B：B+ 树索引。** `id` 上有索引时，定位走 B+ 树。MiniOB 在 `src/observer/storage/index/bplus_tree.cpp:34` 的 `calc_internal_page_capacity` 里计算内部页容量：每个键值项占 `attr_length + sizeof(RID) + sizeof(PageNum)` 字节，INT 键就是 `4 + 8 + 4 = 16` 字节，于是一个内部页能容纳 `(131056 - 节点头) / 16`，约 8000 个键。扇出按 8000 估算：8000² = 6400 万，已经远大于 100 万——**整棵树只有 2 层**（根页 + 叶子页）。定位一次 = 2 次索引页 I/O + 1 次数据页回表，约 3 页。

```text
全表扫描：约 770 页 I/O        O(N)
索引查找：约   3 页 I/O        O(log N)
差距：约 250 倍，且表越大差距越悬殊
```

这就是第 2.3 节"同一条 SQL 的等价执行方式代价天差地别"的定量含义。优化器的全部工作，就是在执行前选出方案 B 这一类计划；而它凭什么知道有索引可用、凭什么估计两个表 JOIN 谁先谁后，就是 [第 06 章](06_query_optimizer.md) 的主题。顺便记住这组复杂度对比，它是面试最高频的送分题：

| 操作 | 复杂度 | 出处章节 |
|---|---|---|
| 顺序扫描 | O(N) | [第 02 章](02_record_layout.md) |
| B+ 树点查 | O(log N) | [第 03 章](03_bplus_tree.md) |
| 嵌套循环连接 | O(N·M) | [第 07 章](07_join_algorithms.md) |
| 哈希连接 | O(N+M) | [第 07 章](07_join_algorithms.md) |
| 排序 | O(N log N) | [第 08 章](08_sort_and_aggregate.md) |

## 8. MiniOB 的六层映射与目录导读

MiniOB 是 OceanBase 团队开源的教学数据库，完整实现了上面七层。说"六层"是因为在代码组织上，日志（`clog`）与事务（`trx`）都住在 `storage/` 大目录下——这与官方架构图（[MiniOB 架构](../../design/miniob-architecture.md)）和 [项目架构与执行链](../01_project_architecture.md) 的六层划分一致。把第 4 节的抽象分层映射到真实目录和类：

| 抽象层 | MiniOB 目录 | 关键类/文件 |
|---|---|---|
| 接入层 | `src/observer/net` | `SqlTaskHandler`（`sql_task_handler.cpp`）、`Communicator`、`Server` |
| SQL 前端 | `src/observer/sql/parser`、`src/observer/sql/stmt` | `ParseStage`、`lex_sql.l`、`yacc_sql.y`、`ResolveStage`、`ExpressionBinder`、各种 `Stmt` |
| 优化器 | `src/observer/sql/optimizer` | `OptimizeStage`、`LogicalPlanGenerator`、`ExpressionRewriter`、`PhysicalPlanGenerator` |
| 执行器 | `src/observer/sql/operator`、`src/observer/sql/executor` | `PhysicalOperator`（`open/next/close`）、`ExecuteStage`、`SqlResult`、`CommandExecutor` |
| 事务层 | `src/observer/storage/trx` | `Trx`、`MvccTrx`、`VacuousTrx` |
| 存储层 | `src/observer/storage/table`、`record`、`index`、`buffer` | `Table`/`TableMeta`、`RecordFileHandler`、`BplusTreeIndex`、`DiskBufferPool` |
| 日志恢复 | `src/observer/storage/clog` | `DiskLogHandler`、`IntegratedLogReplayer` |

这条流水线在代码里的真实入口是 `SqlTaskHandler::handle_sql`（`src/observer/net/sql_task_handler.cpp:67`），它依次调用各阶段：

```cpp
rc = query_cache_stage_.handle_request(sql_event);  // 查询缓存（当前是空转桩，直接返回 SUCCESS）
rc = parse_stage_.handle_request(sql_event);        // SQL 字符串 -> ParsedSqlNode
rc = resolve_stage_.handle_request(sql_event);      // ParsedSqlNode -> 具体 Stmt，绑定表/字段
rc = optimize_stage_.handle_request(sql_event);     // 逻辑计划 -> 改写 -> 物理算子树
rc = execute_stage_.handle_request(sql_event);      // 有物理计划交给 SqlResult，否则执行 DDL
```

火山模型的统一接口定义在 `src/observer/sql/operator/physical_operator.h`：

```cpp
virtual RC open(Trx *trx) = 0;                  // 初始化资源并打开孩子算子
virtual RC next();                              // 逐行模型：产生下一条结果
virtual RC next(Chunk &chunk);                  // 向量化模型：产生下一批结果
virtual RC close() = 0;                         // 关闭孩子并释放资源
virtual Tuple *current_tuple();                 // 取出当前行
```

几个值得在第一天就建立的源码印象：

- **页很大**：MiniOB 页 128KB，远大于 InnoDB 的 16KB 和 PostgreSQL 的 8KB。这是教学简化，也让 64KB 的 TEXT 字段可以内联存放（赛题 17，见 [第 01](01_buffer_pool.md)、[02 章](02_record_layout.md)）。
- **行列存都有**：记录管理里既有行存的 `RowRecordPageHandler`，也有 PAX 列存的 `PaxRecordPageHandler`（同在 `src/observer/storage/record/record_manager.h`，[第 02 章](02_record_layout.md)）。
- **索引不止 B+ 树**：除 `bplus_tree.h` 外，还有内存态的 `fulltext_index.h`（全文）和 `ivfflat_index.h`（向量），对应赛题 23 和 18（[第 11](11_fulltext_bm25.md)、[12 章](12_vector_search.md)）。
- **MVCC 靠两个隐藏字段**：`MvccTrx` 给每张表追加 `__trx_xid_begin` / `__trx_xid_end` 两个隐藏列（`src/observer/storage/trx/mvcc_trx.cpp:37`），用事务号区间判断版本可见性（[第 09 章](09_transaction_mvcc.md)）。
- **缓冲池带 double write**：`DiskBufferPool` 与 `DiskDoubleWriteBuffer`（`src/observer/storage/buffer/double_write_buffer.h`）配合，防止半页写损坏（[第 01 章](01_buffer_pool.md)）。
- **redo 日志分两层**：`DiskLogHandler` 负责落盘，`IntegratedLogReplayer` 负责崩溃后按模块分发重演（`src/observer/storage/clog/`，[第 10 章](10_logging_recovery.md)）。

一条 SQL 从入口到存储的完整链路梳理，见本系列 [项目架构与一条 SQL 的完整执行链](../01_project_architecture.md)，本章不重复。

## 9. 工业数据库怎么做：InnoDB / PostgreSQL / OceanBase

MiniOB 的分层不是教学玩具的专利，工业数据库是同一副骨架，只是每块肌肉都更强壮：

| 维度 | MiniOB | MySQL InnoDB | PostgreSQL | OceanBase |
|---|---|---|---|---|
| 页大小 | 128KB | 默认 16KB | 默认 8KB | 宏块 2MB 级，微块 16KB 级 |
| 索引组织 | B+ 树存"键→RID"，需回表 | 主键**聚簇索引**：叶子即数据行 | 堆表 + 非聚簇索引，均需回表 | LSM-Tree：MemTable + 分层 SSTable |
| MVCC 实现 | 隐藏事务号区间字段 | undo 日志构造旧版本 + ReadView | 元组头存 xmin/xmax，旧版本留在堆内 | 多版本 + 全局时间戳/快照 |
| 崩溃恢复 | redo 日志（clog） | redo + doublewrite buffer | WAL + full page write | redo 日志 + Paxos 多副本 |
| 优化器 | 规则为主、少量启发式 | 基于成本的优化器（CBO） | CBO，统计信息丰富 | CBO + 分布式计划 |
| 并发 | 教学型 MVCC，隔离级别不完整 | 四种隔离级别、行锁、间隙锁 | MVCC + 谓词锁（SSI） | 分布式事务、两阶段提交 |

读这张表的正确姿势：MiniOB 的每个"简化"，都精确对应工业实现里一块真实的复杂度。比如 MiniOB 的 `DiskDoubleWriteBuffer` 对应 InnoDB 的 doublewrite buffer；MiniOB 的隐藏事务号字段，思路上介于 InnoDB 的 undo 版本链和 PostgreSQL 的堆内多版本之间。面试时被问"你的项目和 MySQL 有什么区别"，按这张表的行来回答就不会散。

## 10. 全教材学习路线：13 章与 24 题

### 10.1 章节地图（完整目录）

本教材共 13 章（00–12），编排顺序是**先沿 SQL 流水线讲清"一条 SQL 怎么跑"，再补事务与日志这两条横切线，最后收束到两个高级索引专题**。完整目录如下（均可点击跳转）：

- [00 数据库内核全景：从一张表到一个数据库系统](00_overview.md)（本章）：分层架构、SQL 之旅、学习路线。
- [01 磁盘、页与 Buffer Pool：数据库的内存管理](01_buffer_pool.md)：页式存储、缓存淘汰、脏页刷盘、double write。支撑赛题 17、24。
- [02 记录如何躺在磁盘上：堆文件、RID 与行列存储](02_record_layout.md)：堆文件组织、页内槽管理、行存与 PAX 列存。支撑赛题 17、13、24。
- [03 索引与 B+ 树：数据库最重要的数据结构](03_bplus_tree.md)：B+ 树原理、键设计、唯一约束。支撑赛题 8、9、1。
- [04 SQL 编译：从字符串到可执行的语句对象](04_sql_frontend.md)：词法/语法分析、表达式绑定、类型挂载。支撑赛题 4、5、6、7、12。
- [05 查询执行：火山模型、向量化与物化](05_execution_model.md)：open/next/close 契约、阻塞算子、内存与物化的关系。支撑赛题 1、15、24。
- [06 查询优化：逻辑计划、重写规则与代价模型](06_query_optimizer.md)：关系代数改写、RBO/CBO、访问路径选择。支撑赛题 5、8、15、18。
- [07 连接算法：Nested Loop、Sort-Merge 与 Hash Join](07_join_algorithms.md)：三种连接的成本模型与外部化。支撑赛题 5、24。
- [08 排序与聚合：从内存排序到外部排序](08_sort_and_aggregate.md)：ORDER BY/GROUP BY 实现、run 生成与多路归并。支撑赛题 10、15、24。
- [09 事务与并发控制：ACID、隔离级别与 MVCC](09_transaction_mvcc.md)：并发异常、快照、版本可见性。支撑赛题 2、9、20。
- [10 日志与崩溃恢复：WAL、Redo 与 Checkpoint](10_logging_recovery.md)：先日志后数据、重演、DDL 的持久性。支撑赛题 3、19、20。
- [11 全文检索：分词、倒排索引与 BM25](11_fulltext_bm25.md)：倒排表、相关性打分、索引接入执行引擎。支撑赛题 23。
- [12 向量检索：距离度量、精确搜索与 ANN](12_vector_search.md)：L2/内积/余弦、IVF-Flat、计划改写。支撑赛题 16、18。

### 10.2 依赖关系图

```text
                        ┌──────────────┐
                        │ 00 全景(本章) │
                        └──────┬───────┘
       ┌───────────────────────┼───────────────────────┐
       ▼                       ▼                       ▼
  存储线（自底向上）       SQL 线（自顶向下）        事务恢复线（横切）
  01 Buffer Pool         04 SQL 前端               09 事务与 MVCC
       │                       │                       │
  02 记录布局                05 执行模型               10 日志与恢复
       │                       │                       │
  03 索引与 B+ 树            06 查询优化                  │
       │                       │                       │
       │                  07 连接算法                    │
       │                       │                       │
       │                  08 排序与聚合                  │
       └──────────┬────────────┴────────────┬──────────┘
                  ▼                         ▼
           11 全文检索 BM25           12 向量检索
```

两条竖线不是孤立的，真正读代码时会反复跨界：

- 05 的扫描算子直接调用 01–03 的页、记录、索引接口（执行线踩在存储线上）；
- 06 的"选不选索引"依赖 03 对 B+ 树键结构的理解；
- 09/10 横切整个存储线：每一次改页都同时牵涉版本可见性（09）和 redo 日志（10）；
- 11/12 是"索引思想 + 执行器/优化器改造"的两个高级实例，分别落地赛题 23 和 16/18。

建议第一轮按编号顺序读，第二轮带着赛题回头精读。

### 10.3 24 道赛题到章节的映射

赛题题面见仓库根目录 [miniob_2025_problems.md](../../../../miniob_2025_problems.md)，逐题精讲解析见 [赛题 1–8](../02_problems_01_08.md)、[赛题 9–16](../03_problems_09_16.md)、[赛题 17–24](../04_problems_17_24.md)。下表给出每题的主修章节（精讲该题所需核心知识）和辅修章节，与各章末尾"回到赛题"小节的认领保持一致：

| 题号 | 赛题 | 主修章节 | 辅修章节 |
|---:|---|---|---|
| 1 | basic | [05 查询执行](05_execution_model.md) | [04](04_sql_frontend.md)、[03](03_bplus_tree.md) |
| 2 | update | [09 事务与 MVCC](09_transaction_mvcc.md) | [05](05_execution_model.md) |
| 3 | drop-table | [10 日志与恢复](10_logging_recovery.md) | [02](02_record_layout.md) |
| 4 | date | [04 SQL 前端](04_sql_frontend.md) | [05](05_execution_model.md) |
| 5 | join-tables | [07 连接算法](07_join_algorithms.md) | [04](04_sql_frontend.md)、[06](06_query_optimizer.md) |
| 6 | expression | [04 SQL 前端](04_sql_frontend.md) | [05](05_execution_model.md) |
| 7 | function | [04 SQL 前端](04_sql_frontend.md) | [05](05_execution_model.md) |
| 8 | multi-index | [03 索引与 B+ 树](03_bplus_tree.md) | [06](06_query_optimizer.md) |
| 9 | unique | [03 索引与 B+ 树](03_bplus_tree.md) | [09](09_transaction_mvcc.md) |
| 10 | group-by | [08 排序与聚合](08_sort_and_aggregate.md) | [05](05_execution_model.md) |
| 11 | simple-sub-query | [05 查询执行](05_execution_model.md) | [04](04_sql_frontend.md)、[06](06_query_optimizer.md) |
| 12 | alias | [04 SQL 前端](04_sql_frontend.md) | — |
| 13 | null | [04 SQL 前端](04_sql_frontend.md) | [02](02_record_layout.md)、[08](08_sort_and_aggregate.md) |
| 14 | union | [05 查询执行](05_execution_model.md) | [06](06_query_optimizer.md)、[08](08_sort_and_aggregate.md) |
| 15 | order-by | [08 排序与聚合](08_sort_and_aggregate.md) | [05](05_execution_model.md)、[06](06_query_optimizer.md) |
| 16 | vector-basic | [12 向量检索](12_vector_search.md) | [04](04_sql_frontend.md) |
| 17 | text | [02 记录布局](02_record_layout.md) | [01](01_buffer_pool.md) |
| 18 | vector-search | [12 向量检索](12_vector_search.md) | [06](06_query_optimizer.md) |
| 19 | alter | [10 日志与恢复](10_logging_recovery.md) | [02](02_record_layout.md)、[01](01_buffer_pool.md) |
| 20 | update-mvcc | [09 事务与 MVCC](09_transaction_mvcc.md) | [10](10_logging_recovery.md) |
| 21 | complex-sub-query | [05 查询执行](05_execution_model.md) | [04](04_sql_frontend.md)、[06](06_query_optimizer.md) |
| 22 | create-view | [05 查询执行](05_execution_model.md) | [04](04_sql_frontend.md) |
| 23 | full-text-index | [11 全文检索](11_fulltext_bm25.md) | [03](03_bplus_tree.md) |
| 24 | big-order-by | [08 排序与聚合](08_sort_and_aggregate.md) | [01](01_buffer_pool.md)、[07](07_join_algorithms.md) |

说明：题 11/13/14/21/22 没有与之同名的专章，它们的原理分散在 SQL 前端、执行模型等章节的小节里，建议以赛题解析文档为主线、回读本教材对应章节打地基。

### 10.4 推荐的三轮读法

```text
第一轮：00 → 01..12 顺序通读，只求建立地图，卡住的概念先标记
第二轮：按上表，每读完一道赛题解析，回读对应主修章节
第三轮：合上资料，对每道题做八步复述（问题→原理→执行链→数据结构
        →算法→复杂度→实现边界→工业改进），再对照原文查漏
```

## 11. 小结

- 数据库因文件的四大痛点而存在：**并发控制、崩溃恢复、声明式查询、数据独立性**。
- 关系模型把数据组织为关系（表）、元组（行）、属性（列），主键唯一标识每一行；SQL 是声明式的，只说"要什么"。
- 事务满足 ACID：原子性、一致性、隔离性、持久性；其中 A、I、D 是手段，C 是目的。
- 数据库内核七层：接入 → SQL 前端 → 优化器 → 执行器 → 事务 → 存储 → 日志恢复，单向依赖；MiniOB 代码把后两层归入 `storage/`，故常称六层。
- SELECT 走通全部七层但止于"读页"；INSERT 要额外走到"先写日志、后刷脏页"。
- 索引把点查从 O(N) 降到 O(log N)——100 万行的表，全表扫约 770 页 I/O，B+ 树点查约 3 页；优化器负责选出这种计划，这是声明式语言必须配优化器的定量理由。
- MiniOB 完整覆盖这套架构，代码主线是 `SqlTaskHandler::handle_sql` 的五段流水线；每个模块在 MySQL/PostgreSQL/OceanBase 里都有对应物。

## 12. 面试追问

**问：什么是数据库内核？**

答：内核是数据库软件中直接负责数据存取与正确性的那部分，相对的是客户端、驱动、运维工具等外围组件。按分层说，内核覆盖 SQL 前端、优化器、执行器、事务、存储和日志恢复；接入层处于内核边界上。以 MiniOB 为例，`src/observer` 下的 observer 进程就是内核：它解析 SQL、规划执行、管理事务和页式存储、写 redo 日志。面试时再补一句"内核的核心矛盾是在正确性（ACID）约束下压榨 I/O 和 CPU 的效率"，就能引出后续任何一层的话题。

**问：SQL 是声明式语言，为什么因此需要优化器？**

答：声明式意味着 SQL 只描述结果集的性质，不指定算法。同一个结果集有大量等价执行方案：全表扫还是索引扫、两表 JOIN 谁做外表、先过滤还是先连接，它们的代价可以差几个数量级——100 万行的表，全表扫约 770 页 I/O，B+ 树点查约 3 页。总得有个组件在这么多方案里选一个，这就是优化器。它先做逻辑改写（等价变换），再基于规则和统计信息生成物理计划。反过来，如果 SQL 是命令式的（用户自己写循环），选择执行方案的责任就在用户身上，也就不存在优化器这个组件了。

**问：为什么数据库自己管理 Buffer Pool，而不用操作系统的页缓存？**

答：三个理由。第一，**替换策略**：数据库知道哪些页以后用不上（比如全表扫描扫过的页），可以用 LRU-K 等策略避免缓存污染，OS 不知道这些语义。第二，**刷盘顺序**：WAL 要求"日志先于数据页落盘"，刷盘时机由恢复逻辑决定，不能交给 OS。第三，**页一致性**：数据库要在页上维护 LSN、校验和，并防半页写（InnoDB 的 doublewrite buffer、MiniOB 的 `DiskDoubleWriteBuffer`），这都必须自己控制写路径。因此工业数据库通常用 `O_DIRECT` 绕过 OS 页缓存，自建缓冲池。展开见 [第 01 章](01_buffer_pool.md)。

**问：ACID 里的一致性（C）和隔离性（I）有什么区别？**

答：一致性说的是**数据满足业务不变式**，比如转账前后总额不变、库存不能为负，它是目的；隔离性说的是**并发事务的执行效果等价于某种串行顺序**，它是手段之一。一个事务单独执行也可能破坏一致性（比如程序写错），隔离性管不着；反过来只有隔离性没有原子性和持久性，一致性也守不住。所以常说 A、I、D 是数据库提供的机制，C 是应用与数据库共同维持的目标。

**问：文件系统也有"块"，数据库的"页"和普通文件块有什么区别？**

答：页是数据库自己定义的 I/O 与组织单位（MiniOB 128KB、InnoDB 16KB、PostgreSQL 8KB），它在文件块之上叠加了结构：页头里有页号、LSN（最后修改它的日志位置，恢复时用来判断要不要 redo）、校验和——MiniOB 的 `BP_PAGE_DATA_SIZE` 就是页大小减去这三样的结果；页内还有槽目录管理一条条记录。文件系统的块只是定长字节，不携带任何这些语义。可以说"页 = 文件块 + 数据库自己的账本信息"。

**问：INSERT 为什么要先写日志再写数据？直接改数据文件不行吗？**

答：直接改数据有两个问题。一是**性能**：数据页的修改是随机 I/O（改哪行刷哪页），而日志是顺序追加，顺序写比随机写快得多，提交时只需保证日志落盘即可返回，数据页攒批异步刷。二是**原子性**：事务改一半宕机，数据页已经部分落盘就说不清了；有了 WAL，重启时按日志 redo 已提交事务、忽略未提交事务，就能恢复到一致状态。这就是"先日志后数据"的 WAL 协议，[第 10 章](10_logging_recovery.md) 会展开讲 MiniOB 的 clog 实现。

## 13. 回到赛题

本章是 24 道赛题的公共地基：每道题都是"某条 SQL 在某几层上的缺口"。建议按下面的分组把赛题解析文档与本教材章节穿插阅读——先读解析里"题目目标"和"完整源码执行链"，遇到不懂的层，回读本教材对应章节：

- [赛题 1–8：基础 SQL、类型、连接、表达式与复合索引](../02_problems_01_08.md)。basic（题 1）是第 5 节 SELECT 旅程的最小完整实例，建议第一个吃透（主修 [05](05_execution_model.md)）；update（题 2）在 SELECT 链路上叠加了表达式逐行求值与事务接口（[09](09_transaction_mvcc.md)、[05](05_execution_model.md)）；drop-table（题 3）展示 DDL 绕过优化器、直接操作元数据与文件，并靠 checkpoint 保证持久性（[10](10_logging_recovery.md)）；date（题 4）、expression（题 6）、function（题 7）都是 SQL 前端表达式与类型体系的扩展（[04](04_sql_frontend.md)）；join-tables（题 5）是连接算法的入门（[07](07_join_algorithms.md)）；multi-index（题 8）是 B+ 树键设计与优化器选索引的交界（[03](03_bplus_tree.md)、[06](06_query_optimizer.md)）。
- [赛题 9–16：唯一性、聚合、子查询、NULL、集合、排序与向量](../03_problems_09_16.md)。unique（题 9）是"索引即约束"（[03](03_bplus_tree.md)、[09](09_transaction_mvcc.md)）；group-by（题 10）、order-by（题 15）对应排序与聚合（[08](08_sort_and_aggregate.md)）；simple-sub-query（题 11）、union（题 14）是执行器里的表达式求值与集合算子（[05](05_execution_model.md)）；alias（题 12）属于绑定阶段的名字解析（[04](04_sql_frontend.md)）；null（题 13）从存储层的一个标记字节贯穿到表达式三值逻辑（[02](02_record_layout.md)、[04](04_sql_frontend.md)）；vector-basic（题 16）是类型系统向向量扩展的第一步（[12](12_vector_search.md)）。
- [赛题 17–24：TEXT、检索、ALTER、MVCC、视图与大数据算子](../04_problems_17_24.md)。text（题 17）依赖 128KB 页内联 64KB 文本的记录布局（[02](02_record_layout.md)、[01](01_buffer_pool.md)）；vector-search（题 18）与 full-text-index（题 23）是两个高级索引专题（[12](12_vector_search.md)、[11](11_fulltext_bm25.md)）；alter（题 19）考验 DDL 与崩溃恢复的配合（[10](10_logging_recovery.md)）；update-mvcc（题 20）是 MVCC 的主战场（[09](09_transaction_mvcc.md)、[10](10_logging_recovery.md)）；complex-sub-query（题 21）是相关子查询的执行与绑定（[05](05_execution_model.md)、[04](04_sql_frontend.md)）；create-view（题 22）是 DDL Executor 与视图扫描算子（[05](05_execution_model.md)）；big-order-by（题 24）是外部排序、连接与内存管理的综合大题（[08](08_sort_and_aggregate.md)、[07](07_join_algorithms.md)、[01](01_buffer_pool.md)）。

口述模拟和更多开放题，见本系列 [推免面试拷打题库](../05_interview_drill.md)；各题关键源码文件索引见 [24 题源码索引与实测证据](../06_source_and_test_index.md)。
