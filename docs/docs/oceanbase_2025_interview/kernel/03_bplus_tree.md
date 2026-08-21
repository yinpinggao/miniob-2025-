# 索引与 B+ 树：数据库最重要的数据结构

## 本章导读

如果只能选一个数据结构代表数据库内核，那一定是 B+ 树。MySQL InnoDB、PostgreSQL、OceanBase 的索引底层都是它（或其变体）。面试官问"数据库为什么快"，标准答案不是"因为用 C++ 写的"，而是"因为索引把 O(N) 的全表扫描变成了 O(log N) 的查找"。

本章面向零基础读者，从"为什么需要索引"讲起，沿着哈希表、二叉搜索树、B 树一路演进到 B+ 树，讲清楚它的结构、查找/插入/删除全流程、并发控制（蟹行协议）和崩溃恢复（WAL），最后落到 MiniOB 的真实源码上。读完你应该能徒手算树高、徒手推分裂合并过程，并能对着 `src/observer/storage/index/bplus_tree.h` 讲出每个类是干什么的。

建议配合两份官方设计文档阅读：[MiniOB B+Tree 实现](../../design/miniob-bplus-tree.md) 与 [B+ 树并发操作](../../design/miniob-bplus-tree-concurrency.md)，本章会在对应小节引用它们而不重复其内容。

## 为什么需要索引：全表扫描为什么不可接受

先给直觉。假设 `student` 表有 100 万行，你要执行：

```sql
SELECT * FROM student WHERE id = 123456;
```

没有索引时，数据库只能一行一行地翻：读第一行，看 `id` 是不是 123456，不是就下一行……平均要翻 50 万行。磁盘上一次 I/O 大约读一个页，假设一页放 100 行，就要约 5000 次磁盘 I/O。机械磁盘一次 I/O 约 10ms，加起来就是 50 秒——用户早就关掉网页了。

这就是**全表扫描**，时间复杂度 O(N)。N 越大越不可接受，而真实业务的 N 是千万、亿级。

索引的思想和字典一模一样：字典不会从第一页开始翻"数据库"三个字，而是先查拼音/部首目录，直接定位到某一页。索引就是数据库为表预先维护好的一份"目录"，用一个排好序的数据结构把"键 → 记录位置"存起来，让查找变成 O(log N)。

> 面试一句话版：索引是用**空间**和**写入时的维护开销**，换取**查询**时的数量级提速。它本质是一份冗余的、按查询键有序组织的数据。

## 从哈希到 B+ 树：索引结构的演进

为什么不直接用教科书里现成的数据结构？逐个分析它们当索引的问题，就能理解 B+ 树为什么赢。

### 哈希索引：O(1) 但只会等值

哈希表做等值查询是 O(1)，看起来完美。但它有两个硬伤：

- **不支持范围查询**。哈希值把键的顺序彻底打乱了，`WHERE id BETWEEN 100 AND 200` 在哈希表里无从下手，只能退化为全表扫描。
- **不支持排序和前缀匹配**。`ORDER BY`、`LIKE 'abc%'`、最小/最大值都无法利用哈希。

数据库的查询负载里范围扫描极其常见，所以纯哈希索引（如 PostgreSQL 的 HASH index、InnoDB 内部的自适应哈希）只能是配角。

### 二叉搜索树 / 平衡二叉树：有序但太"瘦高"

二叉搜索树（BST）有序，中序遍历就是排序结果，等值和范围都能做。平衡版本（AVL、红黑树）保证树高 O(log N)。但它有一个致命弱点：**扇出（fanout）只有 2**。

100 万个键，AVL 树的高度约 log2(1000000) ≈ 20。每个节点一次磁盘 I/O 的话，一次查找要 20 次 I/O——还是太慢。问题的根源在于：二叉树是为内存设计的，内存里访问一个节点只要纳秒级；而磁盘的特征是"读一个字节和读一个页（几 KB）成本几乎一样"。树越矮，I/O 次数越少，这才是磁盘数据结构的第一优化目标。

### B 树：多叉平衡，但数据放错了地方

B 树把二叉变成多叉：一个节点（对应一个磁盘页）里塞几百上千个键，扇出从 2 变成几百。扇出 f、N 个键时树高约 log_f(N)，f=500 时 3 层就能装下一亿多键——I/O 次数从 20 降到 3。

B 树的每个节点既存键也存数据指针（或数据本身）。这带来两个问题：

- 内部节点存了数据，能放下的键变少，扇出被拉低，树变高；
- 范围扫描时，B 树只能中序遍历，在节点之间来回跳，顺序 I/O 变成随机 I/O。

### B+ 树：为磁盘和范围扫描而生

