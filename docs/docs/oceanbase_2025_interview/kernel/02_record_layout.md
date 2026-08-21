# 记录如何躺在磁盘上：堆文件、RID 与行列存储

## 本章导读

上一章（[架构与执行链](../01_project_architecture.md)）我们跟着一条 SQL 走完了从网络到执行器的全程，最后所有的增删改查都会落到同一个问题上：**一张表的成千上万行记录，到底以什么样的字节布局存放在磁盘文件里？**

这个问题看似底层，却是存储引擎一切设计的原点：

- 记录是定长还是变长？这决定了能不能用"页号 + 槽号"直接算出记录的位置；
- 一页之内怎么管理空闲空间？这决定了插入和删除的开销；
- 用什么东西来"指"一条记录？这决定了索引、MVCC、锁全部围绕谁来设计；
- 一行放一起还是一列放一起？这决定了 OLTP 和 OLAP 两条完全不同的技术路线。

MiniOB 对这些问题的回答非常"教学化"：**全定长记录、堆文件组织、RID 物理寻址、行存为主 + 页内列式（PAX）扩展**。学完本章，你应该能徒手算出一个 128KB 的页能装几条记录，能讲清楚 `insert_record` 从 `free_pages_` 到 bitmap 置位的完整路径，也能解释为什么赛题 17 的 TEXT 类型（65535 字节定长内联）会把存储布局逼到"一页只装一条记录"的窘境。

阅读本章前不需要任何数据库基础，只需要知道"磁盘按块读写、内存按字节寻址"。

## 1. 从字节到行：定长记录 vs 变长记录

### 1.1 直觉：停车场与路边野停车

把磁盘文件想象成一个停车场：

- **定长记录**像标准化停车楼：每个车位一样大，编号连续。要找 37 号车位，不用问任何人，"车位大小 × 37"算一下就知道在哪。
- **变长记录**像路边随意停车：车有长有短，想知道第 37 辆车在哪，只能从头一辆一辆数过去，或者额外立一块"第 N 辆车从 X 米处开始"的指示牌。

数据库面对的记录天然是"变长"的：`VARCHAR(200)` 里可能存 3 个字符也可能存 200 个。但变长带来三个麻烦：

1. **定位慢**：不能直接用槽号算偏移，需要页内维护一张"槽位 → 偏移量"的数组（这就是 PostgreSQL 里的 line pointer / slot directory）；
2. **更新麻烦**：新值比旧值长，原地放不下，要把记录挪走，页内出现碎片，还需要定期整理（PostgreSQL 的 VACUUM 很大程度就是在还这个债）；
3. **跨页问题**：一条记录比一个页还大时，要切成若干段用指针串起来（InnoDB 的溢出页、PostgreSQL 的 TOAST）。

MiniOB 的选择是把问题砍掉：**所有记录都按表结构的最大声明长度定长存放**。`CHAR(10)` 就永远占 10 字节，哪怕只存了 `'a'`。这在 `src/observer/storage/record/record_manager.h` 的注释里写得很直白：

> 从这个页头描述的信息来看，当前仅支持定长行/记录。如果要支持变长记录，或者超长（超出一页）的记录，这么做是不合适的。

代价是空间浪费，换来的是极致的简单：**给定槽号，一次乘法就能算出记录在页内的地址**。

### 1.2 字段 offset：行内怎么找到每一列

记录定长之后，行内每一列的位置也是编译期（建表期）就能算出来的。建表时每个字段的元信息 `FieldMeta`（`src/observer/storage/field/field_meta.h`）记录了：

- `offset()`：该列在一条记录中的起始字节偏移；
- `len()`：该列占多少字节（nullable 字段会 +1，见本章第 7 节）；
- `type()`：类型，决定怎么解释这段字节。

例如 `CREATE TABLE t(id INT, name CHAR(10), score FLOAT)`，三列长度 4、10、4，那么一条 18 字节的记录布局就是：

```
偏移:   0        4              14       18
        | id(4B) | name(10B)    | score(4B) |
```

