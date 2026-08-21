# 事务与并发控制：ACID、隔离级别与 MVCC

## 本章导读

前面章节解决的是"一条 SQL 怎么跑"的问题：解析、优化、执行、存储。但数据库从来不是一个人在用。当两个客户端同时改同一行数据、一个人在转账 halfway 时另一个人查余额，会发生什么？这就是**事务（Transaction）**要回答的问题。

事务是数据库内核中公认最难、也最爱考的模块。面试里"说说 ACID""隔离级别有哪些""MVCC 是什么"几乎是必问题，而大多数人只会背定义。本章的目标是让你**真正算得出来**：给你三个事务的时间线，你能手动推出每条记录上的版本号是什么、每个事务能看到哪条数据。能算，才算懂。

阅读路径：

- 9.1 用转账例子建立直觉，引出并发执行的四种异常；
- 9.2 严格讲清 ACID 四个性质分别由谁保证；
- 9.3 讲四种隔离级别和快照隔离（SI），用表格记住"谁允许什么反常"；
- 9.4 讲基于锁的并发控制（2PL），它是 MVCC 出现之前的主流方案；
- 9.5 讲 MVCC 原理，并带你手动推演一遍可见性判断；
- 9.6 讲 MVCC 下 UPDATE/DELETE 的语义、唯一约束为什么难、垃圾回收为什么必需；
- 9.7 落到 MiniOB 源码：`Trx`/`TrxKit`、`MvccTrx` 的每一个函数在做什么；
- 9.8 复杂度分析；9.9 对比 InnoDB、PostgreSQL、OceanBase；
- 最后是面试追问和"回到赛题"。

本章对应的官方设计文档是 `docs/docs/design/miniob-transaction.md`（相对路径 [../../design/miniob-transaction.md](../../design/miniob-transaction.md)），源码在 `src/observer/storage/trx/` 目录下，代码量很小，值得全文通读。

## 9.1 事务要解决什么问题

### 9.1.1 从转账说起：原子性

假设银行表 accounts 里有两行：A 有 100 元，B 有 100 元。A 给 B 转 50 元，对应的 SQL 是：

```sql
UPDATE accounts SET balance = balance - 50 WHERE name = 'A';
UPDATE accounts SET balance = balance + 50 WHERE name = 'B';
```

如果在第一条 UPDATE 执行完、第二条还没执行时数据库宕机了，重启后 A 少了 50 元，B 却没多出来——50 元凭空消失。这两条 UPDATE 必须**要么都生效，要么都不生效**，不允许停在中间状态。把这样一组不可分割的操作打包，就是一个事务：

```sql
BEGIN;
UPDATE accounts SET balance = balance - 50 WHERE name = 'A';
UPDATE accounts SET balance = balance + 50 WHERE name = 'B';
COMMIT;   -- 或 ROLLBACK：全部撤销
```

这是事务存在的第一个理由：**原子性（Atomicity）**。

### 9.1.2 并发执行的四种异常

数据库会有很多客户端同时连接。如果两个事务的操作交错执行（这在多线程数据库里是常态），会出什么问题？设事务 T1、T2 并发，x 是某行数据：

**脏读（Dirty Read）**：T1 改了 x 但还没提交，T2 读到了这个新值。随后 T1 回滚，T2 读到的值从未真实存在过。

```text
T1:  UPDATE x = 200   (未提交)
T2:                    SELECT x  → 200   ← 读到了"不存在过"的数据
T1:  ROLLBACK         (x 恢复为 100)
```

**不可重复读（Non-Repeatable Read）**：T1 在同一个事务里读两次 x，期间 T2 修改并提交了 x，导致 T1 两次读到不同的值。

```text
T1:  SELECT x → 100
T2:             UPDATE x = 200; COMMIT;
T1:  SELECT x → 200   ← 同一个事务里，两次结果不一样
```

**幻读（Phantom Read）**：与不可重复读类似，但差别在于 T2 不是改已有行，而是**插入/删除了满足 T1 查询条件的行**。T1 用同样的 WHERE 条件查两次，第二次多出了"幻影"行。

```text
T1:  SELECT * FROM accounts WHERE balance > 50  → 2 行
T2:             INSERT 一行 balance=80; COMMIT;
T1:  SELECT * FROM accounts WHERE balance > 50  → 3 行  ← 多出一行幻影
```

**写-写冲突（Write-Write Conflict）与丢失更新（Lost Update）**：T1 和 T2 同时改同一行。最经典的错误是"读-改-写"交错：

```text
初始 x = 100
T1:  读 x = 100
T2:             读 x = 100
T1:  写 x = 100 + 50 = 150
T2:             写 x = 100 + 30 = 130   ← T1 的 +50 被覆盖，丢了
```

两个事务都基于旧值 100 计算，后写的覆盖先写的，一个更新丢失。转账系统里这就是真金白银的事故。