B+ 树在 B 树基础上做了两个关键改造：

1. **数据只放在叶子节点**。内部节点只存键（路标），不存数据。内部节点变"轻"了，一个页能塞下更多键，扇出更高，树更矮。
2. **叶子节点之间用链表串起来**。范围查询定位到左端点的叶子后，顺着链表向右扫即可，完全是顺序访问。

对比一下四种结构：

| 结构 | 等值查询 | 范围查询 | 扇出 | 磁盘友好度 |
| --- | --- | --- | --- | --- |
| 哈希表 | O(1) | 不支持 | — | 差（随机 I/O） |
| 平衡二叉树 | O(log N) | O(log N + k) | 2 | 差（树太高） |
| B 树 | O(log N) | O(log N + k)，中序遍历跳节点 | 中 | 较好 |
| B+ 树 | O(log N) | O(log N + k)，叶子链表顺序扫 | 高 | 最好 |

> 面试一句话版：B+ 树 = 高扇出（压低树高，减少 I/O）+ 数据全在叶子（内部节点更轻）+ 叶子链表（范围扫描变顺序 I/O）。

## B+ 树结构精讲

### 节点结构

一棵 B+ 树只有三种角色：

```
            [ 内部节点: 只存 键 | 子节点页号 ]        <- 路标
           /          |           \
   [内部节点]     [内部节点]      [内部节点]
    /    \         /    \         /    \
 [叶子] [叶子]  [叶子] [叶子]  [叶子] [叶子]        <- 数据
   键|RID 键|RID ... 键|RID   键|RID 键|RID
     <-------> <-------> <-------> <------->       <- 双向/单向链表
```

- **内部节点（internal node）**：存 `键 + 子节点页号` 的键值对。键只是"路标"，告诉你"小于它往左走，大于等于它往右走"。
- **叶子节点（leaf node）**：存真正的索引项 `键 + 记录位置`。在 MiniOB 里这个位置就是 `RID`（页号 + 槽位号，定义见 `src/observer/storage/record/record.h`）。
- **阶 / 扇出（order / fanout）**：一个节点最多能容纳多少键值对，由页大小和键大小决定。工程上不固定阶数，而是"一页能放多少就放多少"。

还有两个重要不变量：

- 所有叶子节点在**同一层**（B+ 树是完美平衡的，任何查找的 I/O 次数都相同）；
- 除根节点外，每个节点至少是**半满**的（键值对数 ≥ 最大容量的一半，向上取整），这保证了空间利用率不低于 50%，也保证了分裂/合并操作的存在性。

### 数字演算：为什么百万条记录只要 3 层

这是面试必考的心算题。用一个偏保守的经典假设：页大小 16KB，键 + 指针约 16 字节，内部节点扇出 f ≈ 1000；叶子节点一页放约 100 条索引项（真实数据更宽）。

一层一层往上数叶子能装多少条记录：

- 1 层（只有根叶子）：100 条；
- 2 层：1000 个叶子 × 100 = 10 万条；
- 3 层：1000 × 1000 个叶子 × 100 = 1 亿条；
- 4 层：1000³ × 100 = 1000 亿条。

**结论：3 层 B+ 树足以索引亿级记录，一次查找只需 3 次磁盘 I/O**（而且根节点和第二层常驻内存 Buffer Pool，实际常常只有 1 次真实 I/O）。对比全表扫描的几十万次 I/O，这就是索引的全部意义。

MiniOB 的页比经典设置大得多：`BP_PAGE_SIZE = (1 << 17)` 即 **128KB**（见 `src/observer/storage/buffer/page.h:26`）。容量在创建索引时按真实键长计算，公式就在 `src/observer/storage/index/bplus_tree.cpp` 顶部：

```cpp
int calc_internal_page_capacity(int attr_length)
{
  int item_size = attr_length + sizeof(RID) + sizeof(PageNum);   // 键 + RID + 子页号
  int capacity  = ((int)BP_PAGE_DATA_SIZE - InternalIndexNode::HEADER_SIZE) / item_size;
  return capacity;
}
int calc_leaf_page_capacity(int attr_length)
{
  int item_size = attr_length + sizeof(RID) + sizeof(RID);       // 键 + 两个 RID
  int capacity  = ((int)BP_PAGE_DATA_SIZE - LeafIndexNode::HEADER_SIZE) / item_size;
  return capacity;
}
```

代入一个 4 字节的 INT 键（`sizeof(RID)` 为 8 字节：页号 4 + 槽位 4；`sizeof(PageNum)` 为 4）：内部节点每项 16 字节，容量约 (131060 − 12) / 16 ≈ **8190**；叶子每项 20 字节，容量约 6500。也就是说 MiniOB 里两层 B+ 树（8190 个叶子 × 约 6500 项）就能索引 **五千多万** 条记录，三层更是到千亿量级。页越大扇出越高，这就是为什么教学库也敢用大页。