读 `name` 列就是 `record_data + 4` 起取 10 字节，不需要任何查找。这套"字段 offset 表"的思路在定长和变长系统里都存在，区别只在于：定长系统里 offset 是全局常量，变长系统里每行（或每页）都要单独存一份。

## 2. 页内组织：PageHeader + slot bitmap + 记录区

### 2.1 磁盘按页读写

数据库从不以"字节"为单位读写磁盘，而是以**页（Page）**为单位。MiniOB 的页大小是 128KB，定义在 `src/observer/storage/buffer/page.h`：

```cpp
static constexpr const int BP_PAGE_SIZE      = (1 << 17);  // 128KB
static constexpr const int BP_PAGE_DATA_SIZE = (BP_PAGE_SIZE - sizeof(PageNum) - sizeof(LSN) - sizeof(CheckSum));
```

其中 `PageNum` 是 `int32_t`（4B）、`LSN` 是 `int64_t`（8B）、`CheckSum` 是 `unsigned int`（4B），见 `src/observer/common/types.h`。所以页内可用的数据区是 `131072 - 4 - 8 - 4 = 131056` 字节。页头是 LSN（日志序列号，崩溃恢复用）和校验和，数据区的布局才是 RecordManager 的地盘。

另外约定：**0 号页留给 Buffer Pool 自己存元数据**（有多少页、哪些页已分配），数据页从 1 号开始。所以 `RecordFileScanner` 打开扫描时是 `bp_iterator_.init(buffer_pool, 1)`（`src/observer/storage/record/record_manager.cpp`）。

### 2.2 一个数据页的内部结构

MiniOB 行存页的布局（`record_manager.h` 中 `RowRecordPageHandler` 的注释）：

```
| PageHeader | record allocate bitmap | record1 | record2 | ... | recordN |
```

- **PageHeader**：28 字节，7 个 `int32_t`，见 `record_manager.h` 的 `struct PageHeader`：

| 字段 | 含义 |
|---|---|
| `record_num` | 当前页里已有几条记录 |
| `column_num` | 列数（仅 PAX 格式使用，行存为 0） |
| `record_real_size` | 一条记录的真实字节数 |
| `record_size` | 一条记录实际占用的空间（8 字节对齐后，≥ real_size） |
| `record_capacity` | 本页最多能装几条记录 |
| `col_idx_offset` | 列索引区的偏移（仅 PAX 使用） |
| `data_offset` | 第一条记录的起始偏移 |

- **slot bitmap**：一个位图，第 `i` 位为 1 表示第 `i` 个槽位（slot）上有有效记录，为 0 表示空槽。`capacity` 条记录需要 `(capacity + 7) / 8` 个字节（`page_bitmap_size`，`record_manager.cpp`）。
- **记录区**：`record_size` 字节一格，槽号 `i` 的记录地址由 `RecordPageHandler::get_record_data` 一行算完：

```cpp
char *get_record_data(SlotNum slot_num)
{
  return frame_->data() + page_header_->data_offset + (page_header_->record_size * slot_num);
}
```

这就是定长记录红利的核心：**O(1) 地址计算，无指针、无间接跳转**。

### 2.3 跟着算一遍：一页能装几条记录

`record_manager.cpp` 里容量公式是：

```cpp
int page_record_capacity(int page_size, int record_size, int fixed_size)
{
  // (record_capacity * record_size) + record_capacity/8 + 1 <= (page_size - fix_size)
  return (int)((page_size - PAGE_HEADER_SIZE - fixed_size - 1) / (record_size + 0.125));
}
```

含义：每条记录除了占 `record_size` 字节，还要摊 1/8 字节的 bitmap。`fixed_size` 是 PAX 的列索引占的固定开销，行存为 0。

以上面 `t(id INT, name CHAR(10), score FLOAT)` 为例，跟着算一遍：

1. `record_real_size = 4 + 10 + 4 = 18` 字节；
2. 对齐：`record_size = align8(18) = (18 + 7) & ~7 = 24` 字节（`align8` 见 `record_manager.cpp`）；
3. 容量：`capacity = (131056 - 28 - 0 - 1) / (24 + 0.125) = 131027 / 24.125 ≈ 5431` 条；
4. bitmap 大小：`(5431 + 7) / 8 = 679` 字节；
5. `data_offset = align8(28 + 679) = align8(707) = 712`；
6. 校验：`712 + 5431 × 24 = 131056 = BP_PAGE_DATA_SIZE`，恰好放满，不需要 `fix_record_capacity()` 回退。

