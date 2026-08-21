# 磁盘、页与 Buffer Pool：数据库的内存管理

> 本章导读
>
> 数据库做的所有事情——增删改查、建索引、排序——最终都归结为一件事：在正确的时间把正确的数据在磁盘和内存之间搬来搬去。为什么慢查询总是慢在 I/O 上？为什么数据库不直接用 `fread`/`fwrite` 读文件，而要自己造一个叫做 Buffer Pool 的东西？为什么 MiniOB 的页是 128 KB 而 MySQL 是 16 KB？本章从"磁盘到底有多慢"讲起，依次建立页、缓冲池、淘汰策略、脏页刷盘、double write 这五个概念，然后钻进 MiniOB 的 `src/observer/storage/buffer/` 目录，把 `Page`、`Frame`、`DiskBufferPool`、`BPFrameManager`、`DiskDoubleWriteBuffer` 这几个类的分工讲清楚。这是整个存储引擎的地基，后面讲 Record Manager、B+ 树、WAL 时都会反复回到本章。

## 一、为什么数据库要绕开文件系统自己管理页

### 1.1 先看一组数字

计算机存储是一个金字塔：越往上越快，越往下越大越便宜。下面是经典工程数据（Jeff Dean 的 "Numbers Every Programmer Should Know" 一类资料中常见的量级），重点是**数量级**而不是精确值：

| 操作 | 典型耗时 | 相当于主存访问的倍数 |
| --- | --- | --- |
| L1 缓存访问 | 约 0.5~1 ns | 0.01x |
| 主存（DRAM）访问 | 约 100 ns | 1x |
| NVMe SSD 随机读（4 KB） | 约 10~100 us | 100~1000x |
| 机械硬盘随机 I/O | 约 5~10 ms | 约 100,000x |
| 机械硬盘顺序读 | 约 100 MB/s 量级 | 取决于块大小 |

把 ns 换成人能感知的尺度：假设主存访问是 1 秒，那么一次 SSD 随机读大约是两三天，一次机械盘随机 I/O 大约是三四个月。**CPU 和磁盘之间的速度差是五到六个数量级**——这就是" I/O 是数据库第一瓶颈"这句话的全部含义。一条 SQL 慢，十有八九不是 CPU 算得慢，而是它在等磁盘。

由此得到一个数据库设计的总原则：**想尽一切办法减少磁盘 I/O 的次数，把不可避免的 I/O 尽量变成顺序 I/O。**

### 1.2 为什么不能依赖操作系统的页缓存

你可能会问：Linux 本身就有 page cache，`read()` 一个文件时内核已经把数据缓存进内存了，数据库为什么还要自己再缓存一遍？

原因在于**控制权**。数据库内核知道一些操作系统不可能知道的事情：

- 它知道哪些页是热点（比如 B+ 树的根页几乎每次查询都要用），操作系统的通用淘汰策略不了解这些语义；
- 它知道哪些页**正在被使用、绝不能被淘汰**（一个线程正在往页里写记录，页却被换出去了，就是灾难）；
- 它必须精确控制**刷盘时机**：redo log 必须先于数据页落盘（WAL 原则），而操作系统的写回策略不受它指挥；
- 它需要为崩溃恢复做准备：检测写了一半的"撕裂页"（torn page），这需要数据库自己在页里放校验和。

所以所有正经的数据库都自己实现一层缓存，这就是 **Buffer Pool（缓冲池）**。它本质上是一个"以页为单位的、应用层可控的缓存系统"。

## 二、页（Page）：磁盘读写的最小单位

### 2.1 为什么按页而不是按行读写

假设一行记录 100 字节。如果每次读一行就发起一次磁盘 I/O，那么扫描 100 万行就是 100 万次随机 I/O——在机械盘上按 10 ms 一次算是 10000 秒，接近 3 个小时。

但磁盘 I/O 的耗时几乎不由"读多少字节"决定，而由"要不要移动磁头/要不要发起一次请求"决定。读 100 字节和读 16 KB 的一次随机 I/O，耗时几乎一样。那不如一次多读一点：一次 I/O 把**连续的一大块**读进来，里面装着上百行，后面的行访问就全部命中内存了。这就是**空间局部性**的利用。

这个"一大块"就是**页（Page）**：数据库把每个数据文件、索引文件都切成固定大小的页，所有磁盘 I/O 以页为最小单位。页是数据库世界里磁盘与内存之间唯一的"货币"。

跟着算一遍：一张 1 GiB 的表，按 MiniOB 的 128 KB 页切分，共 `1 GiB / 128 KiB = 8192` 页。全表扫描就是 8192 次页 I/O：