### MiniOB 的磁盘布局

MiniOB 把"一个节点 = 一个页"贯彻得很彻底（详见 [MiniOB B+Tree 实现](../../design/miniob-bplus-tree.md)）。索引文件第一个页是文件头 `IndexFileHeader`（`bplus_tree.h:175`），记录根页号、内部/叶子节点最大容量、键长等元信息；从第二个页（`FIRST_INDEX_PAGE = 1`）开始才是树节点。

每个节点页以公共头部 `IndexNode`（`bplus_tree.h:207`）开头，只有 12 字节（`HEADER_SIZE = 12`）：`is_leaf`、`key_num`、`parent`（父节点页号）。两种节点在头部之后各存各的：

- 叶子节点 `LeafIndexNode`（`bplus_tree.h:229`）：头部再加一个 `next_brother` 页号（4 字节），之后顺序存放 `key, rid` 对。注意当前代码只有**右兄弟**指针，是单向链表；官方设计文档里的插图仍画着 `prev_brother`，以代码为准。
- 内部节点 `InternalIndexNode`（`bplus_tree.h:251`）：顺序存放 `key, page_id` 对，且**第一个键无效**（`key0` 被忽略，只用它旁边的 `page_id`），这是 B+ 树"n 个键配 n+1 个子指针"的常见实现技巧——MiniOB 选择存 n 个键和 n 个指针，牺牲一个键的空间换来整齐的布局。

## 查找、插入、删除全流程

用一个小例子走一遍。设每个节点最多 3 项（max_size = 3，半满线 min_size = 2），依次插入键 5、10、15、20、25、30。

### 查找：从根到叶的"下楼"

查找 `key = 20`：

1. 读根节点页，在键数组里**二分查找**第一个大于 20 的路标，沿其左侧子指针下楼；
2. 重复直到叶子节点；
3. 在叶子内二分查找 20，命中则返回对应的 RID，否则返回"不存在"。

每次"下楼"正好是一次页的读取，查找成本 = 树高次 I/O + 页内二分（内存操作，可忽略）。

### 插入：先找位置，满了就分裂

插入永远发生在叶子。流程：

1. 用查找流程定位到目标叶子；
2. 叶子没满：把键值对插进有序数组的合适位置（挪动后面的元素），结束；
3. 叶子满了：**分裂（split）**——新建一个右兄弟节点，原节点留前一半，后一半搬到新节点，接好叶子链表，然后把"新节点的最小键 + 新节点页号"**向上插入父节点**；
4. 父节点若因此也满，就递归地分裂父节点；一路分裂到根时，新建一个根节点，树长高一层。

插入 25 触发叶子分裂的示意：

```
插入 25 前(叶子满):                插入 25 后:
        [15 | ...]                       [20 | ...]        <- 父节点多了路标 20
        /   |    \                       /    |     \
   [5,10] [15,20] [30,...]   ->   [5,10] [15] [20,25] [30,...]
```

注意 B+ 树**只通过根分裂长高**，这就是它永远完美平衡的原因——所有叶子始终在同一层。

### 删除：先删键，太空就合并或再分配

删除同样先定位到叶子，删掉键值对，然后检查节点是否跌破半满线（`key_num < min_size`）：

1. 没跌破：直接结束；
2. 跌破了，看相邻兄弟（优先左兄弟，最左节点则取右兄弟）：
   - **能合并**：两节点的键值对加起来 ≤ max_size，就把右节点的数据全部搬进左节点，摘除链表上的右节点，并**从父节点删掉指向右节点的那个路标**。父节点因此可能也跌破半满线，递归向上处理；根节点若只剩一个孩子，就让这个孩子成为新根，树变矮一层。
   - **不能合并（再分配 redistribute）**：两节点加起来超过 max_size，就从富余的兄弟那里"借"一个键值对过来（左兄弟的最后一个，或右兄弟的第一个），同时更新父节点中对应的路标键。两节点都回到半满以上，操作结束。

MiniOB 源码里这三步分别对应 `BplusTreeHandler::coalesce_or_redistribute`、`coalesce`、`redistribute`（`bplus_tree.h:592-607`），注释写得很直白："当节点中的键值对小于最小值时，需要合并或重新分配"。

### 复杂度小结

设 N 为索引项总数，f 为扇出，单次操作涉及的页数：