所以这张表一个 128KB 的页能装 **5431 条**记录。因为第一个槽位要求 8 字节对齐，`init_empty_page` 最后还会用 `fix_record_capacity()` 兜底：如果最后一条记录会溢出页面，就把 `record_capacity` 减到不溢出为止。

### 2.4 插入、删除、更新在页内发生什么

看 `RowRecordPageHandler` 的三个函数（`record_manager.cpp`），逻辑简单得可以背下来：

**插入** `insert_record`：页满直接返回 `RC::RECORD_NOMEM`；否则在 bitmap 里找第一个 0 位置位，记录数加一，把数据 `memcpy` 进去，标脏页：

```cpp
Bitmap bitmap(bitmap_, page_header_->record_capacity);
int    index = bitmap.next_unsetted_bit(0);
bitmap.set_bit(index);
page_header_->record_num++;
// ... 写日志 ...
char *record_data = get_record_data(index);
memcpy(record_data, data, page_header_->record_real_size);
frame_->mark_dirty();
```

**删除** `delete_record`：只是把 bitmap 对应位清零、`record_num--`。**数据本身一个字节都不动**——旧字节还留在页上，只是"逻辑上不存在"了。这是所有堆组织存储的共同做法：删除是 O(1) 的标记操作，空间留给未来的插入复用。

**更新** `update_record`：定长的又一个红利——新记录和旧记录一样长，直接原地 `memcpy` 覆盖，不需要移动任何其他记录，也不会产生碎片。

**读** `get_record`：返回的 `Record` 直接持有指向页帧（frame）内存的指针，**零拷贝**：

```cpp
record.set_rid(rid);
record.set_data(get_record_data(rid.slot_num), page_header_->record_real_size);
```

`record.h` 的注释特别提醒：这种直接指向页面内存的 `Record`，使用期间必须持有该页的锁（latch），否则页面可能被换出或修改。这也是 `RecordPageHandler::init` 按 `ReadWriteMode` 加读锁/写锁、`cleanup` 里解锁并 `unpin_page` 的原因。

遍历一页用 `RecordPageIterator`：把 bitmap 包一层 `common::Bitmap`，反复调 `next_setted_bit` 跳到下一个有效槽位，空槽直接跳过，不产生任何无效读取。

## 3. RID = (page_num, slot_num)：记录的物理地址

### 3.1 什么是 RID

`src/observer/storage/record/record.h`：

```cpp
struct RID
{
  PageNum page_num;  // record's page number
  SlotNum slot_num;  // record's slot number
  // ...
};
```

`PageNum` 和 `SlotNum` 都是 `int32_t`，所以一个 RID 只有 8 字节，却是一台"全球定位仪"：哪个页、页里哪个槽，两步直达记录。文件本身不需要出现在 RID 里，因为 MiniOB 一张表一个数据文件，RID 天然只在一张表内有意义。

### 3.2 物理地址 vs 逻辑主键的取舍

RID 是一种**物理地址**：它说的是记录"现在躺在哪"，而不是"这条记录是谁"。围绕它有一组经典取舍：

**物理地址（RID）的优点**：

- 快。索引叶子节点存 RID，命中后一次页访问就拿到整行，不需要二次查找；
- 小。8 字节定长，比较、哈希、排序都便宜（`record.h` 还提供了 `RID::compare`、`RID::min()`、`RID::max()` 和 `RIDHash`，MiniOB 的 B+ 树叶子节点存的就是 `(key, RID)`）。

**物理地址的缺点**：

- 记录一旦搬家，所有引用它的索引都要改。定长原地更新的 MiniOB 几乎不搬家，所以问题被掩盖了；但在变长系统里，一次 UPDATE 让行变长换页，全部二级索引都要回写，代价很高。
- 没有逻辑含义。表重建（`ALTER TABLE` 重写、VACUUM FULL）之后 RID 全部失效。