- 若这些页在磁盘上连续存放，机械盘顺序读按 100 MB/s 估算，约 `1 GiB / 100 MB/s ≈ 10.7` 秒；
- 若退化成 8192 次随机 I/O，每次 10 ms，就是约 82 秒——同样的数据量慢了 8 倍；
- 若更进一步退化成"每行一次 I/O"（假设一行 100 字节，共约 1070 万行），就是 1070 万次随机 I/O，约 30 个小时。

三个数字记住结论就行：**I/O 次数由页大小决定，I/O 单价由顺序还是随机决定**。数据库内核的很大一部分工作，就是在和这两个变量作斗争。

### 2.2 页大小的权衡

页不是越大越好，也不是越小越好：

| | 大页（如 128 KB） | 小页（如 4 KB） |
| --- | --- | --- |
| 顺序扫描吞吐 | 好，一次 I/O 拿进更多数据 | 差，I/O 次数多 |
| 随机点查 | 浪费：只想读一行却读进一整页（读放大） | 精准 |
| 缓存效率 | 同样内存能缓存的"不同页"少，粒度粗 | 同样内存覆盖更多分散的页 |
| 页内管理开销 | 页头、位图等占比小 | 占比大 |
| 刷盘写放大 | 改一个字节也要写一整页，放大严重 | 放大较小 |

各家的选择体现了工作负载的差异：

- **PostgreSQL**：8 KB（编译期固定），偏点查的 OLTP 传统；
- **MySQL InnoDB**：默认 16 KB（`innodb_page_size`）；
- **MiniOB**：**128 KB**，定义在 `src/observer/storage/buffer/page.h`：

```cpp
static constexpr const int BP_PAGE_SIZE      = (1 << 17);  // 128KB
static constexpr const int BP_PAGE_DATA_SIZE = (BP_PAGE_SIZE - sizeof(PageNum) - sizeof(LSN) - sizeof(CheckSum));
```

`PageNum` 是 `int32_t`、`LSN` 是 `int64_t`、`CheckSum` 是 `unsigned int`（见 `src/observer/common/types.h`），所以 `BP_PAGE_DATA_SIZE = 131072 - 4 - 8 - 4 = 131056` 字节。教学场景下表往往被整表扫描，大页能减少 I/O 次数；同时 128 KB 的页也让"一个 65535 字节的 TEXT 字段直接塞进一页"成为可能——这正是赛题 17（text）的立足点，见文末"回到赛题"。

## 三、Buffer Pool 的职责：缓存、pin 与 unpin

### 3.1 一个生活类比

把磁盘想象成图书馆的地下书库，内存是你书桌的桌面。你要写论文（执行 SQL）时，不会每查一句话就跑一趟书库，而是把要用的书（页）一次性借到桌面上。桌面大小有限，书看完了要还（淘汰），**摊开在桌上正在抄的那本不能还**——这就是 pin。抄完了合上书，它才可以被还掉——这就是 unpin。如果你在书上做了批注（修改了页），还书之前得先把批注誊抄回书库的存档（刷脏页）。

### 3.2 严格定义

Buffer Pool 是数据库在内存中开辟的一块区域，划分成若干个与页等大的槽位，每个槽位叫一个**页帧（Frame）**。它的职责：

1. **缓存**：所有对数据文件、索引文件的读写都必须经过它。上层模块（Record Manager、B+ 树）永远拿不到"裸磁盘"，只能拿到内存中的 Frame；
2. **映射**：维护"哪个文件的第几页"在哪个 Frame 里；
3. **淘汰**：Frame 不够用时，按某种策略牺牲一个旧页，给新页腾地方；
4. **写回**：被修改过的页（脏页）在淘汰或检查点时写回磁盘。

在 MiniOB 里，"哪个文件的第几页"由 `FrameId` 表示（`src/observer/storage/buffer/frame.h`），它就是一对 `(buffer_pool_id, page_num)`：

```cpp
class FrameId {
  int     buffer_pool_id_ = -1;   // 哪个文件（哪个 DiskBufferPool）
  PageNum page_num_       = -1;   // 文件内第几页
};
```

### 3.3 pin/unpin 语义：为什么 pin 住的页不能淘汰

淘汰一个 Frame 意味着这块内存要被覆写成别的页。如果此时还有一个线程正握着指向这块内存的指针读数据，覆写之后它读到的就是垃圾——这是一个 use-after-free 级别的 bug。

解决办法是引用计数。Frame 里有一个 `pin_count_`（`frame.h` 中是 `atomic<int>`）：

- 上层要用某个页，先通过 `get_this_page` / `allocate_page` 拿到 Frame，此时 `pin_count_` 加 1，这个页就"驻留缓冲区"，淘汰算法**绝对不能选它**；
- 用完后调用 `unpin_page`，`pin_count_` 减 1；
- `can_purge()` 的定义就是 `pin_count_ == 0`（`frame.h:140`）。

