# 日志与崩溃恢复：WAL、Redo 与 Checkpoint

## 本章导读

前面章节讲的都是数据库"活着"的时候怎么工作：页在 Buffer Pool 里改，事务靠 MVCC 并发。但数据库进程随时可能死——coredump、`kill -9`、机器掉电。这一章回答一个问题：**崩溃之后，数据库凭什么保证已提交的数据不丢、没提交的事务不留半截？** 答案是三件相互配合的武器：WAL（Write-Ahead Logging，预写日志）、Checkpoint（检查点）与 Double Write（双写）。

这一章是面试的绝对高频区。"什么是 WAL""redo 和 undo 的区别""为什么 commit 很快但数据没丢""torn page 怎么解决"，几乎场场必问。赛题方面，update-mvcc（题 20）的每一条版本变更都靠 redo 日志保证崩溃一致性，drop-table（题 3）和 alter（题 19）这类 DDL 则依赖"DDL 后立即 checkpoint"的机制保证原子性，本章最后的"回到赛题"会逐一对应。

阅读本章前不需要任何日志系统的背景知识，我们会从"记账"这个生活例子开始，一步步推到 MiniOB 源码 `src/observer/storage/clog/` 下的真实实现。阅读路径：

- 先建立直觉：崩溃到底会弄坏什么（三个问题对应三种机制），再理解 WAL 为什么快（顺序写 vs 随机写）；
- 然后严格化：日志内容的三派（物理/逻辑/生理）、LSN 与页 LSN、commit 协议与 group commit；
- 接着讲恢复：checkpoint 如何限定恢复起点、ARIES 三阶段、撕裂页与 double write 的配合；
- 最后落到源码：`LogHandler`/`DiskLogHandler`、四个模块各自的日志、`BplusTreeMiniTransaction`、`IntegratedLogReplayer` 与 `Db::recover`，并坦承当前实现的缺口。

本章对应的官方设计文档是 `docs/docs/design/miniob-durability.md`（相对链接 [../../design/miniob-durability.md](../../design/miniob-durability.md)），建议与源码一起读。

## 崩溃时会发生什么：三个问题

先用一个生活类比建立直觉。假设你在用 Excel 记账，账本文件在硬盘上，你改动的数字先在内存里，点"保存"才写回硬盘。现在突然停电了，会发生三件糟糕的事：

1. **刚改的数字没了**：你改了十个格子，只保存过一次——内存里的修改全丢了。对应数据库：**已提交事务修改的页还在 Buffer Pool 里，没来得及刷盘**。
2. **文件写了一半**：停电的瞬间正好在保存，文件前半是新的、后半是旧的，整个文件损坏打不开了。对应数据库：**一个数据页只写了一半到磁盘（torn page，撕裂页）**。
3. **一笔转账做了一半**：你从 A 账户扣了 100 块，还没加到 B 账户就停电了。对应数据库：**未提交事务的部分修改已经落盘**，重启后数据处于中间态。

数据库内核用三种机制分别应对，记住这张对应表，全章都围绕它展开：

| 崩溃造成的问题 | 应对机制 | 一句话原理 |
| --- | --- | --- |
| 已提交事务的修改只改了内存，没刷盘 | **redo 日志 + WAL** | 修改先记日志，崩溃后重放日志把修改"再做一遍" |
| 数据页写了一半（torn page） | **double write + checksum** | 页先整体写到双写区，再写真实位置；坏页从双写区恢复 |
| 未提交事务做了一半 | **undo / 事务日志回滚** | 重启时识别出"没提交完"的事务，把它的修改逆向撤销 |

这里有一个关键的设计权衡叫 **steal / no-force**，面试常考名词解释：

- **steal（偷取）**：Buffer Pool 允许把"未提交事务"修改过的脏页提前刷盘（比如内存不够要淘汰页）。这要求系统有 undo 能力，否则这个事务回滚时磁盘上的脏数据没法清除。
- **no-force（不强制）**：事务提交时**不要求**把它改过的数据页刷盘。这要求系统有 redo 能力，否则提交后崩溃数据就丢了。

steal + no-force 是性能最好的组合：刷盘时机完全由系统自己把握，与事务边界解耦。代价是恢复时必须同时具备 redo（重做已提交事务）和 undo（回滚未提交事务）两套能力。经典的 ARIES 恢复算法就是 steal + no-force，MiniOB 的 Buffer Pool 同样是 steal/no-force 模式——脏页随时可能被淘汰刷盘，而 commit 只等日志落盘，后面都会看到代码证据。

## WAL 原则：先写日志，后写数据页

WAL 的规则只有一句话：**任何数据页在被写回磁盘之前，描述这次修改的日志必须已经先持久化到磁盘。**

用记账类比：你改账本之前，先在一个按顺序追加的"流水便签"上写一行"3 月 5 日，把 A 账户余额从 500 改成 400"。便签永远只在末尾追加，写起来飞快；账本本身可以慢慢誊抄。就算誊到一半停电，只要便签在，重启后照着便签把没誊完的补上即可。

为什么这样快？因为**顺序写比随机写快几个数量级**。算一笔具体的账（机械硬盘）：

- 一次事务提交，假设修改了 3 个数据页（表页 + 索引页），它们散落在磁盘各处。如果 commit 时必须把这 3 个页刷盘，就是 3 次**随机写**。机械硬盘一次随机写约 5~10 毫秒（寻道 + 旋转延迟），3 次约 15~30 毫秒，即单连接每秒最多提交约 30~60 个事务。
- 改用 WAL：commit 时只需把这个事务的日志（可能只有几百字节）**顺序追加**到日志文件末尾。顺序写没有寻道开销，机械硬盘可达 100MB/s 量级，一次追加的耗时主要是 fsync 本身的固定开销（约 1 毫秒级）。TPS 上限立刻提高一个数量级以上，而且还可以用 group commit 继续摊薄（后文详述）。