| 操作 | I/O 复杂度 | 说明 |
| --- | --- | --- |
| 查找 | O(log_f N) | 恰好等于树高 |
| 插入 | O(log_f N) | 分裂沿路径向上，均摊 O(1) 次分裂 |
| 删除 | O(log_f N) | 合并/再分配沿路径向上 |
| 范围扫描 k 条 | O(log_f N + k/B) | B 为每页索引项数，叶子链表保证顺序 |

## 键的设计：为什么 key = 字段值 + RID

### 让键唯一

普通索引允许重复值：`age` 列上可能有一万个人都是 20 岁。但 B+ 树内部需要靠键来精确定位一个索引项（尤其是删除时），如果键不唯一，删除"age=20 的某一行"就不知道删哪一项。

MiniOB 的解法是把**记录的位置 RID 拼进键里**，注释写在 `KeyComparator` 头上（`bplus_tree.h:88-91`）："BplusTree的键值除了字段属性，还有RID，是为了避免属性值重复而增加的"。`KeyComparator::operator()`（`bplus_tree.h:129-139`）的比较逻辑是两段式的：

```cpp
int operator()(const char *v1, const char *v2) const
{
  auto result = compare_key(v1, v2);      // 先逐字段比较用户键
  if (result != 0) {
    return result;
  }
  const RID *rid1 = (const RID *)(v1 + index_.fields_total_len());
  const RID *rid2 = (const RID *)(v2 + index_.fields_total_len());
  return RID::compare(rid1, rid2);        // 用户键相同，再比 RID 决胜
}
```

这样树中任意两项都不同，等值查找"age=20"会命中一段连续的键（它们用户键相同、RID 不同），`BplusTreeHandler::get_entry` 能把这段 RID 全部收集回来。这个设计与 InnoDB 的二级索引"键 + 主键值"异曲同工——InnoDB 二级索引的叶子存的是主键值，MiniOB 存的是物理位置 RID。

### 复合索引与最左前缀：电话簿类比

复合索引 `(last_name, first_name)` 就是电话簿：先按姓排，姓相同再按名排。这带来著名的**最左前缀原则**——索引 `INDEX(a, b, c)` 能加速：

- `WHERE a = 1`（用第 1 列）；
- `WHERE a = 1 AND b = 2`（用前 2 列）；
- `WHERE a = 1 AND b = 2 AND c = 3`（全用上）；
- `WHERE a = 1 AND c = 3`（只能用 a，c 只能扫出来再过滤）；

但**帮不上** `WHERE b = 2` 或 `WHERE c = 3`——跳过姓直接在电话簿里找名叫"伟"的人，只能全簿翻一遍，因为数据不是按名排序的。

字典序的实现就在 `KeyComparator::compare_key`（`bplus_tree.h:104-127`）：按 `IndexMeta::fields()` 的顺序逐字段比较，第一个不相等的字段决定大小，全部相等才算平。`IndexMeta`（`src/observer/storage/index/index_meta.h`）用 `vector<FieldMeta> fields_` 保存所有索引列、`fields_offset_` 记录每列在键里的偏移，`make_entry_from_record` 负责把一条记录拼接成索引键——这正是 multi-index 赛题要动的核心。另外可以看到比较逻辑里还处理了 `nullable()`：可空字段用键末尾的标记位区分 NULL，且 NULL 被排为最小（`v1_is_null` 时直接返回 -1）。

## 并发控制：latch 与蟹行协议

### latch 不是 lock

这是面试高频概念辨析，一句话区分：

| | latch（闩） | lock（锁） |
| --- | --- | --- |
| 保护对象 | 内存中的物理数据结构（B+ 树页、Buffer Pool 页帧） | 逻辑数据（行、表、谓词范围） |
| 持有时间 | 极短（一次物理操作内，纳秒~微秒级） | 长（整个事务，直到 commit/rollback） |
| 实现 | 互斥锁、读写锁、自旋锁 | 锁管理器（锁表、死锁检测） |
| 回滚 | 不参与，物理结构不需要"事务回滚" | 与事务隔离级别、2PL 绑定 |

类比：lock 是酒店房间的预订（整个入住期间归你），latch 是你进电梯时挡一下门（用几秒就放）。B+ 树并发控制要保护的是"页"这种物理结构，用的是 latch，具体是**读写 latch**：读共享（S）、写互斥（X）。

### 蟹行协议（crabbing protocol）

最朴素的并发方案是"整棵树加一把大锁"，正确但串行，白瞎了多核。蟹行协议（也叫 lock coupling，"锁耦合"）的思路是：**像螃蟹横着走一样，抓住下一只脚才松开上一只脚**——从根开始，先给子节点加好 latch，再决定要不要放掉父节点的 latch。