规则只有一条：**pin 住的页不可淘汰**。代价是对称的：谁 pin 了谁就必须 unpin，忘了 unpin 的页会永远赖在内存里（一种内存泄漏），MiniOB 为此专门留了调试接口 `check_all_pages_unpinned()`（`disk_buffer_pool.cpp:519`）。

注意 pin 和锁（latch）是两回事：pin 保护的是"内存别被回收"，latch（`read_latch`/`write_latch`）保护的是"并发读写别打架"。MiniOB 的 Frame 两者都有。

用一个时间线把 pin 计数走一遍。线程 A 要读第 5 页，线程 B 同时也要读第 5 页，缓冲池此时已满，淘汰器正在找牺牲品：

| 时刻 | 事件 | 第 5 页的 pin_count | 能否被淘汰 |
| --- | --- | --- | --- |
| t1 | A 调 `get_this_page(5)`，未命中，加载后返回 | 1 | 不能 |
| t2 | 淘汰器扫到第 5 页，`can_purge()` 为假，跳过 | 1 | 不能 |
| t3 | B 调 `get_this_page(5)`，命中缓存 | 2 | 不能 |
| t4 | A 用完，调 `unpin_page` | 1 | 不能 |
| t5 | B 用完，调 `unpin_page` | 0 | **可以** |

关键观察：命中缓存的 `get` 也会加 pin（MiniOB 是在 `BPFrameManager::get_internal` 里统一 `pin()` 的，`disk_buffer_pool.cpp:112`），所以"读同一个页的一百个线程"对应 pin_count 涨到一百再回落，全程该页都钉在内存里，直到最后一个使用者放手。

### 3.4 一次真实的页访问流程

以 Record Manager 为例（`src/observer/storage/record/record_manager.cpp`），插入一条记录时：

1. `buffer_pool.get_this_page(page_num, &frame_)` 拿页（pin +1），命中则直接返回；
2. `frame_->write_latch()` 加写锁；
3. 往页里写记录，`frame_->mark_dirty()` 标脏；
4. 解锁、`disk_buffer_pool_->unpin_page(frame_)` 释放 pin。

B+ 树索引走完全相同的通道——索引文件也是分页文件，同样由 Buffer Pool 管理（`src/observer/storage/index/bplus_tree_index.cpp` 中同样从 `db->buffer_pool_manager()` 拿管理器）。

## 四、淘汰策略：FIFO、LRU 与它的亲戚们

内存装不下所有页时，选谁淘汰？这是缓存系统的核心问题。

### 4.1 FIFO 的问题：Belady 异常

最直觉的做法是先进先出（FIFO）：先加载的页先淘汰。但 FIFO 有一个著名的反直觉缺陷——**Belady 异常**：缓存变大，缺页反而可能变多。

经典例子：访问序列 `1 2 3 4 1 2 5 1 2 3 4 5`。

- 3 个 Frame 的 FIFO：缺页 9 次；
- 4 个 Frame 的 FIFO：缺页 **10** 次。

内存变大了性能反而变差，说明 FIFO 根本没有利用"访问的时间局部性"。

### 4.2 LRU：最近最少使用

**LRU（Least Recently Used）**：淘汰最长时间没有被访问的页。它依据的是一个经验假设——最近用过的页大概率马上还会用（时间局部性）。LRU 属于"栈式算法"，缓存扩大时命中率单调不降，不会有 Belady 异常。

标准实现是**哈希表 + 双向链表**：哈希表 O(1) 定位页，每次访问把节点挪到链表头部，淘汰时取链表尾部，所有操作 O(1)。结构长这样：

```
        哈希表（FrameId -> 链表节点）          双向链表（按访问时间排序）
       ┌─────────────────────┐        头（最热）                    尾（最冷）
       │ (文件A,页3) ────────┼──┐    ┌────┐   ┌────┐   ┌────┐   ┌────┐
       │ (文件A,页7) ────────┼─┐└──> │页3 │<->│页7 │<->│页12│<->│页9 │
       │ (文件B,页12)────────┼┐ └──> │    │   │    │   │    │   │    │
       │ (文件B,页9) ────────┼┼────> └────┘   └────┘   └────┘   └────┘
       └─────────────────────┘         ↑ 每次访问挪到这里     ↑ 淘汰从这里取
```

面试时能手画这张图、说出"为什么是两个结构而不是一个"，LRU 这道题就过了一半：单靠链表查找是 O(N)，单靠哈希表又回答不了"谁最久没用"。

但 LRU 也有代价和毛病：

- **每次访问都要动链表**，而链表头指针是全局共享的——高并发下所有线程抢着改同一个链表，锁竞争严重。工业实现会把 Buffer Pool 切成多个分片（shard），每个分片一把锁；
- **顺序扫描污染**：全表扫描会把几百万个"只读一次"的页灌进来，把真正的热点页全部挤走。这些页扫描完再也不会被用，LRU 却对它们一视同仁。