即使在 NVMe SSD 上，随机写与顺序写的差距缩小了，但"一次小日志写"和"多次页面写"的 I/O 次数差距、以及 SSD 内部小随机写的写放大，仍然让 WAL 稳赚不赔。

WAL 把"持久化"的成本从 O(修改的页数 × 随机写延迟) 降到了 O(1 次顺序追加)，这是现代数据库提交快的根本原因。注意 WAL 约束的是**顺序**（日志必须先于数据页落盘），而不是说日志要立刻写——日志本身也可以先放内存缓冲区攒批。

MiniOB 中，这个"先日志后页"的检查点非常集中，全部逻辑只有一行，在 `src/observer/storage/buffer/buffer_pool_log.cpp`：

```cpp
RC BufferPoolLogHandler::flush_page(Page &page) { return log_handler_.wait_lsn(page.lsn); }
```

每个内存页都带一个 `lsn` 字段（见 `src/observer/storage/buffer/page.h` 的 `struct Page`：页开头依次是 `LSN lsn` 和 `CheckSum check_sum`，之后是数据区），记录"最后一次修改本页的日志的编号"。刷页之前调用 `wait_lsn(page.lsn)`，等到编号不超过 `page.lsn` 的日志全部落盘，才允许写页。调用点在 `DiskBufferPool::flush_page_internal`（`src/observer/storage/buffer/disk_buffer_pool.cpp`）：先 `log_handler_.flush_page(frame.page())` 等日志，再算 checksum、交给 double write buffer 写盘——WAL、撕裂页保护在同一个函数里串起来了。

## Redo 日志记什么：物理、逻辑与生理日志

"记日志"听起来简单，但日志里到底记什么内容，有三种经典选择，面试必考对比：

| 类型 | 记录内容 | 例子 | 优点 | 缺点 |
| --- | --- | --- | --- | --- |
| 物理日志（physical） | 页内哪些字节改成了什么 | "页 P5 偏移 100~116 的字节改为 `<二进制>`" | 重做简单、幂等 | 日志量大，与页结构强耦合 |
| 逻辑日志（logical） | 做了什么操作 | "向表 t 插入一行 (1,'abc')" | 日志量小 | 重做时要重新执行操作，代价高、难以幂等 |
| 生理日志（physiological） | 定位到页，页内按逻辑记 | "向页 P5 的 3 号槽插入记录 `<整行>`" | 折中：日志较小，重做高效且幂等 | 实现略复杂 |

工业界（InnoDB、ARIES）主流选择是**生理日志**：定位粒度到页（物理），页内的操作按逻辑描述。因为页内结构是数据库自己管的，重做时不需要重新走一遍 B+ 树查找，直接把记录放进指定槽位就行，天然幂等。

MiniOB 的日志基本就是这个思路。看 `src/observer/storage/record/record_log.h` 里的 `RecordLogHeader`：每条记录日志带 `buffer_pool_id`（哪个文件）、`page_num`（哪页）、`slot_num`（哪个槽）加上操作类型；插入和更新日志还会附带完整的新行数据。重做时按"页号 + 槽号"直接覆盖写入，与页当前内容无关——这就是幂等的来源。而事务层的日志（`MvccTrxLogHeader`）则是**逻辑日志**：只记"事务 X 在表 T 的 RID 处插入/删除了一条记录"，具体行数据靠 Record Manager 层的日志恢复，官方设计文档 `../../design/miniob-durability.md` 明确说"整个事务日志是一个逻辑日志，而不是物理日志"，并强调日志系统是**分层**的：事务日志依赖 Record Manager 和 B+ 树日志，后两者又依赖 Buffer Pool 日志。

### LSN：日志的序号与重做的边界

每条日志都有一个 **LSN（Log Sequence Number，日志序列号）**，一个全局单调递增的整数，每产生一条日志就加 1。LSN 有两个作用：

1. **确定顺序**：日志 105 描述的修改一定发生在 103 之后，重放时按 LSN 从小到大依次执行即可。
2. **标记重做边界**：每个数据页的页头记录一个**页 LSN**——"最近一次修改本页的日志的 LSN"。重放某条日志时，若它的 LSN ≤ 目标页的页 LSN，说明这次修改已经在页里了，**直接跳过**。

第二条是 redo 幂等性的关键，也让恢复可以从任意早的位置开始扫描而不会重复执行。跟着算一遍这个例子：

```
时间线（LSN 单调递增）：
  LSN 100:  checkpoint，此刻所有脏页已刷盘
  LSN 101:  页 P5，插入记录 A        （内存中 P5 的页 LSN 变为 101）
  LSN 103:  页 P5，更新记录 B        （内存中 P5 的页 LSN 变为 103）
  LSN 105:  页 P8，删除记录 C        （内存中 P8 的页 LSN 变为 105）
  —— P5 被淘汰刷盘过一次，P8 没刷过 —— 然后崩溃
```

崩溃后磁盘上的状态：P5 页头 LSN = 103（101 和 103 的修改都已在盘上），P8 页头 LSN = 60（105 的修改丢了）。恢复时从 LSN 100 开始扫描：

- 日志 101 → 目标页 P5，页 LSN 103 ≥ 101，**跳过**；
- 日志 103 → 页 LSN 103 ≥ 103，**跳过**；
- 日志 105 → 目标页 P8，页 LSN 60 < 105，**重做**，把 P8 的页 LSN 提升到 105。