这四种异常就是**隔离性（Isolation）**要解决的问题。怎么解决？最极端的办法是所有事务排队一个个执行（串行执行），这样什么异常都没有——但数据库就慢得没法用了。并发控制的全部艺术，就是在"正确"和"快"之间找平衡点。

## 9.2 ACID 逐个拆解

ACID 是事务的四个基本性质。注意：它们不是平级的四个目标，理解它们的"责任方"是面试加分点。

**A — 原子性（Atomicity）**：事务内的操作要么全部生效，要么全部不生效。由数据库的**恢复机制**保证：未提交的事务崩溃后靠日志回滚（undo），已提交的靠日志重做（redo）。MiniOB 中对应 `MvccTrx::rollback()` 里的逆向补偿和 `storage/clog` 的 WAL。

**C — 一致性（Consistency）**：这是最容易背错的一个。一致性指的是"数据库从一个满足业务约束的正确状态，迁移到另一个正确状态"——比如转账前后总金额不变、账户余额不为负。**这些约束是应用层定义的，数据库并不知道"总金额要守恒"**。准确的说法是：一致性是应用层的目标，数据库提供 A、I、D 三个工具，应用开发者用这三个工具把业务约束保住。如果应用把两条 UPDATE 拆开不放在一个事务里，数据库再强也救不了。

**I — 隔离性（Isolation）**：并发事务互不干扰，效果上等同于某种串行顺序。这是**并发控制模块**的目标，也是本章的重头戏：锁（9.4）和 MVCC（9.5）都是实现隔离性的手段。隔离性不是非黑即白，而是一组可以权衡的级别（9.3）。

**D — 持久性（Durability）**：事务一旦 COMMIT 成功返回，它的修改即使宕机也不能丢。由 **WAL（Write-Ahead Logging）** 保证：提交前先把日志刷盘，数据页可以慢慢写。MiniOB 的 `MvccTrxLogHandler::commit()` 注释里明确写了"会等待日志落地"，这就是在为持久性买单。

一句话总结：**AID 是数据库保证的手段，C 是应用借助 AID 达到的目标**。面试时说清这一句，比背定义强得多。

## 9.3 隔离级别与快照隔离

### 9.3.1 四种标准隔离级别

完全不隔离（快但不安全）和完全串行（安全但慢）是两个极端。SQL 标准在中间定义了四档，逐级增强：

| 隔离级别 | 脏读 | 不可重复读 | 幻读 | 含义 |
|---|---|---|---|---|
| Read Uncommitted (RU) | 可能 | 可能 | 可能 | 读完全不看别人是否提交 |
| Read Committed (RC) | 禁止 | 可能 | 可能 | 只读已提交的数据，但每条语句看到的可以不同 |
| Repeatable Read (RR) | 禁止 | 禁止 | 可能 | 事务内多次读同一行结果相同 |
| Serializable | 禁止 | 禁止 | 禁止 | 效果等价于某个串行执行顺序 |

记法：级别越高，允许的反常越少，并发度越低。两个实务要点：

- **MySQL InnoDB 默认 RR，PostgreSQL 默认 RC**。InnoDB 的 RR 通过 next-key lock 实际上把幻读也挡住了大部分，这是面试常见考点。
- 隔离级别说的是"**允许哪些反常**"，不是"怎么实现"。同一个 RC，可以用锁实现也可以用 MVCC 实现。

### 9.3.2 快照隔离（Snapshot Isolation, SI）与写偏斜

MVCC 数据库实际提供的往往不是标准四级，而是另一个模型：**快照隔离**。规则很简单：

- 事务开始时拍一张"快照"：此刻已提交的数据对它可见，之后别人提交的都不可见；
- 事务整个生命周期都读这张快照，所以脏读、不可重复读、幻读（对快照内的重扫描）天然不存在；
- 写-写冲突用"先提交者赢"（first-committer-wins）解决：两个事务改同一行，后提交的那个回滚。

SI 看起来很完美，但它**不等价于串行化**，有一种特有的反常叫**写偏斜（Write Skew）**。例子：银行约束 A + B >= 0，初始 A = 100, B = 100。

```text
T1: 读 A=100, B=100，判断 A+B=200 >= 200，允许 A 取出 200
    UPDATE A = A - 200        (只写 A 这一行)
T2: 读 A=100, B=100（自己的快照），同样判断通过
    UPDATE B = B - 200        (只写 B 这一行)
```

T1 和 T2 写的是**不同的行**，没有写-写冲突，都能提交。最终 A = -100, B = -100，A + B = -200，约束被破坏。两个事务各自读了共同的前提、写了不同的数据、合起来违反了约束——这就是写偏斜。它说明：SI 阻止了"写写打架"，但阻止不了"读写之间隐含的依赖"。工业界的解法是**可串行化快照隔离（SSI）**：在 SI 之上追踪读写依赖，发现危险结构时主动回滚一个事务（PostgreSQL 的 Serializable 级别就是这么做的）。

## 9.4 基于锁的并发控制