### 4.3 LRU-K 与 Clock：两个方向的修正

针对扫描污染，**LRU-K** 记录每页**最近 K 次**访问的时间，淘汰"第 K 近访问最久远"的页。K=2 时，一个只被扫过一次的页根本没有第二次访问记录，会优先于热点页被淘汰，从而扛住扫描污染。MySQL InnoDB 的 "midpoint insertion"（新页先插入 LRU 的 5/8 处而不是头部，全表扫描的页进 old 区，停留时间不够就不提升）是同样的思想。

针对链表维护代价，**Clock（时钟算法，又称 second chance）** 放弃精确排序：所有 Frame 排成一个环，每个 Frame 带一个访问位（reference bit）。扫描指针转圈，遇到访问位为 1 的就清 0 放它一马，遇到 0 的就淘汰。它用 O(1) 的无锁位操作逼近 LRU 的效果，PostgreSQL 的 clock-sweep 就是这种近似（每个 buffer 带一个 0~5 的 usage_count，扫描时递减，减到 0 才可淘汰）。

## 五、脏页与刷盘：torn page 与 double write

### 5.1 写路径：先改内存，落盘延后

Buffer Pool 的写哲学是：**写操作只改内存页，标脏（`mark_dirty`），不落盘**。落盘（flush）被推迟到这些时刻：

- 该脏页被淘汰，必须先把新内容写回磁盘才能覆写内存（MiniOB 的 `purge_frame` 里就是这么做的）；
- 显式的 checkpoint / 同步命令（如 MiniOB 的 `flush_all_pages`、`Db::sync`）；
- 关闭文件时（`close_file` → `purge_all_pages`）。

好处是明显的：同一个页被改 100 次只写 1 次磁盘（写合并），而且多次修改可以攒成一次顺序写。坏处是：**崩溃时内存里没落盘的修改会丢**——这个问题由 WAL（redo log）解决，先记日志、后刷数据页，属于另一章的内容。本章要解决的是一个更隐蔽的问题。

### 5.2 torn page：写了一半的页

MiniOB 一页 128 KB，而文件系统通常按 4 KB 块、磁盘按 512 字节扇区写。刷一个 128 KB 的页需要很多次底层写操作。如果写到一半断电，磁盘上的页就是**一半新一半旧**的"撕裂页"（torn page / partial page write）。

关键在于：**redo log 救不了撕裂页**。因为 redo log 记录的是"对第 N 页偏移 X 处写入 Y"这样的物理修改，它的前提是"目标页本身是一个完好的旧版本"。页本身已经烂了，往一个烂页上重做日志，得到的还是烂数据。

怎么发现撕裂？校验和。MiniOB 的 `Page` 结构（`page.h`）就是为此设计的：

```cpp
struct Page
{
  LSN      lsn;        // 本页最后一次修改对应的日志序列号，redo 用
  CheckSum check_sum;  // 数据区的 CRC32 校验和，检测撕裂用
  char     data[BP_PAGE_DATA_SIZE];
};
```

刷盘前算一遍 `crc32(page.data, BP_PAGE_DATA_SIZE)` 填进 `check_sum`；读页时重算一遍，对不上就说明页不完整。

### 5.3 double write buffer：先写副本，再写真身

检测到撕裂之后，得有一份完好的副本来还原，这就是 **double write buffer（双写缓冲区）**：

1. 脏页要落盘时，**先把整页写到一个共享的副本区**（double write 文件），等这份副本确认写完；
2. 再把页写到它真正的数据文件位置；
3. 第 2 步中如果宕机，数据文件里的页可能撕裂，但副本区里有完好的副本——重启后用副本覆盖数据页，把数据文件修复成"完好但可能偏旧"的状态；
4. 之后再跑 redo log 重放到最新。

之所以可行，是因为副本区是**顺序追加写、且每次写都有校验和保护**：副本区自己的某个页写坏了，校验和对不上，直接丢弃即可，数据文件里那份旧页还没被碰过，依然是完好的。

代价是同一个页写了两遍磁盘（所以叫 double write），还有额外的 fsync。工业界认为这笔账划算：机械盘时代这两次写可以都做成顺序写；SSD 时代也有更省的替代方案（见第八节 PostgreSQL 的 full page write）。

## 六、MiniOB 源码实现

整体设计文档见 `docs/docs/design/miniob-buffer-pool.md` 与 `docs/docs/design/miniob-double-write-buffer.md`（仓库内相对路径），本节直接对照源码讲。涉及的文件都在 `src/observer/storage/buffer/`：`page.h`、`frame.h/.cpp`、`disk_buffer_pool.h/.cpp`、`double_write_buffer.h/.cpp`。

### 6.1 五个角色，一张图