MiniOB 里的对应代码在 `src/observer/storage/record/record_log.cpp` 的 `RecordLogReplayer::replay`：写日志成功时顺手 `frame->set_lsn(lsn)` 更新页 LSN；重放时先比较：

```cpp
const LSN frame_lsn = frame->lsn();
if (frame_lsn >= entry.lsn()) {
  // 页面已经包含这条日志的修改，跳过重放
  return RC::SUCCESS;
}
```

这几行就是上面整个数字例子的代码化身。Buffer Pool 与 B+ 树的重放器也有同样的 LSN 比较逻辑。

## 提交协议：commit 只需把日志刷盘

把前面的点串起来，一个事务的提交流程是：

1. 事务执行过程中，每次修改页之前/之后，把 redo 日志追加到**日志缓冲区**（内存）；
2. commit 时，追加一条 COMMIT 日志，然后**等待日志缓冲区中本事务的所有日志刷盘（fsync）**；
3. 向客户端返回"提交成功"。此时数据页**一个字节都不用刷**，继续留在 Buffer Pool 里，等以后被淘汰或 checkpoint 时再刷。

"commit 只等日志、不等数据页"是数据库性能的关键设计：它把事务的持久化延迟锁定为一次（或几次）日志 fsync，而不是若干次随机页写。只要 COMMIT 日志落了盘，这个事务就绝不会丢——崩溃后重放日志一定能把它重做回来。

### Group Commit：把 fsync 摊薄

fsync 有固定成本（约几十微秒到毫秒级），如果每个事务 commit 都独立 fsync 一次，那么 TPS 上限 ≈ 1 / fsync 延迟。比如 fsync 要 1 毫秒，TPS 就被钉死在 1000。

**Group commit** 的思路：让短时间内先后到达的多个 commit 合并成一次 fsync。算一笔账：100 个事务在 10 毫秒内先后提交，各自刷盘要 100 次 fsync；如果把它们的日志攒在一起一次刷盘，100 个 commit 只花 1 次 fsync 的成本，每个事务平均摊到 0.01 毫秒，整体吞吐上限从 1000 TPS 提升到 100000 TPS。代价是单个事务的提交延迟略微上升（要等一小会儿凑批），典型的"延迟换吞吐"。

MiniOB 有这一思想的朴素版本。提交路径在 `src/observer/storage/trx/mvcc_trx_log.cpp` 的 `MvccTrxLogHandler::commit`：先 `append` COMMIT 日志，然后 `log_handler_.wait_lsn(lsn)` 死等它落盘才返回。而 `DiskLogHandler`（`src/observer/storage/clog/disk_log_handler.cpp`）内部是"前台线程只写内存缓冲区 `LogEntryBuffer`，后台线程 `thread_func` 不停地把缓冲区刷到日志文件"——多个事务的日志天然在缓冲区里汇合，由后台线程成批写出。不过它的等待方式很粗暴：

```cpp
RC DiskLogHandler::wait_lsn(LSN lsn)
{
  // 直接强制等待。在生产系统中，我们可能会使用条件变量来等待。
  while (running_.load() && current_flushed_lsn() < lsn) {
    this_thread::sleep_for(chrono::milliseconds(100));
  }
  ...
}
```

轮询 + 睡眠 100 毫秒，意味着 MiniOB 一次 commit 最坏可能多等 100 毫秒——教学实现可以容忍，生产系统会用条件变量在刷盘完成的瞬间精确唤醒等待者。

## Checkpoint 与恢复流程

如果日志无限增长，崩溃恢复就要从头扫到尾，恢复时间不可控。**Checkpoint（检查点）** 解决的就是这个问题：选一个时间点，把当时所有脏页刷盘、所有日志落盘，然后记下"这一刻之前的日志所描述的修改都已经在磁盘上了"，记下一个 LSN 作为恢复的起点。以后崩溃，只需从这个 LSN 开始重放，之前的日志可以归档或删除。

### ARIES 三阶段：分析、Redo、Undo

工业界的标准恢复算法是 ARIES（1992 年那篇著名论文），恢复分三个阶段：

```
 崩溃前                                                崩溃       恢复
  |---- 事务T1(已提交) ----|                              |          |
  |---- 事务T2(未提交) --------x                           |          |
  |================| 最后 checkpoint                       |          |
                   |<--------- 恢复只需扫这段日志 --------->|
                   阶段1 分析：扫描日志，重建"脏页表"和"活跃事务表"，
                          找出崩溃时还没提交完的事务（loser）
                   阶段2 Redo：从最早的脏页对应 LSN 起，把所有日志
                          重做一遍——包括 loser 的（重复历史，页 LSN 跳过已生效的）
                   阶段3 Undo：把 loser 事务的修改按 LSN 逆序逐个撤销，
                          撤销动作本身也写日志（CLR，补偿日志），保证可重入
```

三个要点：**分析**阶段确定"哪些页可能脏、哪些事务没做完"；**redo** 阶段"重复历史"，把数据库恢复到崩溃瞬间的物理状态（不管事务提交与否）；**undo** 阶段再把没提交的事务清干净。redo 在前、undo 在后，且 redo 阶段连未提交事务的修改也一起重做——因为 undo 需要先在正确的页面上操作。

MiniOB 没有完整实现 ARIES，但骨架都在：

- **Checkpoint 是"全量快照式"的**：`Db::sync`（`src/observer/storage/db/db.cpp`）把所有表的数据页刷盘、等当前所有日志落盘，然后把当前 LSN 记为 `check_point_lsn_`，写入 db 的元文件（先写 `.tmp` 临时文件再 rename，保证元文件本身的原子性）。注意 MiniOB 要求 sync 期间没有任何正在进行的事务，这是人工约束而非程序强制。
- **恢复从 `check_point_lsn_` 起回放**：`Db::recover` 调用 `log_handler_->replay(log_replayer, check_point_lsn_)`，只扫检查点之后的日志。
- **redo + 简单的 undo**：日志按模块分发重做（redo），全部重做完后，事务回放器把"有操作日志但没有 COMMIT/ROLLBACK 日志"的事务找出来逐个回滚（undo）。