**逻辑主键的路线**（InnoDB）：表按主键组织成一棵 B+ 树（聚簇索引），叶子就是行本身；二级索引叶子存主键值，查二级索引得到主键后再回表查一次。行搬家只影响聚簇索引内部，二级索引纹丝不动——代价是每次二级索引查询多走一棵树。

MiniOB 选 RID，教学上干净利落：索引章节你只需要理解"key → RID → 记录"一条直线。

## 4. 堆文件组织：`free_pages_` 与顺序扫描

### 4.1 堆文件：记录没有顺序

把许多数据页装进一个文件，就是**堆文件（heap file）**。"堆"的意思是：记录想放哪页放哪页，没有任何全局顺序——不是按主键排的，也不是按插入时间严格排的。这是与"索引组织表"（IOT，如 InnoDB 聚簇索引）相对的概念。

堆文件要回答的第一个问题是：**插入一条新记录，往哪页放？**

朴素做法是逐页找"哪页还有空"，O(页数) 一次插入。MiniOB 用 `RecordFileHandler` 里的一个内存集合把这件事变成近似 O(1)：

```cpp
unordered_set<PageNum> free_pages_;  ///< 没有填充满的页面集合
```

工作流程（`RecordFileHandler::insert_record`，`record_manager.cpp`）：

1. 打开表时 `init_free_pages()` 扫一遍所有页，把没满的页号塞进 `free_pages_`（代码注释坦承"这个效率很低，会降低启动速度"——这是用启动时间换运行时速度）；
2. 插入时加锁从 `free_pages_` 取一个页号，打开该页复查一遍 `is_full()`——因为并发下别的线程可能刚把它填满，满了就从集合删掉再取下一个；
3. 集合空了，就向 Buffer Pool `allocate_page` 申请新页、`init_empty_page` 初始化页头，加入 `free_pages_`；
4. 删除成功后，把该页号重新 `insert` 回 `free_pages_`（此时可能并发线程又把它填满了，没关系，第 2 步的复查会兜底）。

这是个教科书式的"近似空闲空间管理"：不保证精确，靠使用时的惰性校验修正，换来无扫描的插入定位。

### 4.2 顺序扫描的代价

没有索引可用时（比如 `WHERE` 列上没建索引），唯一的办法就是**全表扫描**：`RecordFileScanner` 用 `BufferPoolIterator` 从 1 号页开始逐页打开，页内用 `RecordPageIterator` 沿 bitmap 逐条取出，每条再过 `condition_filter_` 过滤、过 `trx_->visit_record` 做 MVCC 可见性判断（见 `RecordFileScanner::fetch_next_record_in_page`）。

代价模型很清楚：N 条记录、每页 C 条，就要读 **N/C 个页**，每条记录做一次谓词判断，总成本 O(N)。堆文件里没有任何"按值跳过"的捷径——这正是索引存在的意义，也是赛题 24（big-order-by）里全表数据必须先被完整扫出来喂给外部排序器的原因。

## 5. 行存 vs 列存 vs PAX

### 5.1 两种负载，两种访问模式

- **OLTP**（联机事务处理）：`UPDATE account SET balance = balance - 100 WHERE id = 42;`——一次摸到**一整行**，点查点写，量小频次高。
- **OLAP**（联机分析处理）：`SELECT region, AVG(sales) FROM orders GROUP BY region;`——一次扫**几千万行的少数几列**，只读，量大频次低。

行存对 OLTP 友好：一整行连续存放，一次页访问拿全所有列。但对 OLAP 很伤：只用 3 列却要为 100 列宽的行付 I/O，90% 以上的带宽在读垃圾。

### 5.2 NSM、DSM、PAX 三种布局

官方设计文档 `docs/docs/design/miniob-pax-storage.md` 给了三幅经典图示，这里用更小的例子复现。假设 4 行 3 列的数据：

```
Row1: a1 b1 c1      Row2: a2 b2 c2      Row3: a3 b3 c3      Row4: a4 b4 c4
```