```
   SQL 执行层
       |
   Record Manager / B+Tree          （只认 Frame，不碰磁盘）
       |
   DiskBufferPool  ----------------  每个打开的 .data/.index 文件一个对象
       |        |                    （文件头页 + 位图：哪些页已分配）
       |        +--> DoubleWriteBuffer (DiskDoubleWriteBuffer)
       |                             脏页先写 dblwr.db 副本，再写数据文件
       v
   BPFrameManager                    （全局唯一：所有文件共享的 Frame 池）
       LruCache<FrameId, Frame*>  +  MemPoolSimple<Frame>
       |
   磁盘文件（page 0 = 文件头，page 1..N = 数据/索引页）
```

### 6.2 Page 与 Frame：磁盘页与内存槽

`Page` 是页在磁盘/内存中的裸数据（lsn + check_sum + data，恰好 128 KB），上一节已经看过。`Frame`（`frame.h`）是页在内存中的**管理外壳**：

```cpp
class Frame {
  bool          dirty_ = false;      // 脏标记：淘汰时要不要先写盘
  atomic<int>   pin_count_{0};       // 引用计数：>0 不可淘汰
  unsigned long acc_time_ = 0;       // 最近访问时间
  FrameId       frame_id_;           // 我是谁：(buffer_pool_id, page_num)
  Page          page_;               // 页数据本体
  common::RecursiveSharedMutex lock_; // 页级读写锁（latch）
};
```

一句话区分：**Page 是数据，Frame 是"数据 + 管理信息"**。上层模块拿到的永远是 `Frame *`，通过 `frame.data()` 读写页内容，通过 `mark_dirty()` 告诉 Buffer Pool"这页被我改过了"。

### 6.3 DiskBufferPool：一个文件一个池，文件头管分配

`DiskBufferPool`（`disk_buffer_pool.h:188`）是"一个分页文件"在内存中的管理者：每张表的数据文件、每个索引文件各对应一个对象。它管两件事：文件里哪些页已分配（空间管理），以及页在磁盘和 Frame 之间的搬运（I/O）。

**文件头页与位图**。文件的第 0 页（`BP_HEADER_PAGE = 0`）是元数据页，存放 `BPFileHeader`（`disk_buffer_pool.h:64`）：

```cpp
struct BPFileHeader
{
  int32_t buffer_pool_id;   // 本文件的 buffer pool id
  int32_t page_count;       // 文件当前一共有多少页
  int32_t allocated_pages;  // 已分配多少页
  char    bitmap[0];        // 位图：第 i 位为 1 表示第 i 页已分配
  static const int MAX_PAGE_NUM = (BP_PAGE_DATA_SIZE - sizeof(page_count) - sizeof(allocated_pages)) * 8;
};
```

位图紧跟在文件头后面，躺在第 0 页的 data 区里。因为位图必须装进一个页，`MAX_PAGE_NUM ≈ (131056 - 8) × 8 = 1,048,384` 页，折合约 128 GiB——这就是单个 MiniOB 数据文件的大小上限（文件头注释里的 TODO 也承认了这一点，并把"支持无限多页"留作思考题）。

整个文件在磁盘上的布局：

```
偏移 0                    偏移 128 KB               偏移 256 KB
┌──────────────────────────┬──────────────────────┬──────────────────────┬─
│ page 0（文件头页）        │ page 1               │ page 2               │ ...
│ ┌──────────────────────┐ │                      │                      │
│ │BPFileHeader          │ │  给上层模块用         │  给上层模块用         │
│ │ buffer_pool_id       │ │  （Record Manager 的  │  （B+ 树的内部结点/  │
│ │ page_count           │ │   记录页，或索引页，  │   叶子结点，由页内     │
│ │ allocated_pages      │ │   页内结构 Buffer     │   自定义格式决定，    │
│ │ bitmap: 11101000...  │ │   Pool 不关心）       │   Buffer Pool 不关心）│
│ └──────────────────────┘ │                      │                      │
└──────────────────────────┴──────────────────────┴──────────────────────┴─
 位图第 i 位 = 1 表示 page i 已分配；page 0 自己的位永远是 1
```

第 N 页在文件里的偏移就是 `N × 128 KB`——`load_page`/`write_page` 里那句 `offset = ((int64_t)page_num) * BP_PAGE_SIZE`（`disk_buffer_pool.cpp:743`）就是这个对应关系，定长页让"页号 → 磁盘地址"变成一次乘法，不需要任何索引。

**分配新页**（`allocate_page`，`disk_buffer_pool.cpp:352`）就是操作这个位图：

```cpp
for (int i = 0; i < file_header_->page_count; i++) {
  byte = i / 8;  bit = i % 8;
  if (((file_header_->bitmap[byte]) & (1 << bit)) == 0) {   // 找到空洞页
    file_header_->allocated_pages++;
    file_header_->bitmap[byte] |= (1 << bit);               // 置位
    hdr_frame_->mark_dirty();                               // 文件头页也变脏了
    ...
    return get_this_page(i, frame);                         // 把该页读进缓冲区返回
  }
}
// 没有空洞：page_count++ 扩文件，并把新页 flush 一次以扩展文件长度
```