`Db::recover`（`src/observer/storage/db/db.cpp`）的核心几行：

```cpp
IntegratedLogReplayer log_replayer(*buffer_pool_manager_, unique_ptr<LogReplayer>(trx_log_replayer));
RC rc = log_handler_->replay(log_replayer, check_point_lsn_ /*start_lsn*/);
...
rc = log_handler_->start();        // 回放结束后才启动后台刷日志线程
...
rc = log_replayer.on_done();       // 各模块收尾；事务层在这里回滚未提交事务
```

未提交事务的识别在 `src/observer/storage/trx/mvcc_trx_log.cpp` 的 `MvccTrxLogReplayer`：它用 `trx_map_`（事务 ID → 事务对象）收集回放中遇到的事务，遇到 COMMIT 或 ROLLBACK 日志就从表里删掉；`on_done()` 时表里剩下的就是崩溃时未提交的事务，逐个 `trx->rollback()`——这就是 MiniOB 的"undo 阶段"。

## Torn Page 与 Double Write

还有一个 WAL 解决不了的问题：**页本身写一半**。MiniOB 的页是 128KB（`BP_PAGE_SIZE = 1 << 17`，见 `src/observer/storage/buffer/page.h`），InnoDB 默认 16KB，而磁盘只能保证一个扇区（512 字节）或一个块（4KB）的原子写。写一个 128KB 的页要拆成许多次小块写，如果中途断电，磁盘上的页就是"新一半旧一半"的撕裂页（torn page）。

撕裂页为什么可怕？因为 redo 日志是"页内操作"，它假设页的其他部分是完好的；面对一个物理损坏的页，重放日志也救不回来——redo 重做得了"逻辑修改"，修不了"页本身烂了"。

检测手段是 **checksum**：页头存一个 CRC32 校验和，读页时重新算一遍，对不上就说明页坏了。修复手段是 **double write（双写）**：

```
正常刷页：  内存页 --①先整体写入--> 双写区（连续磁盘空间，顺序写）
                    --②再写入--> 数据文件中的真实位置（随机写）
崩溃恢复：  读真实位置的页 → checksum 失败 → 从双写区把完好的副本拷回去
            （双写区的页也带 checksum，双写区自己写一半时直接丢弃该副本）
```

双写区是连续空间的批量顺序写，成本远低于随机写，所以这套机制的开销可以接受。MySQL InnoDB 的 `innodb_doublewrite` 就是这个机制。PostgreSQL 用了另一个思路：**full_page_writes**——checkpoint 后每个页的第一次修改，把整个页镜像写进 WAL，撕裂页直接用 WAL 里的完整镜像恢复（代价是 WAL 变大）。

MiniOB 实现了双写，代码在 `src/observer/storage/buffer/double_write_buffer.cpp` 的 `DiskDoubleWriteBuffer`：刷页时页先 `add_page` 进双写 buffer，攒批后先写双写文件、再写真实位置；重启时 `Db::init_dblwr_buffer` 调 `DiskDoubleWriteBuffer::recover()`（等价于把双写区里完好的页写回真实位置），其中 `load_pages` 逐个读双写区的页并用 `crc32(page.data, BP_PAGE_DATA_SIZE)` 校验，校验不过的副本直接丢弃。注意恢复顺序：MiniOB 先做双写恢复、再回放 redo 日志（`Db::init` 中 `init_dblwr_buffer()` 在 `recover()` 之前）——先把"页本身"修完好，再谈"重放修改"，顺序不能反。

## MiniOB 源码走读：clog 模块

现在把通用原理落到 MiniOB 的具体实现上。日志模块在 `src/observer/storage/clog/` 下，官方设计文档是 `../../design/miniob-durability.md`。整体架构：

```
                        LogHandler（接口，log_handler.h）
                        ┌─────────────┴─────────────┐
              DiskLogHandler（落盘）      VacuousLogHandler（不记日志，调试用）
                        │
   append() ──> LogEntryBuffer（内存缓冲区，log_buffer.h）
                        │  后台线程 thread_func 循环刷盘
                        ▼
              LogFileManager / LogFileWriter / LogFileReader（log_file.h）
                        │
              磁盘上的 clog_0.log, clog_1000.log, ...（每 1000 条日志一个文件）
```

### 日志的格式

一条日志在代码里是 `LogEntry`（`src/observer/storage/clog/log_entry.h`），由定长的 `LogHeader` 加二进制负载组成：

```cpp
struct LogHeader final
{
  LSN     lsn;        /// 日志序列号 log sequence number
  int32_t size;       /// 日志数据大小，不包含日志头
  int32_t module_id;  /// 日志模块编号
};
```

`module_id` 是 MiniOB 的一个特色设计：日志按**产生它的模块**分类（`LogModule::Id`：`BUFFER_POOL`、`BPLUS_TREE`、`RECORD_MANAGER`、`TRANSACTION`），各模块自定义负载格式、各自负责重放。用官方文档里 `clog_dump` 工具的真实输出感受一下（`../../design/miniob-durability.md`）：