在 MVCC 流行之前，数据库用锁实现隔离性。这套理论至今仍是面试主干，而且 MVCC 也并没有完全替代锁（写-写冲突最终还是要某种互斥）。

### 9.4.1 S 锁与 X 锁

最基本的两种锁：

| | 读（S 锁） | 写（X 锁） |
|---|---|---|
| **读（S 锁）** | 兼容 | 冲突 |
| **写（X 锁）** | 冲突 | 冲突 |

- **S 锁（共享锁）**：读数据前加。多个事务可以同时持有同一行的 S 锁（大家一起读没关系）。
- **X 锁（排他锁）**：写数据前加。同一行同一时刻只允许一个事务持有 X 锁，且此时别人连 S 锁都拿不到。

冲突时后到者**等待**，直到持锁者释放。

### 9.4.2 两阶段锁（2PL）

只规定锁的种类还不够，还要规定**什么时候加锁、什么时候放锁**。两阶段锁协议（Two-Phase Locking）：

- **增长阶段**：事务可以不断申请新锁，但不能释放任何锁；
- **收缩阶段**：开始释放锁后，就不能再申请新锁。

实践中普遍用的是**严格两阶段锁（Strict 2PL）**：所有锁一直持有到事务 COMMIT/ROLLBACK 才一次性释放。这样自然没有收缩阶段的复杂性，还能防止级联回滚。定理：**2PL 能保证冲突可串行化**——也就是说，只要大家都守 2PL 规矩，任何交错执行的结果都等价于某个串行顺序。

代价也很明显：读和写互相阻塞，并发度差；读一个报表要锁住所有扫过的行，转账这类短事务会被长查询拖死。这正是 MVCC 要解决的痛点。

### 9.4.3 死锁

锁会等待，等待就可能成环：

```text
T1: 锁住 A，想要 B
T2: 锁住 B，想要 A
```

两个事务都在等对方手里的锁，谁都走不了——**死锁（Deadlock）**。处理策略分两类：

- **检测**：维护一张"等待图"（wait-for graph），节点是事务，边是"在等谁"。后台线程定期在图上找环，找到环就牺牲（回滚）其中一个事务，让别人活。InnoDB、PostgreSQL 都用这种。
- **预防**：从制度上杜绝环。比如 **wait-die**（老事务等年轻人，年轻事务等老年人就直接自杀重试）和 **wound-wait**（老事务直接抢走年轻人的锁，年轻人等老事务）——核心都是给事务按年龄定一个全局偏序，让等待关系永远朝一个方向，环就不可能形成。

工程上还有兜底手段：**锁超时**，等太久直接报错回滚（MySQL 的 `innodb_lock_wait_timeout`）。

## 9.5 MVCC 原理

### 9.5.1 直觉：为什么不删旧数据

2PL 的痛点是"读阻塞写、写阻塞读"。MVCC（Multi-Version Concurrency Control，多版本并发控制）换了个思路：

> **写数据时不覆盖旧值，而是造一个新版本；读数据时根据自己的快照挑一个合适的旧版本读。**

类比 git：你改文件不是把旧文件抹掉，而是提交一个新 commit；别人随时可以 checkout 任意历史版本，互不干扰。

效果立竿见影：

- **读（快照读）完全不加锁**：读的是旧版本，写得再热闹也不影响；
- **写不阻塞读，读不阻塞写**：只剩写-写之间需要互斥；
- 只读事务的并发度大幅提升——这是 MVCC 成为主流的根本原因。

### 9.5.2 两种版本组织方式

"多版本"具体怎么存，有两大流派：

```text
流派一：版本链（InnoDB 风格）
  主表只存最新版本 → 旧版本被推进 undo log，用指针串成链
  row(v3, trx=15) --roll_ptr--> undo: v2(trx=12) --> undo: v1(trx=8)
  读快照太老时，沿着链往回找

流派二：行内区间（PostgreSQL / MiniOB 风格）
  每个版本就是表里一条完整记录，行头带两个隐藏字段：
  [ begin_xid | end_xid | 用户数据... ]
  表示"这个版本在 [begin_xid, end_xid] 这段时间内有效"
  UPDATE = 把旧行 end_xid 封掉 + 新插一行完整记录
```

- 版本链的优点是主表干净、读最新版快；代价是读旧版本要沿链回溯，且需要 purge 线程清理 undo。
- 行内区间的优点是所有版本地位平等、回滚容易（旧版本就在原地）；代价是表会随着更新不断膨胀，**垃圾回收（GC）成为刚需**。

### 9.5.3 可见性判断：手动推演一遍

这是本章最重要的技能点。采用行内区间模型，规则如下：

- 每个事务有一个单调递增的事务号 trx_id；
- 每个版本带 `[begin_xid, end_xid]`：begin 是创建它的事务的提交号，end 是删除它的事务的提交号，未删则为 +∞（实现里用 int32 最大值）；
- 读事务用自己的 trx_id 当快照：版本对它可见，当且仅当 `trx_id ∈ [begin_xid, end_xid]`（MiniOB 用闭区间）；
- 未提交的特殊标记：负号表示"还没提交"。`-T` 表示事务 T 正在操作、尚未提交。