关键在于**安全节点（safe node）**判定：如果对当前节点的这次操作**保证不会波及父节点**（不会分裂、不会合并），那么父节点及更上层的 latch 现在就可以全部释放。具体规则（B+ 树单次只插/删一个键）：

- 读操作：任何节点都是安全的（读不改结构）；
- 插入：节点当前 `size < max_size`，即再插一个也不会分裂 → 安全；
- 删除：节点 `size > min_size`，即删一个也不会触发合并 → 安全。

一次插入的加锁轨迹（X 表示写 latch）：

```
root[X] -> internal[X] -> leaf[X]
              ↑ 若 internal 安全: 拿到 internal 后立刻释放 root
                        若 leaf 安全:    拿到 leaf 后立刻释放 internal
```

最坏情况（路径上所有节点都濒临分裂）才需要把 latch 从根一路攥到叶子；绝大多数操作下，下几层楼就能放掉上面的 latch，并发度大大提高。协议规定**只能从父到子加锁**这一固定方向，所以天然不会死锁——死锁需要循环等待，而"父先子后"是偏序的。

唯一需要特殊处理的是**叶子链表的横向扫描**：扫描沿着 `next_brother` 向右走，加锁方向是"兄弟到兄弟"，与更新操作"父到子"的方向组合起来可能死锁（扫描持有左叶等右叶，更新持有右叶等父节点……）。MiniOB 的做法是 `try_slatch` 尝试加锁，拿不到就放弃本次扫描、稍后重试，用活锁风险换死锁免疫。这段分析的细节直接来自官方文档 [B+ 树并发操作](../../design/miniob-bplus-tree-concurrency.md)，面试前值得通读。

### MiniOB 代码里的蟹行

判定逻辑原封不动地写在 `IndexNodeHandler::is_safe`（`bplus_tree.cpp:102-129`）：

```cpp
bool IndexNodeHandler::is_safe(BplusTreeOperationType op, bool is_root_node)
{
  switch (op) {
    case BplusTreeOperationType::READ:    return true;
    case BplusTreeOperationType::INSERT:  return size() < max_size();
    case BplusTreeOperationType::DELETE: {
      if (is_root_node) {
        if (node_->is_leaf) return size() > 1;   // 根叶子删空意味着删整棵树
        return size() > 2;                        // 根内部节点只剩一个孩子时要降树高
      }
      return size() > min_size();
    }
    ...
  }
}
```

逐行看：读永远安全；插入在"还没满"时安全（插完正好满也没关系，满才分裂）；删除区分了根节点——根的半满规则特殊，所以单列分支，根内部节点要求 `size > 2`（对应"合并后根只剩一个孩子就要换根"的场景）。

蟹行的"抓子放父"在 `BplusTreeHandler::crabing_protocal_fetch_page`（`bplus_tree.cpp:1213` 附近，函数名里的拼写 "protocal" 在源码里就是如此）里：

```cpp
LatchMemoType latch_type = readonly ? LatchMemoType::SHARED : LatchMemoType::EXCLUSIVE;
mtr.latch_memo().latch(frame, latch_type);       // 1. 先给子节点加 latch
IndexNodeHandler index_node(mtr, file_header_, frame);
if (index_node.is_safe(op, is_root_node)) {
  latch_memo.release_to(memo_point);             // 2. 安全则释放所有祖先 latch
}
```

`release_to` 会顺着 `LatchMemo` 的记录把进入当前节点之前持有的祖先 latch 和 pin 全部放掉——这就是"松开上一只脚"。

### LatchMemo：把加过的锁记在小本本上

蟹行过程中一个操作会碰很多页：pin 住若干 `Frame`、加若干读写 latch。谁来保证它们最终被释放、不泄露？MiniOB 的答案是 `LatchMemo`（`src/observer/storage/index/latch_memo.h:51`）——一个"记账本"：

- `get_page`：取页并 pin（引用计数 +1），记入账本；
- `xlatch / slatch / try_slatch`：加写/读 latch，记入账本（`LatchMemoType` 枚举区分 `SHARED`、`EXCLUSIVE`、`PIN`）；
- `release_to(point)`：释放账本中 point 之后的所有记录（蟹行放祖先用）；
- `release()` / 析构：全部释放。整个操作结束（无论成功失败）一次性清账，杜绝资源泄露。

注意释放顺序：必须**先解锁再 unpin**。反过来，unpin 后 Frame 可能被 Buffer Pool 淘汰复用，此时再解锁就是在操作别人的页了。这也是官方文档留给读者的思考题之一。