**NSM（行存）**：行是存储单位，页内一行接一行。

```
Page: | header | a1 b1 c1 | a2 b2 c2 | a3 b3 c3 | a4 b4 c4 |
```

**DSM（纯列存）**：列是存储单位，每列单独连续存放（通常跨文件/跨块）。

```
Block_a: | header | a1 a2 a3 a4 |
Block_b: | header | b1 b2 b3 b4 |
Block_c: | header | c1 c2 c3 c4 |
```

**PAX（Partition Attributes Across，页内列式）**：页还是原来的页，但页内按列分区。

```
Page: | header | a1 a2 a3 a4 | b1 b2 b3 b4 | c1 c2 c3 c4 |
```

PAX 的精妙在于折中：

- 对 OLAP 式扫描，读某一列时，页内该列的值是**连续**的，CPU cache 和预取友好，还能整段压缩、整段喂给 SIMD；
- 对 OLTP 式整行访问，一行仍在一个页内，只需要一次页 I/O，不像纯 DSM 要凑齐 N 个列块才能拼出一行。

### 5.3 为什么列存配向量化执行

火山模型一次 `next()` 吐一行，每行都走一遍"取列 → 解释类型 → 计算"的解释器循环，函数调用和分支预测失败的开销远超数据本身。列存天然把同类型数据排成连续数组，于是可以换一个玩法——**向量化执行**：一次取一批（比如 1000 行）的某一列，放进紧凑数组，循环里只做纯计算，编译器还能自动上 SIMD 指令。

行存也能攒批，但要先把每行的目标列从行里"抠"出来重排成数组，多一次 gather 开销；列存（含 PAX）的数据**在磁盘上就已经是数组**，读出来直接算。这就是为什么 MiniOB 给 PAX 配套的扫描接口不是"一行一行取"，而是 `ChunkFileScanner::next_chunk` 一次取一整页、按列组织成 `Chunk`。

## 6. MiniOB 源码实现

### 6.1 类族谱

`src/observer/storage/record/record_manager.h` 把职责切得很干净：

| 类 | 职责 |
|---|---|
| `RecordFileHandler` | 管理整个数据文件：插入/删除/更新/按 RID 取记录，维护 `free_pages_` |
| `RecordPageHandler` | 单个页的抽象基类：页头、bitmap、加锁、初始化 |
| `RowRecordPageHandler` | 行存页的实现 |
| `PaxRecordPageHandler` | PAX 页内列式实现 |
| `RecordFileScanner` / `RecordPageIterator` | 全文件 / 单页的记录遍历 |
| `ChunkFileScanner` | 按 Chunk（整页按列）遍历，供向量化执行使用 |

`RecordPageHandler` 是个多态基类，用工厂方法按存储格式创建（`record_manager.cpp`）：

```cpp
RecordPageHandler *RecordPageHandler::create(StorageFormat format)
{
  if (format == StorageFormat::ROW_FORMAT) {
    return new RowRecordPageHandler();
  } else {
    return new PaxRecordPageHandler();
  }
}
```

虚函数接口包括 `insert_record`、`delete_record`、`update_record`、`get_record`，以及 PAX 专属的 `get_chunk`——`record_manager.h` 注释明确写着 `get_chunk` "只需由 PaxRecordPageHandler 实现"。基类把公共逻辑（页初始化、页头解析、bitmap 定位、容量计算、加锁解锁）全部收走，子类只写布局相关的读写。

### 6.2 `Record` 与 `RID`

`Record`（`record.h`）本质上就是 `{rid, data 指针, len, owner 标志}`：

- `set_data()` 只是借指针（零拷贝，指向页帧内存，需持锁使用）；
- `set_data_owner()` / `copy_data()` 则自己 `malloc` 一份，可以带离页面生命周期；
- `get_field(field_meta, value)` 演示了怎么从字节序列还原出一列：按 `field_meta.offset()` 定位、按类型解释，nullable 字段先查最后一个字节的 NULL 标志（见第 7 节）。

注意 `RecordFileHandler::get_record` 与页级 `get_record` 的区别：页级是零拷贝借指针，文件级会 `copy_data` 复制一份再返回——因为出了这个函数页面就要解锁，不能再借。