推演（这就是 MiniOB 的实际语义，9.7 节会对照源码）：

```text
时间线（trx_id 从 1 开始分配）：

t1  TA 启动，id=1
t2  TA: INSERT 行 R(data=100)      → R: [begin=-1, end=+∞]   （未提交，仅自己可见）
t3  TA: COMMIT，分配 commit_id=2   → R: [begin=2,  end=+∞]   （转正，正式生效）
t4  TB 启动，id=3
t5  TC 启动，id=4（纯读事务）
t6  TB: UPDATE R 改为 200：
        旧版本 R:  [begin=2,  end=-3]   （标记"事务3准备结束它"）
        新版本 R': [begin=-3, end=+∞]   （标记"事务3刚插入、未提交"）
t7  TC: SELECT R  → 能看到哪个版本？
t8  TB: COMMIT，分配 commit_id=5
        R:  [begin=2, end=5]    （旧版本正式失效）
        R': [begin=5, end=+∞]   （新版本正式生效）
t9  TC: 再 SELECT R → ？
t10 TD 启动，id=6，SELECT R → ？
```

逐个手动算：

**t7，TC（id=4）读**：

- 看 R：`begin=2 > 0`，`end=-3 < 0`，落在"end 为负"分支——有人正在删它但还没提交，对只读者来说**视为还存在**；且动手的 `-(-3)=3 ≠ 4`，不是 TC 自己删的 → **可见**。
- 看 R'：`begin=-3 < 0`，落在"begin 为负"分支——未提交的新版本，只有创建者自己可见；`3 ≠ 4` → 不可见。
- 结论：TC 读到 **100**（旧版本）。TB 未提交的修改对 TC 完全透明。

**t9，TC 再读**（TB 已提交）：

- R：`begin=2, end=5` 都为正，`4 ∈ [2,5]` → 可见。
- R'：`4 ∈ [5, +∞]`？不在 → 不可见。
- 结论：TC **还是读到 100**。同一个事务里两次读结果一致——不可重复读被快照天然挡住了。

**t10，TD（id=6）读**：

- R：`6 ∈ [2,5]`？不在 → 不可见。
- R'：`6 ∈ [5,+∞]` → 可见。
- 结论：TD 读到 **200**。TD 启动于 TB 提交之后，看到新世界。

整个过程 TC 没加任何锁，TB 的写也没被 TC 阻塞。这就是"读写互不阻塞"的精确含义。你能独立把这个例子从头算到尾，MVCC 就算入门了。

## 9.6 MVCC 下的 UPDATE/DELETE、唯一约束与垃圾回收

### 9.6.1 UPDATE 和 DELETE 的版本语义

行内区间模型下：

- **DELETE 不物理删除**，只把 `end_xid` 从 +∞ 改成删除者的提交号。旧版本还躺在表里，因为可能还有老事务的快照需要读它。
- **UPDATE = DELETE + INSERT**：旧版本封 `end_xid`，新版本以完整一行插入（拿到新的物理位置/RID）。一次逻辑 UPDATE 产生两个物理版本，这就是**写放大**。
- 索引也要为新版本插新 entry；旧版本的 entry 暂时留着，靠可见性过滤——索引随之膨胀。

### 9.6.2 唯一约束为什么难

这是 MVCC 面试的高级题。唯一索引的重复检查发生在**物理层**（B+ 树插入时探测同 key），但 MVCC 的可见性判断在**事务层**。两者错位：

- T1 删除了 key=5 的行（未提交），T2 想插入 key=5。物理上旧版本的索引项还在，T2 会被判重复——但 T1 可能回滚，那时 T2 的插入本来应该成功。反过来 T1 提交了，T2 又必须失败。
- 正确处理需要"看到未提交数据的事务状态"：T2 插入时发现冲突，要**等 T1 尘埃落定**（提交则冲突报错，回滚则放行）。PostgreSQL 的 speculative insertion、InnoDB 的锁等待重复检查，解决的都是这件事。
- 教学实现（包括 MiniOB）通常不做这层等待逻辑，于是 MVCC 并发下的唯一性保证是有缺陷的——这正是赛题 9 的已知边界。

### 9.6.3 垃圾回收为什么必需

旧版本不能永生。当再也没有任何活跃事务的快照能"看到"某个旧版本时，它就是**垃圾**，必须回收，否则：

- 表和索引无限膨胀（update 一次多一行）；
- 扫描时要跳过越来越多的死版本，读放大越来越严重。

回收的判据：找到当前**最老的活跃快照**（所有在读事务中最小的快照号，PostgreSQL 叫 xmin horizon），`end_xid` 小于它的版本对任何人都不可见，可以物理删除、回收空间。PostgreSQL 的 VACUUM、InnoDB 的 purge 线程干的都是这个活，只是回收对象不同（堆内旧版本 vs undo log 版本链）。