另外根页号本身存在文件头里，不受任何 Frame latch 保护，所以 `BplusTreeHandler` 单独用一把 `common::SharedMutex root_lock_`（`bplus_tree.h:646`）保护根页号的读写，加锁记录同样可以交给 `LatchMemo` 管理。

## B+ 树的 WAL 与 BplusTreeMiniTransaction

### 问题：B+ 树操作是"多页写"

一次插入可能分裂叶子、改父节点、甚至新建根节点——**多个页要么全部生效，要么全部不生效**。如果改到一半断电，磁盘上留下一棵撕裂的树：叶子链表断掉、父节点路标指向不存在的页。单靠事务的 undo/redo 还不够，因为 B+ 树结构变化（SMO，Structure Modification Operation）和普通行修改交织在一起，需要专门的日志。

WAL（Write-Ahead Logging，预写日志）的原则一句话：**任何脏页刷盘之前，描述这次修改的日志必须先落盘**。崩溃后重放（redo）日志即可把树修回一致状态。

### MiniOB 的做法：BplusTreeMiniTransaction

MiniOB 把"一次 B+ 树操作"包装成一个**迷你事务（mini-transaction, mtr）**，这个思想直接借鉴自 InnoDB 的 mtr。`BplusTreeMiniTransaction`（`src/observer/storage/index/bplus_tree_log.h:169`）非常瘦，只有两个成员：

```cpp
class BplusTreeMiniTransaction final
{
  ...
private:
  BplusTreeHandler &tree_handler_;
  RC               *operation_result_ = nullptr;
  LatchMemo         latch_memo_;   // 并发控制：记 latch 和 pin 的账本
  BplusTreeLogger   logger_;       // 崩溃恢复：记 redo 日志
};
```

一个 mtr = 一把 latch（`LatchMemo`）+ 一叠日志（`BplusTreeLogger`），正好回答了"B+ 树操作要协调哪两件事"——并发一致性和崩溃一致性。`BplusTreeLogger`（`bplus_tree_log.h:61`）为每种物理修改提供对应的日志方法：`init_header_page`、`update_root_page`、`node_insert_items`、`node_remove_items`、`leaf_init_empty`、`leaf_set_next_page`、`internal_init_empty`、`internal_create_new_root`、`internal_update_key`、`set_parent_page`。其类注释（`bplus_tree_log.h:46-60`）把设计讲得清清楚楚：

- 一次插入/删除产生的所有日志先在内存里攒着，操作全部完成后**一次性写入日志文件**，形成一个"大日志"，要么都成功要么一起回滚；
- 保证脏页刷盘前日志已落盘（WAL 的本义）；
- 中途失败用 `rollback` 把已做的内存修改逆向抹掉；
- 重启恢复时由 `BplusTreeLogReplayer::replay`（`bplus_tree_log.h:197`）调用 `BplusTreeLogger::redo` 重放；重放和回滚期间复用同一套 B+ 树接口，靠 `need_log_` 标志关闭二次记日志。

> 面试时如果被问"MiniOB 的 B+ 树怎么保证崩溃一致性"，答：每个结构修改操作包在 `BplusTreeMiniTransaction` 里，修改前先经 `BplusTreeLogger` 记录 redo 日志并按 WAL 先落盘，重启时由 `BplusTreeLogReplayer` 重放恢复；思想与 InnoDB 的 mini-transaction 一致。

## MiniOB 源码实现总览

把上面的零件拼成全景图。B+ 树相关代码全部在 `src/observer/storage/index/` 下：

```
Index (index.h)                        <- 索引抽象基类: insert_entry/delete_entry/create_scanner
 └── BplusTreeIndex (bplus_tree_index.h)        <- B+树索引外壳，对接 Table
      └── BplusTreeHandler (bplus_tree.h)       <- B+树本体: insert_entry/delete_entry/get_entry
           ├── IndexNodeHandler                 <- 节点操作基类（数据与操作分离）
           │    ├── LeafIndexNodeHandler        <- 叶子: lookup/insert/remove/move_half_to/...
           │    └── InternalIndexNodeHandler    <- 内部: lookup/create_new_root/insert/...
           ├── BplusTreeScanner                 <- 范围扫描器（叶子链表遍历）
           ├── BplusTreeMiniTransaction (mtr)
           │    ├── LatchMemo                   <- latch/pin 账本
           │    └── BplusTreeLogger             <- redo 日志
           └── KeyComparator / AttrComparator   <- 键比较: 字段字典序 + RID 决胜
```

几个值得在面试中展开的设计点：