```
lsn=2, size=12, module_id=0:BUFFER_POOL, buffer_pool_id=1, page_num=1, operation_type=0:ALLOCATE
lsn=3, size=16, module_id=2:RECORD_MANAGER, operation_type:0:INIT_PAGE, page_num=1, record_size=20
lsn=4, size=36, module_id=2:RECORD_MANAGER, operation_type:1:INSERT, page_num=1, slot_num=0
lsn=5, size=20, module_id=3:TRANSACTION, operation_type:0:INSERT_RECORD, trx_id:3, ...
lsn=7, size=12, module_id=3:TRANSACTION, operation_type:2:COMMIT, trx_id:3, commit_trx_id: 4
```

可以看到一次"建表后插入一行并提交"产生的完整日志链：Buffer Pool 分配页 → Record Manager 初始化页、插入记录 → 事务层记 INSERT_RECORD → COMMIT。

### 写入路径：缓冲区 + 后台刷盘线程

`DiskLogHandler`（`src/observer/storage/clog/disk_log_handler.h/.cpp`）的写入路径：

1. 业务线程调 `append` → `_append` → 日志进入内存缓冲区 `LogEntryBuffer`（`src/observer/storage/clog/log_buffer.h`：一个 `deque<LogEntry>` 加互斥锁，分配 LSN 并推进 `current_lsn_`）；
2. 后台线程 `thread_func` 死循环：把缓冲区里的日志通过 `LogFileWriter` 顺序写入当前日志文件，写满 1000 条（`max_entry_number_per_file = 1000`）就通过 `LogFileManager::next_file` 切到下一个文件 `clog_<起始LSN>.log`；
3. 需要等刷盘的人（commit、checkpoint、刷页）调 `wait_lsn(lsn)`，等 `flushed_lsn_` 追上来。

一个值得注意的细节：日志文件是以 `O_SYNC` 标志打开的（`LogFileWriter::open`，`src/observer/storage/clog/log_file.cpp`：`O_WRONLY | O_APPEND | O_CREAT | O_SYNC`），每次 `write` 系统调用本身就要等数据落到磁盘才返回。所以 MiniOB 的"刷盘"语义很直接——后台线程把日志写出去就是持久化了，不需要再显式 fsync；代价是每次写都是同步 I/O，吞吐上还有优化空间。

这正是"日志先攒在内存、成批顺序落盘"的 group commit 骨架，只是如前文所说，等待与唤醒用轮询实现，比较粗糙。

### 四个模块各记什么日志

| 模块（文件） | 日志类型 | 记录内容要点 |
| --- | --- | --- |
| Buffer Pool（`src/observer/storage/buffer/buffer_pool_log.h`） | `ALLOCATE` / `DEALLOCATE` 页面 | `BufferPoolLogEntry`：buffer_pool_id + 页号；改的是文件头元数据页 |
| Record Manager（`src/observer/storage/record/record_log.h`） | `INIT_PAGE` / `INSERT` / `DELETE` / `UPDATE` | `RecordLogHeader`：文件 + 页号 + 槽号；INSERT/UPDATE 附完整行数据 |
| B+ 树（`src/observer/storage/index/bplus_tree_log.h`） | 节点分裂/合并/插入/删除等十余种 | 一次操作的多页改动聚合成**一条**大日志（见下） |
| 事务 MVCC（`src/observer/storage/trx/mvcc_trx_log.h`） | `INSERT_RECORD` / `DELETE_RECORD` / `COMMIT` / `ROLLBACK` | 逻辑日志：事务 ID + 表 ID + RID，不记行数据 |

前三者是"系统日志"，与具体页绑定、由系统自己发起；事务日志是"用户日志"，由用户的事务边界驱动，大小和时长都不确定——设计文档里专门讨论过这两类日志的差异。

### BplusTreeMiniTransaction：多页操作的原子落盘

B+ 树有个别的模块没有的问题：一次插入可能引起叶子分裂、父节点插入、祖先连锁分裂，**涉及多个页**。如果为每页记一条独立日志，崩溃可能发生在两条日志之间，树就残了。MiniOB 的解法是 `BplusTreeMiniTransaction`（`src/observer/storage/index/bplus_tree_log.h`），思路类似 InnoDB 的 mtr（mini-transaction）：

- 操作执行期间，所有页修改一边改内存、一边把日志（连同用于回滚的 undo 数据）攒在内存的 `BplusTreeLogger::entries_` 里，修改过的页全部拴住不刷盘；
- `commit()` 时把攒下的所有日志**合并成一条大日志**一次性 append——日志落盘的原子粒度就是"一条"，因此这次多页操作要么整体重放、要么整体不出现；
- 操作中途失败，`rollback()` 用内存里攒的 undo 数据把已改的页改回去。

这是"用一条日志的写入原子性，换来一个多页操作的原子性"，非常精巧，也是面试里"B+ 树分裂如何保证崩溃一致"的标准答案素材。

### 重放的分发：IntegratedLogReplayer

恢复时，各类日志混在一起顺序读出，由 `IntegratedLogReplayer`（`src/observer/storage/clog/integrated_log_replayer.h`）按 `module_id` 分发：

```cpp
RC IntegratedLogReplayer::replay(const LogEntry &entry)
{
  switch (entry.module().id()) {
    case LogModule::Id::BUFFER_POOL:    return buffer_pool_log_replayer_.replay(entry);
    case LogModule::Id::RECORD_MANAGER: return record_log_replayer_.replay(entry);
    case LogModule::Id::BPLUS_TREE:     return bplus_tree_log_replayer_.replay(entry);
    case LogModule::Id::TRANSACTION:    return trx_log_replayer_->replay(entry);
    default: return RC::INVALID_ARGUMENT;
  }
}
```

每个子回放器只解析自己模块的负载格式。全部日志重放完后 `on_done()` 依次回调各回放器收尾——事务回放器就是在这一步回滚未提交事务的。

### 完整启动恢复链