## 9.7 MiniOB 源码实现

现在把上面的原理对号入座。MiniOB 事务代码在 `src/observer/storage/trx/` 下，全部加起来一千行左右，是通读源码的最佳切入点。

### 9.7.1 接口设计：Trx 与 TrxKit

`src/observer/storage/trx/trx.h` 定义了两个抽象类：

- **`TrxKit`**：事务管理器工厂。`enum Type { VACUOUS, MVCC }`；静态方法 `TrxKit::create(const char *name)` 按名字创建。`src/observer/storage/trx/trx.cpp` 里可以看到：名字为空或 `"vacuous"` 创建 `VacuousTrxKit`，`"mvcc"` 创建 `MvccTrxKit`。启动参数 `-t mvcc`（`src/observer/main.cpp` 第 70 行解析）一路传到 `DefaultHandler::init`，从而选择事务模型。默认是 Vacuous。
- **`Trx`**：事务接口。注意它的 DML 方法签名和 Table 几乎一一对应，但**夹在算子和存储之间**：

```cpp
virtual RC insert_record(BaseTable *table, Record &record)                         = 0;
virtual RC delete_record(BaseTable *table, Record &record)                         = 0;
virtual RC update_record(BaseTable *table, Record &old_record, Record &new_record) = 0;
virtual RC visit_record(BaseTable *table, Record &record, ReadWriteMode mode)      = 0;
virtual RC start_if_need() = 0;
virtual RC commit()        = 0;
virtual RC rollback()      = 0;
```

设计意图（`trx.h` 的注释写得很明白）：DML 必须先经过事务层而不许算子直接调 Table。`VacuousTrx` 把这些调用原样转发给 Table（什么都不做）；`MvccTrx` 在转发前后维护版本字段、操作列表和日志。**上层算子完全不用感知当前是哪种事务实现**——这是面向接口编程隔离变化点的典型例子。

`visit_record` 是 MVCC 的灵魂：扫描器（`RecordFileScanner`，见 `src/observer/storage/record/record_manager.cpp`）每取出一条物理记录，都调它判断"这个版本对我可见吗"。不可见返回 `RC::RECORD_INVISIBLE`，扫描器跳过继续找下一条——**可见性过滤对算子是透明的**。`ReadWriteMode`（`src/observer/common/types.h` 第 37 行，枚举值 `READ_ONLY`/`READ_WRITE`）区分这次访问是只读还是想写，因为写路径要额外做冲突检查。

### 9.7.2 隐藏版本字段：`__trx_xid_begin` / `__trx_xid_end`

`MvccTrxKit::init()`（`src/observer/storage/trx/mvcc_trx.cpp` 第 33-54 行）给**每张表**追加两个隐藏字段：

```cpp
fields_ = vector<FieldMeta>{
    FieldMeta("__trx_xid_begin", AttrType::INTS, 0, 4, false /*visible*/, -1, false),
    FieldMeta("__trx_xid_end",   AttrType::INTS, 0, 4, false /*visible*/, -2, false)};
```

逐行看：

- 两个字段都是 4 字节 INT，行头各存一个事务号；
- `visible=false` 表示对用户不可见——`SELECT *` 不会查出这两列；
- `field_id` 为 -1、-2 是特殊标记，与用户字段（从 0 开始）区分。

这就是 9.5.2 说的**行内区间流派**：每条记录头部带 `[__trx_xid_begin, __trx_xid_end]` 有效区间。MiniOB 在这一点上**和 PostgreSQL 的 xmin/xmax 是同一个设计**，而不是 InnoDB 的 undo log 版本链。

### 9.7.3 负值约定：未提交标记

MiniOB 用一个非常简洁的编码区分"已提交"和"未提交"：**正数是已提交的边界，负数的绝对值是正在操作它的事务号**。`MvccTrx` 里（`mvcc_trx.h` 第 108 行）`MAX_TRX_ID` 即 `numeric_limits<int32_t>::max()`（约 21 亿），扮演 +∞ 的角色。

`MvccTrx::insert_record`（`mvcc_trx.cpp` 第 140-161 行）开头三行：

```cpp
begin_field.set_int(record, -trx_id_);
end_field.set_int(record, trx_kit_.max_trx_id());
RC rc = table->insert_record(record);
```

- 第 1 行：新版本的 begin 写成 `-trx_id_`——"事务 trx_id_ 刚插入、还没提交"；
- 第 2 行：end 写成 INT32_MAX——"有效到世界尽头"；
- 第 3 行：才真正把记录插进表（`Table::insert_record` 内部还会插所有索引项）。

`MvccTrx::delete_record`（第 163-199 行）对称：先在 `visit_record` 的 READ_WRITE 检查通过后，把旧版本的 `end_xid` 原地改成 `-trx_id_`——"本事务准备结束它"。**没有物理删除**。