释放页（`dispose_page`）是逆操作：清位图位、`allocated_pages--`。遍历文件里所有已分配页由 `BufferPoolIterator` 完成，本质就是在位图上不断找下一个置位 bit（`next_setted_bit`）。

**读页**（`get_this_page`，`disk_buffer_pool.cpp:315`）先查内存再读盘：

```cpp
Frame *used_match_frame = frame_manager_.get(id(), page_num);
if (used_match_frame != nullptr) {   // 缓存命中：access() 刷新热度，直接返回
  used_match_frame->access();
  *frame = used_match_frame;
  return RC::SUCCESS;
}
// 未命中：allocate_frame 拿一个空 Frame（可能触发淘汰），再 load_page 从磁盘读入
```

注意 `frame_manager_.get()` 内部已经 `pin()` 了（见 6.4），所以调用方拿到的 Frame 天然是 pin 住的。`load_page` 的顺序也值得记住：**先问 double write buffer 要**（`dblwr_manager_.read_page`），副本区没有才去 `lseek + readn` 读数据文件——因为副本区里的页可能比数据文件里的更新。

### 6.4 BPFrameManager：全局 Frame 池 = LruCache + MemPoolSimple

所有 `DiskBufferPool` 共享一个全局的 `BPFrameManager`（挂在 `BufferPoolManager` 上，`disk_buffer_pool.h:347`），它由两个部件组成（`disk_buffer_pool.h:154`）：

```cpp
using FrameLruCache  = common::LruCache<FrameId, Frame *, BPFrameIdHasher>;
using FrameAllocator = common::MemPoolSimple<Frame>;
```

- `FrameLruCache`（`deps/common/lang/lru_cache.h`）：哈希表 + 双向链表。`get`/`put` 会把节点挪到链表头（`lru_touch`），`foreach_reverse` 从链表尾（最久未用）开始遍历——淘汰扫描正是从尾部开始的；
- `MemPoolSimple<Frame>`（`deps/common/mm/mem_pool.h`）：Frame 对象的定长内存池。分配出的 Frame 不真正 delete，用 `reinit()`/`reset()` 复用，避免频繁构造析构 128 KB 的大对象。

容量是固定的：`BPFrameManager::init` 用 `allocator_.init(false, pool_num)` 关闭了动态扩展。默认参数下 `BufferPoolManager` 构造函数（`disk_buffer_pool.cpp:767`）算出 `pool_num = 2`，每个 pool 128 个 Frame（`DEFAULT_ITEM_NUM_PER_POOL`），即 **256 个 Frame × 128 KB = 32 MiB** 的缓冲池。

**分配与淘汰的闭环**在 `DiskBufferPool::allocate_frame`（`disk_buffer_pool.cpp:687`）：

```cpp
while (true) {
  Frame *frame = frame_manager_.alloc(id(), page_num);
  if (frame != nullptr) { *buffer = frame; return RC::SUCCESS; }
  // 池子满了：从 LRU 尾部找 pin_count==0 的页，脏则先刷盘，然后回收
  (void)frame_manager_.purge_frames(1 /*count*/, purger);
}
```

`purge_frames`（`disk_buffer_pool.cpp:62`）的逻辑：持锁从 LRU 尾部遍历，挑出 `can_purge()`（即 `pin_count == 0`）的 Frame 并临时 pin 住，然后对每个候选调用 `purger`——脏页先经 double write buffer 刷盘（本文件的页走 `flush_page_internal`，别的文件的页转发给 `bp_manager_.flush_page`），最后 `free_internal` 把它从 LRU 缓存摘掉、还回内存池。淘汰的完整链条就是：**LRU 尾部选人 → pin 检查放行 → 脏页写回 → 内存复用**。

### 6.5 DiskDoubleWriteBuffer：副本区

接口 `DoubleWriteBuffer`（`double_write_buffer.h:27`）只有三个方法：`add_page`、`read_page`、`clear_pages`。真正实现是 `DiskDoubleWriteBuffer`（内存最多缓存 `max_pages = 16` 个页，副本文件是数据库目录下的 `dblwr.db`，由 `Db::init` 创建，见 `src/observer/storage/db/db.cpp:75-87`）。

刷一个脏页时（`DiskBufferPool::flush_page_internal`，`disk_buffer_pool.cpp:544`）：

```cpp
RC rc = log_handler_.flush_page(frame.page());              // 1. 先保证 WAL 落盘
frame.set_check_sum(crc32(frame.page().data, BP_PAGE_DATA_SIZE)); // 2. 算校验和
rc = dblwr_manager_.add_page(this, frame.page_num(), frame.page()); // 3. 进 double write buffer
frame.clear_dirty();                                        // 4. 清脏标记
```