把 `Db::init`（`src/observer/storage/db/db.cpp`）里的启动顺序串一遍，就是 MiniOB 的完整恢复流程：

1. `LogHandler::create` + `init`：按配置创建日志处理器（`-d disk` 用 `DiskLogHandler`，`-d vacuous` 不记日志），日志目录形如 `miniob/db/sys/clog/`；
2. `init_meta`：从 db 元文件读出上次 checkpoint 的 `check_point_lsn_`（文件里就存这一个数字）；
3. `open_all_tables`：打开所有表的数据文件和索引文件；
4. `init_dblwr_buffer`：双写恢复，把双写区里完好的页写回真实位置（修撕裂页）；
5. `recover`：构造 `IntegratedLogReplayer`，从 `check_point_lsn_` 开始回放全部日志（redo），回放完启动后台刷日志线程，最后 `on_done()` 回滚未提交事务（undo）。

### 动手实验：亲眼看一次崩溃恢复

理解恢复最好的方式是亲手制造一次崩溃。按官方设计文档给出的步骤（[../../design/miniob-durability.md](../../design/miniob-durability.md)）：

1. 以 MVCC + 磁盘日志模式启动服务端：`./bin/observer -f ../etc/observer.ini -s miniob.sock -t mvcc -d disk`（`-d disk` 表示用 `DiskLogHandler` 把日志记录到磁盘，`-d vacuous` 则不记日志）；
2. 用客户端连上，建表、插入若干数据并提交——此时可以看到 `miniob/db/sys/clog/` 目录下的 `clog_0.log` 在增长；
3. 执行 `kill -9` 强杀 observer 进程（模拟断电，不给它任何刷盘机会）；
4. 用同一条命令重启，服务端自动进入本章讲的恢复流程：先双写恢复、再从 `check_point_lsn_` 回放日志；
5. 重新连上客户端查询，已提交的数据都还在——这就是 WAL 给你的保证。

还可以用 `tools/clog_dump.cpp` 编译出的 `clog_dump` 工具直接查看日志文件内容：`./bin/clog_dump miniob/db/sys/clog/clog_0.log`，输出形如前文展示的那样，每条日志的 LSN、所属模块、操作类型一目了然。对着真实日志文件读源码，是熟悉 clog 模块最快的方式。

## 复杂度与性能分析

- **写入放大**：WAL 意味着每次修改至少写两处（日志 + 最终的数据页），双写又加一处（双写区）。用额外的写放大换取 commit 的低延迟与崩溃可恢复，是存储系统的经典取舍。
- **commit 延迟**：理想情况下等于一次日志 fsync 的延迟，与事务修改的页数无关——这是 no-force 策略的红利。MiniOB 因轮询等待会额外加上最多约 100 毫秒的睡眠粒度。
- **恢复时间**：redo 耗时 ≈ O(checkpoint 之后的日志量)，所以 checkpoint 越频繁，恢复越快，但 checkpoint 本身的刷盘开销越大——这是"运行时开销 vs 恢复速度"的权衡。ARIES 的 fuzzy checkpoint 可以边服务边做；MiniOB 的 `Db::sync` 要求系统静默，属于最重的 sharp checkpoint。
- **undo 代价**：MiniOB 只回滚"有日志但没提交完"的事务，代价与未提交事务的日志量成正比；已提交事务零代价（redo 已保证其生效）。
- **日志空间**：MiniOB 的日志文件只增不减（没有删除接口），长时间运行会无限膨胀——工业实现会用 checkpoint 截断日志（如 InnoDB 的循环 redo 文件）。

## 工业数据库怎么做

**MySQL InnoDB**：redo log 是生理日志，写在固定大小、循环复用的 `ib_logfile` 里；`log buffer` + commit 时刷盘（`innodb_flush_log_at_trx_commit=1`），并实现了真正的 group commit（与 binlog 组成两阶段提交：先 write binlog，再 redo prepare，再一起 fsync）。checkpoint 是 fuzzy 的，后台持续推进，崩溃恢复时从最近 checkpoint 按 LSN 重放。undo 是独立的 undo log（支持 MVCC 与回滚），恢复的三阶段与 ARIES 一致。撕裂页用 double write buffer 解决。

**PostgreSQL**：日志叫 WAL（文件在 `pg_wal` 目录），同样是生理日志 + LSN + 页 LSN。commit 是否等 WAL 落盘由 `synchronous_commit` 控制。它没有 undo log——MVCC 靠元组头的 `xmin/xmax` 直接存在堆里（和 MiniOB 的 `__trx_xid_begin/end` 隐藏字段思路几乎一样），旧版本由 VACUUM 清理，未提交事务的元组崩溃后靠 `xmin` 标记自然失效，不需要物理回滚。撕裂页用 full_page_writes（checkpoint 后首次改页把整页写进 WAL）解决，而非双写。

**OceanBase**：日志模块就叫 clog（commit log，MiniOB 的模块名正是向它致敬）。最大的不同是**日志即复制流**：clog 通过 Multi-Paxos 在多数派副本间同步，日志既是崩溃恢复的凭据，也是高可用的基础——"日志写到多数派"才算提交成功。存储层是 LSM-Tree：redo 重放进内存的 MemTable，MemTable 写满后转储（minor compaction）成磁盘上的 SSTable，转储点就相当于 checkpoint，之前的 clog 即可回收。因为数据最终以不可变的 SSTable 整体写出，对撕裂页问题的处理方式与 B+ 树原地更新的数据库不同。

一张表收拢本节要点：