### 9.7.4 commit 转正与 rollback 补偿

`MvccTrx::commit()`（第 363-369 行）只做一件事：

```cpp
int32_t commit_id = trx_kit_.next_trx_id();
return commit_with_trx_id(commit_id);
```

**提交时重新分配一个新的 commit_id**，而不是用事务自己的 trx_id。为什么？因为版本区间要表达"从提交这一刻起生效"，而事务的启动号在提交号之前；如果用启动号，会出现"启动晚于它、却早于它提交的事务"落进可见区间的尴尬。commit_id 由同一个原子计数器 `current_trx_id_` 分配，保证了提交顺序与事务号全序一致。

`commit_with_trx_id`（第 371-481 行）遍历本事务的 `operations_` 列表，逐个把 `-trx_id_` 转正：

- INSERT：新版本 `begin_xid` 从 `-trx_id_` 改为 `commit_xid`；
- DELETE：旧版本 `end_xid` 从 `-trx_id_` 改为 `commit_xid`；
- UPDATE：旧版本封 end、新版本开 begin，两个都改成 `commit_xid`。

最后调 `log_handler_.commit(trx_id_, commit_xid)` 写提交日志并**等待落盘**（持久性）。

`rollback()`（第 483-618 行）用 `std::ranges::reverse_view` **逆序**遍历操作列表做补偿：INSERT 的物理删掉，DELETE 的把 `end_xid` 恢复为 +∞，UPDATE 的删新版本、恢复旧版本。这就是原子性的实现：任何已做的修改都能精确撤销。

### 9.7.5 visit_record：可见性判断的完整逻辑

`MvccTrx::visit_record`（第 275-331 行）是 9.5.3 推演的代码版，三个分支：

```cpp
if (begin_xid > 0 && end_xid > 0) {
  // 已提交版本：trx_id 落在 [begin, end] 内可见
  rc = (trx_id_ >= begin_xid && trx_id_ <= end_xid) ? RC::SUCCESS : RC::RECORD_INVISIBLE;
} else if (begin_xid < 0) {
  // 未提交新版本：只有创建者自己可见
  rc = (-begin_xid == trx_id_) ? RC::SUCCESS : RC::RECORD_INVISIBLE;
} else if (end_xid < 0) {
  // 有人正在删除/更新它但还没提交
  ...
}
```

第三个分支再按读写分开（第 305-328 行）：

- **READ_ONLY**：别人正在删还没提交——对我这个读者来说它"还在"，可见（除非动手的正是我自己）；这就是"写不阻塞读"；
- **READ_WRITE**：发现 `end_xid` 是别人的负号，直接返回 `RC::LOCKED_CONCURRENCY_CONFLICT`——**写-写冲突，立即报错（fail-fast），不等待**。MiniOB 把冲突抛给上层回滚，而不是像工业库那样排队等锁。

对照 9.5.3 的时间线：t7 时刻 TC 读 R 走的是第三个分支的 READ_ONLY 路径，读 R' 走的是第二个分支。源码里的注释也坦言："当前实现使用事务号近似快照时间，并采用较简单的冲突处理策略"。

### 9.7.6 UPDATE 的 MVCC 化（2025 赛题扩展）

`MvccTrx::update_record`（第 201-273 行）把 9.6.1 的语义完整实现了：

1. 在 `table->visit_record` 的受保护上下文里做 READ_WRITE 可见性检查，通过后把旧版本 `end_xid` 改为 `-trx_id_`（检查与修改之间不留竞争窗口）；
2. 新版本设置 `begin=-trx_id_, end=+∞`，调 `table->insert_record(new_record)` 插入（拿到新 RID，索引项一并插入）；
3. 若插入失败，把旧版本的 `end_xid` 恢复原值再返回错误；
4. 记录一条 `Operation::Type::UPDATE`（带新旧两份 Record 的克隆），供 commit/rollback 使用。

注意官方设计文档 `docs/docs/design/miniob-transaction.md` 写于 2024 年，里面还说"当前 miniob 仅实现了插入和删除，并不支持更新操作"——**这句话已经过时**，2025 赛季的代码（本仓库）把 UPDATE 补上了，这本身就是赛题 20 的核心工作。

### 9.7.7 事务日志与恢复

`src/observer/storage/trx/mvcc_trx_log.h` 定义了四种日志：`INSERT_RECORD`、`DELETE_RECORD`、`COMMIT`、`ROLLBACK`。两个细节值得记住：

- `MvccTrxRecordLogEntry` 只有 `table_id + rid`，**不记录行数据**——行内容的物理恢复靠 record 层自己的日志，事务日志只记"哪个事务动了哪行"；
- **没有 UPDATE 日志类型**：UPDATE 的 redo 只能靠 insert 那条新版本的日志近似覆盖，这是当前已知缺陷；
- `MvccTrxLogHandler::commit` 会等日志落盘，`rollback` 不等（回滚丢了大不了再滚一次）；
- 恢复时 `MvccTrxLogReplayer` 按事务收集操作，重放结束后（`on_done`）对**没有提交记录的事务统一回滚**——WAL 场景下"提交即持久、未提交即回滚"的标准做法。