`add_page`（`double_write_buffer.cpp:99`）把页放进内存哈希表 `dblwr_pages_`，并**立即**把页写进 `dblwr.db`（write-through，保证副本区永远是最新的）；当缓存满 16 页时 `flush_page()`：逐页通过 `bp_manager_.get_buffer_pool` 找到属主文件，调 `DiskBufferPool::write_page` 写到数据文件的真实偏移（`page_num × 128 KB`），写完把副本标记为 invalid。

崩溃恢复时，`Db::init_dblwr_buffer`（`db.cpp:545`）在打开所有表之后、redo 之前调用 `recover()`（就是 `flush_page()`）：启动时 `load_pages` 已把 `dblwr.db` 里校验和完好的页读进内存，`recover` 把它们逐一写回数据文件。这样 redo 开始时，数据文件里绝对没有撕裂页。另外还有一个 `VacuousDoubleWriteBuffer` 空实现：`add_page` 直接写数据文件，等于关掉双写，用于不需要崩溃保护的场合。

## 七、复杂度分析与工程取舍

| 操作 | 复杂度 | 说明 |
| --- | --- | --- |
| 页查找（get_this_page 命中） | O(1) | 哈希表定位 + 链表挪头 |
| 淘汰一个页 | O(N) 扫描 LRU 尾部找 pin==0 的页 | 最坏要跳过一串被 pin 的页；期望 O(1) |
| 分配/释放页 | O(page_count) 位图扫描 | 顺序找第一个 0 位；文件头注释承认这是已知低效点 |
| 刷脏页 | 一次 128 KB 写 × 2（双写） | 写放大是崩溃保护的买路钱 |

工程上的主要取舍：

- **页大小 128 KB**：利于扫描和教学简化（TEXT 可直接内联），牺牲点查读放大与刷盘粒度；
- **固定 32 MiB 缓冲池 + 全局共享**：实现简单、内存上限可控；缺点是淘汰扫描持一把大锁（`purge_frames` 的注释自己承认"极大地降低并发度"）；
- **文件头位图**：分配信息跟文件同生共死、实现不过百行；代价是单文件约 128 GiB 上限、分配要线性扫位图；
- **double write 常驻内存 16 页**：副本区小则落盘及时，但满 16 页就要停下来集中刷盘（代码 TODO 注明"可能会导致程序卡住一段时间"）。

## 八、工业数据库怎么做

- **MySQL InnoDB**：页默认 16 KB；Buffer Pool 默认 128 MiB，可配成多个 instance 减少锁竞争；LRU 链表分 young/old 两段（midpoint insertion，新页进 old 段头部，`innodb_old_blocks_time` 内的重复访问不提升），专治全表扫描污染；脏页由后台 page cleaner 线程按 LSN 水位渐进刷盘；防撕裂用 doublewrite buffer，思路与 MiniOB 完全一致，只是副本放在系统表空间的连续区里，两次写都可以是顺序写。
- **PostgreSQL**：页 8 KB；共享缓存 `shared_buffers` 默认 128 MiB，淘汰用 clock-sweep（带 usage_count 的 Clock 变体）；**不用 double write**，而是靠 WAL 的 full page writes——checkpoint 之后每个页第一次被修改时，把整页镜像写进 WAL，崩溃恢复时直接用页镜像还原，再重放后续日志。用 WAL 体积换掉了双写。
- **OceanBase**：存储是 LSM-Tree 架构，内存中是可写的 MemTable，写满后冻结、转储为磁盘上的 SSTable；基线数据按宏块（默认 2 MB）组织，宏块内部再切 16 KB 级别的微块，微块是读 I/O 与缓存（block cache）的基本单位。因为数据落盘以追加式的顺序写为主，原地更新少，torn page 的暴露面与原地更新的 B+ 树不同，完整性靠块级校验和保证。它证明了一点：页缓存的思想不变，但"页"的形态可以随存储结构而变。

## 小结

- 磁盘比内存慢五六个数量级，I/O 次数是数据库性能的第一变量；数据库因此绕开文件系统缓存，自己实现可控的 Buffer Pool。
- 页是磁盘 I/O 的最小单位；页大小是"扫描吞吐"与"点查放大"之间的权衡，MiniOB 取 128 KB，InnoDB 取 16 KB。
- Buffer Pool = 页帧池 + 映射表 + 淘汰器 + 写回器；pin/unpin 引用计数保证"正在用的页不被淘汰"，忘了 unpin 等于内存泄漏。
- FIFO 有 Belady 异常；LRU 靠哈希表+双向链表做到 O(1)，但怕扫描污染、怕链表锁竞争，于是有了 LRU-K、Clock、分片等变体。
- 写先改内存标脏、延后落盘；崩溃可能造成 torn page，redo log 无能为力，MiniOB 用 `DiskDoubleWriteBuffer` 先写副本再写真身解决。
- MiniOB 实现五件套：`Page`（128 KB 裸页）、`Frame`（页帧：dirty/pin_count/latch）、`DiskBufferPool`（一文件一池，文件头页+位图管分配）、`BPFrameManager`（全局 Frame 池，`LruCache` + `MemPoolSimple`，默认 256 帧 32 MiB）、`DiskDoubleWriteBuffer`（16 页内存副本 + `dblwr.db`）。