- **数据与操作分离**：`IndexNode`/`LeafIndexNode`/`InternalIndexNode` 是纯内存布局的结构体（注意 `char array[0]` 这种柔性数组写法，数据紧跟头部存放），不含任何方法；所有操作由 `IndexNodeHandler` 家族完成。`bplus_tree.h:261-266` 的注释解释了原因：虚函数会改变结构体的内存布局，页数据必须保持纯净的二进制格式。`LeafIndexNodeHandler` 和 `InternalIndexNodeHandler` 都是 `final` 类。
- **分裂/合并用模板复用**：`split`、`coalesce_or_redistribute`、`coalesce`、`redistribute` 都是模板函数（`template <typename IndexNodeHandlerType>`），叶子的搬一半（`move_half_to`）、借首借尾（`move_first_to_end`/`move_last_to_front`）和内部节点的同名操作共用一套流程。
- **Scanner 即范围查询**：`BplusTreeScanner::open`（`bplus_tree.h:678`）接受左右边界键和是否包含边界的开关，`next_entry` 沿叶子链表逐项返回 RID，直到 `touch_end()`。SQL 层的 `WHERE age BETWEEN 20 AND 30` 最终就是它。注意其注释里的警告：遍历时不允许删除数据，否则迭代器失效——这是一个已知限制的诚实标注。
- **唯一性在索引外壳层检查**：`BplusTreeIndex::insert_entry`（`src/observer/storage/index/bplus_tree_index.cpp:89-137`）先用 `make_entry_from_record` 拼出键，若 `IndexMeta::unique()` 为真，先 `get_entry` 找候选冲突 RID，再经当前事务的可见性判断（MVCC 下旧版本不算冲突），确认冲突则返回 `RC::RECORD_DUPLICATE_KEY`。B+ 树本体不知道"唯一"这回事——职责分得很干净。

调试方面，`BplusTreeHandler::print_tree()`、`validate_tree()` 可以打印和校验整棵树（线程不安全，仅限调试用）；`unittest/observer` 与 `benchmark/bplus_tree_concurrency_test.cpp` 里有单线程正确性测试和多线程并发压测。

## 工业界对比：InnoDB 与 LSM-tree

### InnoDB：聚簇索引与回表

MiniOB 的 B+ 树叶子存 `键 + RID`，记录本身另存在表数据文件里，这种组织方式叫**二级索引（非聚簇索引）**形态。InnoDB 更进一步：

- **聚簇索引（主键索引）**：叶子节点直接存**整行数据**，表数据本身就是一棵按主键组织的 B+ 树。主键查找一次到位，不需要二次查找。
- **二级索引**：叶子存 `索引键 + 主键值`。用二级索引查到主键后，还要再去聚簇索引里按主键查一次整行——这个动作就是面试必考的**回表**。
- **覆盖索引**：如果查询要的列都在二级索引的键和主键里（`SELECT id, age` 用 `INDEX(age)`），就不用回表，这是一次重要的优化。

MiniOB 没有聚簇索引（`LeafIndexNode` 注释里甚至留了思考题 "can you implement a cluster index?"），所有索引都是 `键 + RID` 形态，取记录要按 RID 再访问堆表，相当于**每次索引查询都回表**。

### LSM-tree：一句话对比

B+ 树为读优化：查询快，但写入是"原地更新"，随机写多。LSM-tree（RocksDB、LevelDB、HBase，以及 OceanBase 的存储底座）为写优化：写入先落内存 MemTable 和 WAL，再批量顺序刷成磁盘上的有序 SSTable 分层合并，把随机写变成顺序写，代价是读要查多层（用布隆过滤器缓解）且有写放大。一句话：**B+ 树读快写慢、原地更新；LSM-tree 写快读慢、追加合并**。OceanBase 这样的分布式数据库选择 LSM-tree，正是在"写入吞吐"和"合并代价"之间做的另一种权衡。

## 小结

- 索引的意义是把查找从 O(N) 降到 O(log N)；B+ 树凭"高扇出 + 数据全在叶子 + 叶子链表"战胜哈希、二叉树和 B 树，成为磁盘索引的标准答案。
- 树高 ≈ log_f(N)，扇出几百时 3 层即可索引亿级记录；MiniOB 页 128KB，INT 键扇出约 8000，两层就能装五千万条。
- 插入分裂、删除合并/再分配都沿查找路径向上传播，树只在根处长高/变矮，因此永远平衡。
- MiniOB 的键 = 字段值 + RID，保证键唯一；复合索引按字段顺序做字典序比较，遵循最左前缀原则（电话簿类比）。
- 并发用蟹行协议：读写 latch、从父到子加锁、安全节点提前释放祖先 latch；`LatchMemo` 记账防泄露，先解锁再 unpin。
- 崩溃恢复靠 WAL：每次结构修改包在 `BplusTreeMiniTransaction`（=`LatchMemo` + `BplusTreeLogger`）里，日志先落盘，重启由 `BplusTreeLogReplayer` 重放。
- 工业界：InnoDB 的聚簇索引/二级索引引出回表与覆盖索引；LSM-tree 用读性能换写吞吐，是 B+ 树之外的另一条路线。