### 9.7.8 当前缺陷清单（面试要主动说）

MiniOB 的 MVCC 是教学简化版，缺陷官方文档自己也承认（`mvcc_trx.h` 第 65 行直接写着 `TODO 没有垃圾回收`）：

- **无 GC**：旧版本永不回收，表和索引随 UPDATE 单调膨胀；
- **提交不是原子可见的**：`commit_with_trx_id` 逐行转正，中途被另一个新事务看到时，会出现"一个事务的修改看见一半"（设计文档给了 R1/R2/R3 的具体反例）；
- **隔离级别不完整**：没有显式实现 RU/RC/RR/Serializable 任何一档。读路径用事务启动号当快照、整个事务有效，行为上**最接近快照隔离（SI）**，但受上面两条影响并不严格；
- **写冲突 fail-fast**：不等待、不死锁检测，冲突即报错，可用性靠上层重试；
- **UPDATE 无独立 WAL**、**索引新旧版本 entry 共存导致膨胀**（正确性靠读时过滤维持）。

## 9.8 复杂度分析

- **可见性判断**：每行 O(1)——读两个 int 字段、几次比较。代价转移到扫描端：若一行累积 V 个版本，一次全扫的读放大是 O(V) 倍。
- **INSERT/DELETE**：正常存储开销之外，多写 8 字节版本字段，O(1) 额外开销；DELETE 是原地改标记，O(1)。
- **UPDATE**：一行变两行，写放大 O(record_size)；新版本还要在每个索引上插 entry，O(k·log n)（k 个索引）。
- **COMMIT/ROLLBACK**：遍历本事务操作列表，O(操作数)。每个操作是一次定位修改，O(1)~O(log n)。
- **空间**：无 GC 时表大小 ∝ 插入数 + 更新数，单调增长——这是行内区间模型不配 GC 的必然下场。
- **并发度**：读写互不阻塞，冲突面只剩"同时写同一行"，相比 2PL 是质变；但 fail-fast 策略在高冲突下会放大重试成本。

## 9.9 工业界对比

| | MiniOB | PostgreSQL | InnoDB (MySQL) |
|---|---|---|---|
| 版本存储 | 行内 `__trx_xid_begin/end` | 行内 `xmin/xmax` | 主表最新版 + undo log 版本链 |
| 提交状态 | 正负号编码 | 独立 CLOG 查提交状态 | ReadView 活跃事务列表 |
| 快照 | 事务启动号近似 | Snapshot（xmin/xmax/活跃列表） | ReadView（RC 每语句新建，RR 事务级） |
| 旧版本位置 | 原表 | 原表（堆内） | undo 表空间 |
| GC | 无 | VACUUM（autovacuum） | purge 后台线程 |
| 写冲突 | fail-fast | 行锁等待 + 死锁检测 | 行锁等待 + 死锁检测 |

三个记忆点：

- **MiniOB 是 PG 风格**：版本在行内、旧版本留原表、回滚几乎零成本（不用应用 undo），但缺 VACUUM。面试说"我们的 MVCC 更像 PostgreSQL 而不是 InnoDB"，并解释为什么（回滚便宜、膨胀贵），是高分回答。
- **InnoDB 的 ReadView** 值得单独准备：`m_ids`（创建 ReadView 时活跃事务列表）、`min_trx_id`、`max_trx_id`、`creator_trx_id` 四个字段；判断规则是"版本 trx_id < min 可见、>= max 不可见、中间看在不在 m_ids 里"。RC 每条 SELECT 新建 ReadView（所以能读到别人刚提交的），RR 用事务级 ReadView（所以可重复读）——同一个机制，不同创建时机，产出两种隔离级别。
- **OceanBase** 作为分布式数据库，还要解决"跨机器的快照一致性"：用全局时间戳服务（GTS）发统一的版本号，单机 LSN/事务号那一套在分布式下不够用。MVCC 思想相同，工程复杂度高一个量级。

## 小结

- 事务存在的理由：原子性（转账不能转一半）和隔离性（并发不出脏读/不可重复读/幻读/丢失更新）。
- ACID 里 A、I、D 是数据库提供的手段，C 是应用层借这些手段达到的目标。
- 隔离级别是"允许哪些反常"的权衡表；MVCC 实际提供的是快照隔离，要警惕写偏斜。
- 2PL 用 S/X 锁 + "锁持有到提交"保证可串行化，代价是读写互斥和死锁；死锁靠等待图检测或 wait-die/wound-wait 预防。
- MVCC 核心一句话：**写造新版本、读按快照挑版本，读写互不阻塞**。两种组织：undo 版本链（InnoDB）与行内区间（PG/MiniOB）。
- MiniOB 实现要点：`Trx`/`TrxKit` 接口隔离两种事务模型；`__trx_xid_begin/end` 负值表未提交、commit 时转正；`visit_record` 三分支做可见性与写冲突判断；缺陷是无 GC、提交非原子可见、隔离级别不完整。
- 能手动推演可见性（9.5.3 的时间线），比会背任何定义都重要。