### 6.3 PAX 页：列索引与按列取数

PAX 页布局（`PaxRecordPageHandler` 注释）：

```
| PageHeader | record allocate bitmap | column index | column1 | column2 | ... | columnN |
```

多出来的 **column index** 是一个 `int` 数组，`column_index[i]` 存第 `i` 列的结束偏移（累计值）。初始化时按"列长 × 容量"累加（`init_empty_page`）：

```cpp
column_index[i] = table_meta->field(i)->len() * page_header_->record_capacity + column_index[i - 1];
```

定位第 `slot` 行、第 `col` 列的值，`get_field_data` 两行算完：列基址 = `data_offset + column_index[col-1]`（第 0 列基址就是 `data_offset`），再加 `列长 × slot`。也就是说，PAX 在保留"槽号 → 地址 O(1) 计算"这条定长红利的同时，把记录区从"按行分段"换成了"按列分段"。

需要如实说明：**在当前仓库里，`PaxRecordPageHandler::insert_record`、`get_record`、`get_chunk` 还是标注 `// your code here` + `exit(-1)` 的待实现桩**（`record_manager.cpp`），这正是官方 PAX 实验要求完成的三个函数；`delete_record`（清 bitmap）和 `get_field_data` / `get_field_len`（地址计算）已给出，可对照 `RowRecordPageHandler` 完成。建表语法是 `CREATE TABLE t(a int, b int) storage format=pax;`，不指定时默认行存。

### 6.4 `Chunk` 与 `Column`：向量化的数据容器

`src/observer/storage/common/chunk.h` 和 `column.h`：

- `Column`：同类型、定长的一段连续内存（`data_` + `count_` + `capacity_`），注释写明目前只支持定长类型，默认容量 `DEFAULT_CAPACITY = 8192`；还有 `CONSTANT_COLUMN`（常量列，全批同值，省内存）和 `reference()`（引用别的 Column 的数据，避免拷贝）。
- `Chunk`：一组 `Column` 的集合，代表"一批行"。`chunk.column(i)` 取第 i 列，`get_value(col_idx, row_idx)` 取单个值，`rows()` 是批大小。

`PaxRecordPageHandler::get_chunk` 的语义就是把一页里指定列（由 `chunk.column(i).col_id()` 指定）整块填进 `Chunk`，`ChunkFileScanner::next_chunk` 则逐页驱动它。PAX 页内数据本来就是按列连续的，所以填 Chunk 几乎是大段 `memcpy`——这就是"存储布局决定执行效率"最直观的例子。

## 7. NULL 的物理表示（赛题 13 伏笔）

定长记录里怎么表示"这一列是 NULL"？通用做法有两种：

1. **NULL bitmap**：行头放一张位图，第 i 位为 1 表示第 i 列为 NULL（PostgreSQL 的做法）。优点是每个可空列只花 1 bit；
2. **特殊标记值**：在列的数据区里塞一个不可能出现的值当 NULL。

MiniOB 当前走的是标记值路线的极简版：`FieldMeta` 有 `nullable_` 标志（`src/observer/storage/field/field_meta.h`），**可空字段的声明长度里最后一个字节被征用为 NULL 标志**。写入时（`src/observer/storage/table/base_table.cpp` 的 `make_record`）：

```cpp
if (value.is_null()) {
  if (!field->nullable()) {
    return RC::NOT_NULLABLE_VALUE;
  }
  record_data[field->offset() + field->len() - 1] = '1';
}
```

读出时（`record.h` 的 `Record::get_field`）对称地检查 `data_[field_offset + field_meta.len() - 1] == '1'`，是则 `value.set_null()`；真正的数据长度因此是 `len() - nullable()`。

这个设计教学上够用，但要知道它的代价：每个可空列花 1 字节而非 1 bit，且数据区与标志位耦合。如果让你改进，标准答案就是行头 NULL bitmap——这是面试里"NULL 怎么存"的满分答法，也是理解赛题 13（null）实现边界的关键。

## 8. 复杂度速查