## 面试追问

**Q1：为什么 B+ 树比 B 树更适合做数据库索引？**

A：两点。第一，B+ 树内部节点只存键不存数据，同样大小的页能放更多键，扇出更高、树更矮，查找 I/O 更少；第二，所有数据都在叶子且叶子间有链表，范围查询定位一次后顺序扫描即可，而 B 树要中序遍历在节点间跳跃，把顺序 I/O 退化成随机 I/O。另外 B+ 树任何查询都要走到叶子，查询代价稳定。

**Q2：一棵 B+ 树，扇出 500，叶子每页 100 条，存 1 亿条记录要几层？**

A：叶子层需要 1亿/100 = 100 万个叶子页；往上每层除以 500：100万 → 2000 → 4 → 1。内部三层加叶子一层共 4 层；若扇出按 1000 估则 3 层就够。核心算法是逐层除以扇出，根和上层常驻内存时实际磁盘 I/O 只有 1~2 次。

**Q3：latch 和 lock 有什么区别？**

A：latch 保护物理数据结构（B+ 树页、缓冲池页帧），持有时间极短，用互斥锁/读写锁实现，不参与事务回滚；lock 保护逻辑数据（行、表），由锁管理器管理，持有到事务结束，与隔离级别和死锁检测绑定。类比：latch 是进电梯时挡一下门，lock 是酒店房间的预订。

**Q4：讲讲蟹行协议，什么情况下要一路加锁到根？**

A：蟹行协议从根向下，先加子节点 latch，若子节点对本次操作是"安全的"（读永远安全；插入时 size < max_size 不会分裂；删除时 size > min_size 不会合并），就释放所有祖先 latch。最坏情况是路径上每个节点都濒临分裂（插入时全满），分裂会逐层上传，此时必须把写 latch 从根攥到叶子。MiniOB 里对应 `IndexNodeHandler::is_safe` 与 `crabing_protocal_fetch_page`。

**Q5：MiniOB 的索引键为什么要拼上 RID？**

A：因为索引列允许重复值，而 B+ 树内部需要键唯一才能精确定位/删除一个索引项。拼上 RID（页号+槽位）后任意两项都不同，等值查询时相同字段值的项在叶子上连续排列，可一次收集所有 RID。对应 `KeyComparator`：先逐字段比较，平手再比 RID。InnoDB 二级索引的"索引键+主键值"是同一思想。

**Q6：B+ 树崩溃恢复怎么做？什么是 mini-transaction？**

A：靠 WAL：任何脏页刷盘前，描述修改的 redo 日志先落盘；崩溃后重放日志恢复。B+ 树的一次插入/删除会改多个页（分裂、改父节点、换根），必须保证这组修改原子生效，mini-transaction 就是把"一次物理结构修改"打包成原子单元的机制。MiniOB 的 `BplusTreeMiniTransaction` 组合了 `LatchMemo`（并发保护）和 `BplusTreeLogger`（redo 日志与回滚），思想源自 InnoDB 的 mtr。

## 回到赛题

- **赛题 8 multi-index**（解析见 [赛题 1–8](../02_problems_01_08.md)）：把单字段索引扩展为多字段复合索引。本章"键的设计"一节就是它的原理基础——`IndexMeta::fields()`/`fields_offset()` 如何拼接多列键、`KeyComparator::compare_key` 的字典序比较、最左前缀原则在优化器选索引时的作用，都是该题的直接考点。
- **赛题 9 unique**（解析见 [赛题 9–16](../03_problems_09_16.md)）：唯一索引。B+ 树本体的键永远唯一（字段值+RID），所以唯一约束只能在 `BplusTreeIndex::insert_entry` 外壳层检查：先查候选冲突再经 MVCC 可见性过滤，命中返回 `RC::RECORD_DUPLICATE_KEY`。读该题解析前先弄清本章"键 = 字段值 + RID"的设计会顺畅很多。
- **赛题 1 basic**（解析见 [赛题 1–8](../02_problems_01_08.md)）：作为入门题跑通建表、插入、查询全链路，其中 `CREATE INDEX` 与走索引的查询路径（`IndexScanner` → `BplusTreeIndexScanner` → `BplusTreeScanner`）正是本章内容的调用入口，可结合 [架构总览](../01_project_architecture.md) 的 SQL 执行链一起对照源码。