## 面试追问

**问：操作系统有 page cache，数据库为什么还要自己实现 Buffer Pool？**
答：核心是控制权。数据库知道页的访问语义（B+ 树根页永远是热点）、知道哪些页正被引用不能淘汰（pin）、必须控制刷盘顺序（WAL 先于数据页落盘）、要在页内放校验和做崩溃恢复。OS 的通用缓存这些一件都做不到，而且还存在应用缓冲 + OS 缓存的双缓冲浪费。

**问：pin 住的页为什么不能淘汰？如果代码忘了 unpin 会怎样？**
答：淘汰就是覆写内存，有线程正握着指针读写这个 Frame，覆写就是 use-after-free。MiniOB 用 `pin_count_` 计数，`can_purge()` 要求它为 0。忘了 unpin 的页永远无法被淘汰，日积月累缓冲池被"钉子户"占满，`purge_frames` 找不到可淘汰的页，分配新页会陷入死循环或报错；MiniOB 留了 `check_all_pages_unpinned()` 专门排查这类问题。

**问：LRU 已经很经典了，工业界为什么不用"纯 LRU"？**
答：两个痛点。一是并发：每次访问都要把节点挪到链表头，全局链表头是竞争热点，所以要分片或用 Clock 这类无精确排序的近似。二是扫描污染：全表扫描把只用一次的页灌进头部，热点页全被挤走；InnoDB 用 midpoint insertion（新页进 5/8 处）、学术界用 LRU-K 记录最近 K 次访问来缓解。

**问：有了 redo log，为什么还需要 double write？**
答：redo log 是"对完好旧页做物理修改"的重放机制，前提是页本身完整。刷盘断电会把页写成一半新一半旧的撕裂页，往撕裂页上重做日志没有意义。double write 先用共享副本区保存完好副本，恢复时先拿副本把数据页还原成完好状态，redo 才有意义。检测撕裂靠页内 CRC32 校验和。

**问：页是不是越大越好？MiniOB 为什么是 128 KB？**
答：不是。大页顺序吞吐好、管理开销低，但点查读放大严重、同样内存缓存的页数少、刷盘写放大大、撕裂风险面也大。MiniOB 面向教学以扫描型负载为主，且要让 65535 字节的 TEXT 字段能内联进单页，所以取 128 KB；以点查为主的 InnoDB 取 16 KB、PostgreSQL 只有 8 KB。

**问：Buffer Pool 命中率是不是越高越好？能靠它解决所有大数据量问题吗？**
答：命中率只对"重复访问已持久化的页"有意义。如果工作集本身远超内存且没有局部性（比如对从未排序过的中间结果做排序），缓存帮不上忙——这正是外部排序存在的理由，见下一节。

## 回到赛题

**赛题 17（text，解析见 [../04_problems_17_24.md](../04_problems_17_24.md)）**。TEXT 最大 65535 字节，而 MiniOB 旧式的"记录不跨页"假设要求一行必须放进一页。本章的 128 KB 页（`BP_PAGE_SIZE = 1 << 17`，数据区 131056 字节）正是这条路的底座：64 KB 的 TEXT 字段可以作为定长字段直接内联进记录，一页放一行也放得下。代价就是本章第二节的权衡全部兑现：短文本浪费空间、页利用率低、刷盘写放大大。面试时可以主动对比工业界的 locator + 溢出页方案，说明"大页内联"是教学实现而非通用解。

**赛题 24（big-order-by，解析见 [../04_problems_17_24.md](../04_problems_17_24.md)）**。题目要求在内存受限下对四表连接的结果排序。关键在于：Buffer Pool 缓存的是**磁盘上已存在的分页文件**，而排序的中间结果是执行器现场生成的、体量可以任意大的新数据——它们从不在 Buffer Pool 里，靠缓存解决不了"内存装不下"的问题。MiniOB 的做法是绕过 Buffer Pool：`ExternalSorter` 给 5 MB 排序缓冲，满了就写成独立的临时 run 文件（`external_sorter.cpp` 中经 `temp_file_mgr_.create_temp_file("run")` 创建），再多路归并。这就是本章第一节总原则的另一面：当 I/O 不可避免时，把随机 I/O 组织成顺序 I/O、把内存装不下的计算改写成多趟磁盘算法。