| 操作 | 行存堆文件（MiniOB） | 说明 |
|---|---|---|
| 按 RID 读记录 | O(1)：一次页访问 | 槽号直接算地址 |
| 插入 | 摊还 O(1) | `free_pages_` 直接给页；新页分配摊还 |
| 删除 | O(1) | 清 bitmap 一位 |
| 更新 | O(1) 原地覆盖 | 定长的红利；变长系统可能退化为删 + 插 |
| 全表扫描 | O(N) 条 / O(N/C) 页 | C 为每页记录数，无索引时唯一选择 |
| 打开表 | O(页数) | `init_free_pages()` 全量扫一遍页头 |
| 页内找某列（PAX） | O(1) 定位 + 整列连续读 | 列索引数组 + 列长乘法 |

## 9. 工业数据库怎么做

- **MySQL InnoDB**：页 16KB；表按主键聚簇组织（索引组织表），叶子页就是数据行，二级索引存主键做回表；行格式（COMPACT/DYNAMIC）有变长字段长度列表和 NULL bitmap，超长大字段（TEXT/BLOB）可放到溢出页：COMPACT 格式行内保留 768 字节前缀外加 20 字节溢出页指针，DYNAMIC 格式可以只留 20 字节指针整体外移。是"逻辑主键 + 变长 + 溢出页"全套餐。
- **PostgreSQL**：堆表 + 页内 slot 数组（line pointer）支持变长行；行头有 NULL bitmap；UPDATE 产生新版本行（旧版本留原地等 VACUUM），索引指向行版本的物理位置 ctid（等价于 RID），所以 PG 的 UPDATE 会让所有索引新增条目（HOT 优化除外）；大字段用 TOAST 切片另存。
- **OceanBase**：存储层是 LSM-Tree（MemTable + SSTable 分层合并），宏块 2MB、微块 16KB，微块内采用**行列混存**的编码格式（可对列做字典/前缀等编码），兼据点查与分析扫描；记录定位靠主键在 B+ 树/LSM 结构中查找而非裸 RID。生产系统里纯粹的"定长堆文件 + RID"几乎只出现在教学场景，但页头 + bitmap + slot 的骨架在各家微块/页结构里都能看到影子。

## 小结

- MiniOB 的记录是**全定长**的：字段 offset 建表时固定，槽号 × 记录长 = 记录地址，一切查找退化为乘法。
- 页内三段式：`PageHeader`（28B）+ slot bitmap + 定长记录区；插入找 bitmap 第一个 0 位，删除只清位，更新原地覆盖，读取零拷贝借指针。
- `RID = (page_num, slot_num)` 是 8 字节的物理地址，是 B+ 树索引叶子节点的 value；它的对立面是 InnoDB 的逻辑主键回表。
- 堆文件用 `free_pages_`（内存中的未满页集合）把插入定位做到近似 O(1)，代价是打开表时 O(页数) 的全量扫描；全表扫描则是无索引查询逃不掉的 O(N)。
- 行存服务 OLTP、列存服务 OLAP，PAX 在页内做列式分区两头兼顾；列式布局配向量化执行，因为数据在磁盘上就已经是连续数组，MiniOB 用 `Chunk`/`Column` 承接，用 `ChunkFileScanner` 逐页吐批。
- NULL 当前用"可空列最后一字节置 `'1'`"的标记法，是行头 NULL bitmap 的简化版。

## 面试追问

**问：MiniOB 为什么敢用全定长记录？变长记录到底难在哪？**

答：定长让"槽号 → 地址"变成一次乘法，页内不需要 slot directory，更新永远原地覆盖，删除只清 bitmap，整个 RecordManager 几乎无分支。变长的难点有三：定位需要页内偏移数组；更新变长要搬记录、留碎片，得做页内整理；超页记录要切片加指针（InnoDB 溢出页、PG TOAST）。MiniOB 用空间换掉了这三类复杂度。

**问：删除记录为什么只清 bitmap，数据还留在页上，安全吗？**