| | 日志形态 | commit 持久化时机 | 撕裂页对策 | 日志空间管理 |
| --- | --- | --- | --- | --- |
| InnoDB | 生理日志，循环 redo 文件 | redo 刷盘（可配）+ group commit | double write | checkpoint 推进、循环复用 |
| PostgreSQL | 生理日志（WAL 段文件） | WAL 刷盘（`synchronous_commit` 可配） | full_page_writes | checkpoint 后回收/归档 WAL 段 |
| OceanBase | clog + Multi-Paxos 复制 | 日志写到多数派 | LSM 整体写出，规避原地更新 | MemTable 转储后回收 clog |
| MiniOB | 生理（系统层）+ 逻辑（事务层），按模块划分 | commit 时 `wait_lsn` 等日志落盘 | double write | 只增不减（缺口） |

对比着记：MiniOB 把这套工业机制的"骨架"都实现了（WAL、LSN、页 LSN、分模块 redo、commit 等日志、checkpoint、双写、恢复时回滚未提交事务），但每一处都用了最简实现，下一节专门说缺口。

## 当前实现的缺口

读源码时会发现 MiniOB 的日志模块有几个明确的坑（官方文档"一些缺陷和遗留问题"一节也坦承了一部分），面试时主动讲出这些是加分项：

1. **UPDATE 没有事务级日志**。`MvccTrxLogOperation` 只有 `INSERT_RECORD/DELETE_RECORD/COMMIT/ROLLBACK` 四种，没有 UPDATE；`MvccTrx::update_record`（`src/observer/storage/trx/mvcc_trx.cpp`）整个函数没有调用 `log_handler_`——它把旧版本 `end_xid` 标记为 `-trx_id`、再直接调 `table->insert_record` 插入新版本，绕过了 `MvccTrx::insert_record`/`delete_record` 这两个会写事务日志的入口。物理改动本身有 Record Manager 层的日志兜底，已提交的 UPDATE 崩溃后不会丢；但**未提交的 UPDATE 崩溃后无法被识别和回滚**——事务回放器根本不知道它存在，新旧版本上 `-trx_id` 的负事务号标记会永久残留（新版本对所有事务不可见，形成垃圾版本，而 MiniOB 没有版本 GC）。对比：题 20 的赛题改造正是围绕这条 UPDATE 路径展开的。
2. **UPDATE 日志的失败被吞掉**。`RowRecordPageHandler::update_record`（`src/observer/storage/record/record_manager.cpp`）里 append 日志失败时注释写着 `// return rc; // ignore errors`——数据改了、日志没记上，函数仍返回成功，持久性静默受损。
3. **PAX 列存不支持 UPDATE**。`PaxRecordPageHandler` 没有重写 `update_record`，基类默认实现直接返回 `RC::UNIMPLEMENTED`（`src/observer/storage/record/record_manager.h`），列存表上执行 UPDATE 会失败。
4. **跨层操作不原子**。例如"Buffer Pool 分配页"和"Record Manager 初始化页"是两条独立日志，中间崩溃会留下一个分配了却没初始化的孤儿页（`record_log.h` 的注释和官方文档都指出了这一点）。
5. **日志文件只增不减**：没有删除/归档接口，磁盘会无限膨胀；工业实现用 checkpoint 推进 + 循环复用解决。
6. **写一半的日志**：日志条目本身写一半时断电，恢复会在该处失败，文档建议的做法是 truncate 掉残尾，MiniOB 未处理。
7. **等待机制粗糙**：`wait_lsn` 轮询 + 固定 100ms 睡眠，commit 延迟抖动大；生产系统用条件变量精确唤醒。

## 小结

- 崩溃带来三类问题：已提交修改未刷盘（靠 **redo + WAL** 重放）、页写一半（靠 **checksum + double write** 检测修复）、事务做一半（靠 **undo / 事务日志**回滚）。
- WAL 的核心是"**日志先于数据页落盘**"，把 commit 的持久化成本从多次随机页写降为一次顺序日志追加；steal + no-force 让刷盘时机与事务边界彻底解耦。
- 日志内容分物理 / 逻辑 / 生理三种，工业主流是生理日志；**LSN** 是单调递增的日志序号，**页 LSN** 标记重做边界，使重放幂等、可跳过已生效的日志。
- 提交协议：commit 只需把本事务日志 fsync（group commit 把多个 commit 摊到一次 fsync），数据页延迟刷盘。
- Checkpoint 限定恢复扫描的起点；ARIES 恢复分**分析、redo（重复历史）、undo（回滚 loser 事务）**三阶段。
- MiniOB 实现了完整骨架：`LogHandler`/`DiskLogHandler`（`LogEntryBuffer` + 后台刷盘线程 + `LogFileManager`），四个模块各自的日志，`BplusTreeMiniTransaction` 把多页改动聚合成一条原子日志，`IntegratedLogReplayer` 按模块分发重放，`Db::recover` 从 `check_point_lsn_` 起回放并在 `on_done` 回滚未提交事务，`Db::sync` 在每次 DDL 后做全量 checkpoint。
- 主要缺口：UPDATE 无事务级日志、UPDATE 日志失败被吞、PAX 不支持 UPDATE、日志文件只增不减、跨层操作不原子。

## 面试追问

**问：既然有 redo，commit 时为什么不直接把数据页刷盘？**

答：两个问题。一是慢：一个事务可能改了散落在磁盘各处的多个页，commit 时刷页是多次随机写，而刷日志是一次顺序追加，机械硬盘上差一个数量级以上；二是无法控制延迟：页刷盘时机应该由 Buffer Pool 按内存压力统一调度（no-force），与事务边界解耦。redo 落盘已经足以保证"提交不丢"——崩溃后重放日志就能重建数据页，页什么时候刷盘只影响恢复快慢，不影响正确性。