## 面试追问

**问：MVCC 下 SELECT 为什么不加锁？写和读真的完全不互斥吗？**

答：因为读的是快照。写事务造新版本，旧版本原封不动；读事务按自己的快照号挑一个已提交的旧版本读，双方操作的不是同一份数据，自然不需要互斥。注意这不包括"当前读"（`SELECT ... FOR UPDATE`、UPDATE 里的读），当前读必须读最新版本，仍要参与写-写互斥。MiniOB 里就是 `visit_record` 的 READ_WRITE 分支：遇到别人的负号标记直接报 `LOCKED_CONCURRENCY_CONFLICT`。

**问：InnoDB 的 RC 和 RR 都是用 MVCC 实现的，区别在哪？**

答：区别只在 ReadView 的创建时机。RR 在事务第一次快照读时创建一个 ReadView 并用到底，所以事务内多次读结果一致；RC 每条 SELECT 都新建 ReadView，所以能看到别人在这条语句之前刚提交的数据。同一个可见性算法，快照拍得勤不勤而已。

**问：MiniOB 提交时为什么要重新分配 commit_id，直接用事务启动时的 trx_id 不行吗？**

答：版本区间 `[begin_xid, end_xid]` 的语义是"从哪个时刻起对后来的事务可见"，这个时刻必须是**提交时刻**。启动号在提交号之前：若用启动号，一个"启动晚于本事务、但本事务尚未提交"的事务，其事务号会落进这个区间，从而看到未提交的数据——脏读。commit_id 从同一个计数器分配，让版本可见性严格按提交顺序排列。

**问：旧版本什么时候可以物理删除？MiniOB 做了吗？**

答：当它对任何活跃事务的快照都不可见时——具体判据是 `end_xid` 小于当前最老活跃事务的快照号（xmin horizon）。MiniOB **没有做 GC**（`mvcc_trx.h` 里明确 TODO），旧版本和它的索引项会永久堆积，这是行内区间模型最严重的工程缺口。工业实现：PostgreSQL 用 VACUUM 扫表回收，InnoDB 用 purge 线程按 ReadView 水位清理 undo log。

**问：快照隔离已经挡住了脏读、不可重复读和幻读，为什么还说它不是可串行化？**

答：因为它挡不住写偏斜。两个事务读了共同的数据前提、分别写不同的行、合起来违反业务约束——经典例子是两个账户各扣 200、总额变负。SI 只检测"同一行上的写-写冲突"，检测不到这种跨行的读写依赖。解法是 SSI（可串行化快照隔离）：追踪读写依赖形成的危险结构，发现时回滚一个事务，PostgreSQL 的 Serializable 就是这么实现的。

**问：MiniOB 的写冲突处理（fail-fast）和 InnoDB 的行锁等待，各有什么取舍？**

答：fail-fast 实现极简单，永远不会有死锁，也不需要锁管理器和等待图；代价是冲突时整个事务前功尽弃，高冲突负载下重试风暴。行锁等待能保住后到者的工作，吞吐更平稳，但要维护锁表、等待队列、死锁检测，复杂度高一个量级。教学系统选 fail-fast 是合理简化；说清楚这个取舍比背"哪个更好"重要。

## 回到赛题

本章是三道赛题的理论底座：

- **赛题 20 update-mvcc**（[../04_problems_17_24.md](../04_problems_17_24.md)）：本章的直接应用。`MvccTrx::update_record` 把"旧版本封 end + 新版本插 begin"的 UPDATE 语义落地，commit 转正、rollback 补偿的整条链路就是 9.7.4/9.7.6 的内容。该题的已知缺陷（无 UPDATE 日志、索引膨胀、无 GC）也都在 9.7.8 列全了。
- **赛题 2 update**（[../02_problems_01_08.md](../02_problems_01_08.md)）：默认 `VacuousTrx` 下的单字段 UPDATE 主链路。学完本章要能讲清 2 和 20 的分界：同一套 `Trx::update_record` 接口，VacuousTrx 直接物理覆盖（无版本、无并发保护），MvccTrx 走多版本——这正是 9.7.1 接口设计的意义。
- **赛题 9 unique**（[../03_problems_09_16.md](../03_problems_09_16.md)）：唯一性检查发生在 B+ 树物理插入时（`src/observer/storage/index/bplus_tree_index.cpp` 中 `RC::RECORD_DUPLICATE_KEY`），感知不到 MVCC 的事务状态。9.6.2 讲的"唯一约束为什么难"就是这题在 MVCC 并发下有缺陷的根因：正确做法需要对未提交冲突做等待/判定，而当前实现没有。