答：安全且是通用做法。bitmap 位是"存在性"的唯一权威：扫描迭代器只走 `next_setted_bit`，被清位的槽对外完全不可见，空间会被下一次插入复用覆盖。好处是删除 O(1)、不用移动其他记录、不用改任何 RID。缺点是被删数据的字节物理上还留在磁盘上（有数据残留风险），工业数据库靠加密或覆写解决合规需求。

**问：RID 作主键有什么坑？为什么 InnoDB 不这么干？**

答：RID 是物理地址，记录搬家（变长更新换页、表重建、VACUUM）后所有指向它的索引项全部失效，维护代价随索引个数线性增长。InnoDB 让表按主键聚簇、二级索引存主键值，行搬家只动聚簇索引；代价是二级索引查询要回表。MiniOB 全定长 + 原地更新，记录几乎不搬家，所以 RID 方案简单高效。

**问：一次 UPDATE 在 MiniOB 页内会发生什么？和 PostgreSQL 有何不同？**

答：MiniOB 定长记录新值与旧值等长，`RowRecordPageHandler::update_record` 直接按 RID 找到槽位 `memcpy` 覆盖，无碎片、无索引变更。PG 是 MVCC 追加式 UPDATE：在页内（或新页）写入新版本行，旧版本保留供老事务读，等新版本提交且无人再看旧版本后由 VACUUM 回收，索引通常要加新条目。一个原地覆盖，一个多版本追加，是"定长堆"与"MVCC 堆"的经典分野。

**问：为什么列存要配向量化执行？行存不能做吗？**

答：向量化要的是"一批同类型值的连续数组"，列存/PAX 在磁盘上就已经是这个形态，读出来直接算，还能上 SIMD、做整段压缩。行存要先逐行把目标列抠出来重排成数组（gather），多一层开销，且一次行访问会把不需要的列也带进 cache。行存可以做向量化（攒批），但存储布局不匹配让收益打折。

**问：`free_pages_` 有什么缺陷？工业界怎么管空闲空间？**

答：三个缺陷：一是 `init_free_pages()` 打开表时全扫一遍页头，大表启动慢；二是集合在内存里，重启即失效需重建；三是只记"满/未满"两态，插入可能挑到只剩一个空槽的页，空间利用率不可控。工业界如 PostgreSQL 用 FSM（Free Space Map）按"空闲字节数分档"持久化记录每页余量，查找时按所需空间直达合适的页。

## 回到赛题

- **赛题 17 text（[解析](../04_problems_17_24.md)）**：TEXT 在本仓库按 **65535 字节定长内联**存储（parser 在 `yacc_sql.y` 把 TEXT 长度定为 65535，`base_table.cpp` 注释"text 类型最多存 65535 字节"）。用本章公式算一下代价：`record_size = align8(4 + 65535) = 65544`，`capacity = (131056 - 28 - 1) / (65544 + 0.125) ≈ 1`——**一个 128KB 的页只装得下一条记录**，页内近一半空间浪费，扫 1 万条带 TEXT 的记录就要读 1 万个页。这就是"定长的代价"被推到极限的样子，也是理解为什么工业数据库用溢出页/TOAST 把大字段挪出行外的最好案例。

- **赛题 24 big-order-by（[解析](../04_problems_17_24.md)）**：外部排序的第一步是用 `RecordFileScanner` 把全表从堆文件里扫出来——本章的 O(N) 顺序扫描就是它的输入阶段。排序键、记录都以定长字节块形式被序列化进 run 文件；而赛题解析里提到的"serializer 不支持 TEXTS"正源于本章：TEXT 把记录撑到 64KB，外排的 tuple 序列化器没有为它准备变长/超长编码。读懂堆文件扫描的代价模型，才能解释为什么 big-order-by 的总成本下界是"扫一遍全表 + O(N log N) 比较 + run 文件读写 I/O"。

- **赛题 13 null（[解析](../03_problems_09_16.md)）**：本章第 7 节是它的存储层伏笔——可空字段最后一字节存 `'1'` 标志，`make_record` 写入、`Record::get_field` 读出。理解了"NULL 在物理层只是一个字节标记"，才能继续讨论它在表达式层的三值逻辑、在索引层的存放方式，以及为什么 `NULL = NULL` 不为真。