**问：redo 日志重做一遍，会不会把已经生效的修改重复执行导致数据错误？**

答：不会，靠页 LSN 保证幂等。每个数据页页头记录"最近修改本页的日志的 LSN"，重放日志时若 `日志LSN ≤ 页LSN` 就跳过。MiniOB 里就是 `RecordLogReplayer::replay` 中 `frame_lsn >= entry.lsn()` 的那几行判断。因此恢复从多早的位置开始扫都安全，checkpoint 不需要精确对准脏页边界。

**问：redo 和 undo 的区别？为什么两个都要？**

答：redo 回答"已提交事务的修改没刷盘怎么办"——重放日志重做；undo 回答"未提交事务的修改已刷盘怎么办"——逆向撤销。只要 Buffer Pool 允许未提交事务的脏页提前落盘（steal）且提交时不强制刷页（no-force），两者就缺一不可。ARIES 恢复就是 redo（重复历史，含 loser 的修改）+ undo（逆序回滚 loser）。MiniOB 的 undo 较简化：恢复结束后把没有 COMMIT/ROLLBACK 日志的事务找出来逐个执行回滚操作。

**问：torn page 是什么？redo 不能解决吗？为什么要 double write？**

答：torn page 指数据页（如 InnoDB 16KB、MiniOB 128KB）大于磁盘原子写单位（512B/4KB），断电时页只写了一半。redo 解决不了——redo 是"在完好的页上重做页内修改"，页本身物理损坏时无从重放。解法是 checksum 检测 + double write：刷页先整体写入连续的双写区、再写真实位置；恢复时发现页 checksum 不对，就从双写区拷贝完好副本。PostgreSQL 用另一招：checkpoint 后首次改页把整页镜像写进 WAL（full_page_writes）。

**问：group commit 为什么能大幅提升 TPS？**

答：commit 的硬成本是 fsync 的固定延迟，设它为 L 毫秒，则"一事务一刷"时 TPS ≤ 1000/L。group commit 让一批先后到达的 commit 共享一次 fsync，N 个事务摊薄后每事务成本 L/N，TPS 上限放大 N 倍。代价是单事务提交延迟略升（等凑批），用延迟换吞吐。MiniOB 的前台写内存缓冲、后台线程批量刷盘就是这个骨架，只是等待唤醒用了 100ms 轮询而非条件变量。

**问：checkpoint 会不会导致数据丢失？checkpoint 越频繁越好吗？**

答：不会丢数据——checkpoint 只是把"恢复起点"向右推，推到哪由"此刻所有脏页与日志已落盘"保证，之前的日志不再被恢复需要，但 redo 能力本身不受影响。频率是权衡：越频繁恢复越快、日志可截断得越多，但每次 checkpoint 要刷大量脏页，挤占正常 I/O；工业界用 fuzzy checkpoint 平滑刷盘（如 InnoDB 按 redo 产生速率自适应推进），MiniOB 则是在每次 DDL 后做一次全量 sync，并要求期间无并发事务。

**问：如果 redo 日志本身写了一半就断电，恢复时怎么办？**

答：工业实现给每条日志带上长度和校验信息，恢复时顺序扫描，遇到第一条不完整或校验失败的日志就认为日志到此结束、直接截断——因为日志是严格顺序追加的，一条残缺日志之后不可能再有有效的新日志。MiniOB 目前没做处理：`LogFileWriter::write` 旁留有注释"WARNING 这里需要处理日志写一半的情况"（`src/observer/storage/clog/log_file.cpp`），而 `LogFileReader::iterate` 读到非法的 size 会直接报错中断恢复。官方设计文档建议的做法是把残尾 truncate 掉。

## 回到赛题

- **题 20 update-mvcc**（解析见 `../04_problems_17_24.md`）：MVCC 的 UPDATE 被实现为"旧版本置 `end_xid = -trx_id` + 插入 `begin_xid = -trx_id` 的新版本"，COMMIT 时再把负事务号转正。每一次物理改动（旧版本打标记、新版本落页、提交时改写事务号）都会走本章的 Record Manager 层 redo 日志，而"提交即持久"靠的正是 `MvccTrxLogHandler::commit` 里的 `wait_lsn`——这就是本章"提交协议"一节的直接用例。同时本题路径恰好踩中本章"当前实现的缺口"第 1 条：`MvccTrx::update_record` 不写事务级日志，未提交 UPDATE 崩溃后无法回滚，面试被追问"这个实现的崩溃一致性有没有坑"时可以主动展开。
- **题 3 drop-table**（解析见 `../02_problems_01_08.md`）：DDL 不走事务日志，它的持久性靠 `CommandExecutor::execute`（`src/observer/sql/executor/command_executor.cpp`）里的这段逻辑——`stmt_type_ddl(stmt->type())` 为真就调 `Db::sync()`，即本章"Checkpoint 与恢复流程"一节的"全量快照式 checkpoint"：刷全部脏页、等日志落盘、把 `check_point_lsn_` 写入元文件。DROP TABLE 的"要么全做要么全没做"，就是由"DDL 单线程执行 + 完成后立即 checkpoint"保证的，MiniOB 当前并没有 DDL 的并发控制。
- **题 19 alter**（解析见 `../04_problems_17_24.md`）：ALTER TABLE 涉及重建表数据与元数据变更，同样依赖 DDL 后的 `Db::sync()` 落定结果；理解本题"失败回滚"语义时，要分清两层——执行中途的失败靠 SQL 层自己补偿清理，而崩溃恢复层面则靠本章的 checkpoint + redo 把数据库带回到某个一致快照。面试若被问"DDL 原子性怎么保证"，可以把这两层和工业界的 DDL 日志/原子 DDL 机制对比着答。
