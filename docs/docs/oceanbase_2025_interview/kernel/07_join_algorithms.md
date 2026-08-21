# 连接算法：Nested Loop、Sort-Merge 与 Hash Join

## 本章导读

在 SQL 的所有操作里，连接（JOIN）通常是最贵的一个。过滤、投影是"逐行过一遍"的线性工作，而连接要在两张表的行之间两两判断匹配，天然是做"乘法"而不是"加法"。面试里"讲讲你知道的连接算法"几乎是数据库方向的必问题，MiniOB 2025 赛题中也有两道题与它直接相关：第 5 题 join-tables 和第 24 题 big-order-by。

本章从"连接为什么贵"讲起，依次讲清四种经典算法：

- Nested Loop Join（嵌套循环连接），含块嵌套循环、索引嵌套循环两个变体；
- Sort-Merge Join（排序归并连接）；
- Hash Join（哈希连接）；
- Grace Hash Join（内存装不下时的外部哈希连接）。

然后讨论连接顺序（join order）为什么重要，最后落到 MiniOB 的真实源码：你会看到一个实现得非常干净的 Nested Loop Join，和一个"名字叫 Grace Hash Join、实际退化成落盘嵌套循环"的算子——这是本届赛题里最好的一个"算法名与实现不符"的工程案例。

阅读本章前，建议先过一遍 [../01_project_architecture.md](../01_project_architecture.md) 中"SQL 执行链"一节，只需记住一个事实：MiniOB 的物理算子遵循火山模型，上层通过 `open()`/`next()`/`close()` 逐行向下层拉取数据。

## 1. 连接为什么贵：笛卡尔积爆炸

### 1.1 生活类比：给两个班的同学配对

假设你是班主任，要从 A 班（30 人）里给 B 班（40 人）的每位同学找"同月同日生"的搭档。最笨的办法是让 A 班每个人都和 B 班每个人握一次手、互报生日——一共 30×40=1200 次问候，才能保证不漏掉任何一对。如果再加入 C 班（50 人）做三班匹配，问候次数变成 30×40×50=60000。

连接在逻辑上就是这个"两两比对"的过程。`SELECT * FROM A, B WHERE A.id = B.id` 可以读成：先形成 A×B 的全部组合（笛卡尔积），再按条件过滤。所有连接算法要解决的其实是同一个问题：**能不能不真的枚举全部组合，就把满足条件的组合找出来。**

### 1.2 跟着数字算一遍：20 的四次方

MiniOB 赛题 24（big-order-by）的测试数据是 4 张表，每张 20 行、每行 20 个 int 列（见仓库根目录的 `big_order_by_clean.sql`）。四表连接（无连接条件，即纯笛卡尔积）的中间结果是：

```text
20 × 20 × 20 × 20 = 20^4 = 160 000 行
```

每行 4×20=80 个 int，按 4 字节算是 320 字节，整个中间结果约 160000×320B ≈ 51 MB——而这还只是 20 行的"小表"。如果每张表 100 行：

```text
100^4 = 100 000 000 行 ≈ 1 亿行 × 320B ≈ 32 GB
```

一台普通开发机的内存都放不下。如果还要对这个中间结果做 `ORDER BY`，就得先把这几十 GB 排序——这正是 big-order-by 这道题 "big" 的真正来源：不是单表大，而是**连接的中间结果随表数指数膨胀**。

> 记忆要点：连接的代价 ≈ 中间结果的大小；中间结果的大小 ≈ 各输入规模的乘积 × 选择率。优化连接，本质上就是和"乘积爆炸"作斗争。

## 2. Nested Loop Join：最朴素的双重循环

### 2.1 基本算法

Nested Loop Join（NLJ，嵌套循环连接）就是上节"笨办法"的直接翻译：

```text
for each row l in LEFT:          -- 左表，也叫外表 / 驱动表
    for each row r in RIGHT:     -- 右表，也叫内表
        if match(l, r):          -- 连接条件，任意谓词都行
            emit (l, r)
```

它的优点怎么说都不为过：实现只要十几行；支持任意连接条件（等值、不等值，甚至没有条件的笛卡尔积）；边算边输出，不需要额外内存。它是所有数据库的"保底"算法——别的算法用不了时，永远可以退回到它。代价也写在脸上：tuple 比较次数是 |L|×|R|。

### 2.2 跟着算一遍

用两个小表演示 `S ⋈ SC ON S.id = SC.sid`：

S（学生表，3 行）：

| id | name |
|----|------|
| 1  | 张三 |
| 2  | 李四 |
| 3  | 王五 |

SC（选课表，4 行）：

| sid | course |
|-----|--------|
| 1   | 数学   |
| 1   | 英语   |
| 3   | 数学   |
| 4   | 数学   |

双重循环共做 3×4=12 次比较，其中 3 次匹配（S.id=1 碰上 SC 的前两行，S.id=3 碰上一行），输出 3 行。S.id=2 的李四没有选课记录、SC.sid=4 的学生不存在，两边都不输出——这正是 INNER JOIN 的语义：**不匹配的组合直接丢弃**。

### 2.3 真正贵的不是比较，是 I/O

12 次比较听起来不多，但数据库的账要按"页"算（MiniOB 的页是 128KB，见 `src/observer/storage/buffer/page.h:26` 的 `BP_PAGE_SIZE = (1 << 17)`）。设：

- 左表 R：10000 行，占 100 页；
- 右表 S：100000 行，占 1000 页。

朴素 NLJ 每拿到一行左表，就要把右表的全部页重新扫一遍：

```text
页读取次数 = b_R + |R| × b_S = 100 + 10000 × 1000 ≈ 10 000 100 次
```

即使数据全在内存里，10000×100000=10^9 次谓词比较也是秒级开销；如果每次内表扫描真的触发磁盘 I/O，就是灾难。

### 2.4 块嵌套循环（Block NLJ）：一次搬一块

改进思路很朴素：内存既然能放下 B 页，就一次读 B 页左表进来，右表每扫一遍，就和内存里这 B 页的所有行比较。右表被重扫的次数从 |R| 降到 ⌈b_R / B⌉：

```text
取 B = 10：页读取 = b_R + ⌈b_R / B⌉ × b_S = 100 + 10 × 1000 = 10 100 次
```

比朴素版少了三个数量级。口诀是：**给外表的缓冲越大，内表被重扫的次数越少**。极端情况是整个外表放进内存，内表只扫一遍，总代价 b_R + b_S——所以"内存够用时 NLJ 并不差"。MySQL 的 join buffer 做的就是这件事。

### 2.5 Index NLJ：内表有索引时是质变

如果内表在连接键上有索引（比如 B+ 树），内层循环就不必"扫描内表"了：每拿到一行外表，直接用 `l.key` 去索引里查。一次 B+ 树查找约 h+1 次页访问（h 为树高，3 层的树很常见）：

```text
页读取 ≈ b_R + |R| × (h+1) = 100 + 10000 × 4 ≈ 40 100 次
```

从"每行重扫 1000 页"变成"每行查 4 页"，这就是索引对连接的加速。OLTP 里大量的"订单表 JOIN 用户表 ON user_id"走的正是这条路：外表过滤后没几行，内表主键索引一查一个准。**NLJ 本身不慢，慢的是"无索引 + 大内表"的 NLJ。**

### 2.6 三种变体一页账

用上面的数字汇总（页读取次数）：

| 变体 | 公式 | 本例代价 |
|---|---|---|
| 朴素 NLJ | b_R + \|R\|·b_S | ≈ 10 000 100 |
| 块 NLJ（B=10） | b_R + ⌈b_R/B⌉·b_S | 10 100 |
| Index NLJ（h≈3） | b_R + \|R\|·(h+1) | ≈ 40 100 |

注意块 NLJ 与 Index NLJ 谁快取决于数据规模和索引选择率，没有绝对赢家——这正是优化器存在的意义。

## 3. Sort-Merge Join：先排序，再归并

### 3.1 生活类比：两摞按学号排好的卡片

如果 A、B 两个班的花名册都已按学号从小到大排好，找"同学号配对"就不用两两握手了：两摞卡片各取最上面一张，学号小的那摞翻页；学号相等就配成一对，翻完为止。每摞卡片从头到尾只翻一遍——这就是归并。

### 3.2 算法步骤

1. 若输入未按连接键排序：先分别排序（内存放不下就用外部排序，分批排序再归并）；
2. 双指针归并：键小的那侧前进；键相等时进入"重复段处理"，把两侧相同键的段两两组合输出。

### 3.3 跟着算一遍

设 A 的连接键序列是 [1, 3, 5, 7]，B 的是 [3, 5, 5, 8]：

```text
A: 1   3   5   7        B: 3   5   5   8
   ↑                    ↑        1 < 3，A 前进
       ↑                ↑        3 = 3，输出 (3,3)
           ↑                ↑    5 = 5 的重复段：A 段长 1、B 段长 2，
                                 输出 (5,5a)、(5,5b) 两个组合
               ↑                   ↑  7 < 8，A 前进；A 耗尽，结束
```

结果 3 行：(3,3)、(5,5a)、(5,5b)。每个输入只扫一遍，比较次数远小于 4×4=16。

### 3.4 代价与适用场景

- 未排序输入：排序代价 O(M log M + N log N)（外部版本外加磁盘 I/O），归并代价 O(M+N)；
- 已排序输入（连接键上有 B+ 树索引、或上游刚做完 ORDER BY）：省掉排序，只剩 O(M+N)；
- 适用：**等值连接**，且输出天然按连接键有序，可以"白送"给上游的 ORDER BY / GROUP BY——这种"顺带有用的顺序"在优化器里叫 interesting order，是选它的重要理由；某些实现也支持 >、< 一类的范围连接；
- 软肋：连接键大量重复时，重复段两两配对会膨胀；极端情况（两边键全相同）退化回 O(M×N)。

## 4. Hash Join：把"两两比对"变成"各扫一遍"

### 4.1 生活类比：先按月份分抽屉

回到 1.1 节的配对问题。聪明一点的办法：先把 A 班 30 人按出生月份放进 12 个抽屉，B 班每个人过来，只打开自己那个月份的抽屉比对。总工作量是 30+40=70 次"放 / 取"，而不是 1200 次握手。哈希表就是这个"抽屉柜"：用哈希函数把连接键映射到桶，**相同的键必然进同一个桶**。

### 4.2 算法两个阶段

- **build 阶段**：扫一遍 build 侧（通常选小表），按 `hash(key)` 建一张内存哈希表；
- **probe 阶段**：扫一遍 probe 侧，每行算 `hash(key)` 去哈希表里查找，命中就输出组合。

```text
LEFT (build)  --扫一遍--> 内存哈希表 {key -> 行}
RIGHT (probe) --扫一遍--> 每行查一次哈希表 --> 命中即输出
```

### 4.3 跟着算一遍

仍用 2.2 节的 S 和 SC。build 侧选较小的 S（3 行），建表 {1→张三, 2→李四, 3→王五}。probe 侧 SC 的 4 行依次来查：`hash(1)` 命中张三，输出 2 行；`hash(3)` 命中王五，输出 1 行；`hash(4)` 桶空，丢弃。

总操作次数 3（建表）+ 4（探测）= 7，而 NLJ 是 12 次比较。规模放大后差距是量级级的：build 1000 行、probe 5000 行，哈希连接约 6000 次哈希操作，NLJ 是 500 万次比较。

### 4.4 O(M+N) 的三个前提

哈希连接的"线性"不是白来的，它依赖：

1. **build 侧的哈希表装得进内存**——否则哈希表自身频繁换页，性能崩塌（下一节专门解决这个）；
2. **哈希函数足够均匀**——所有键挤进同一个桶，就退化成桶内嵌套循环；
3. **等值连接**——哈希只能回答"相等不相等"，回答不了"大于小于"。`ON A.x < B.y` 用不了哈希连接。

工程上还有一条铁律：**build 侧选小表**。哈希表大小正比于 build 侧行数，选小表内存压力最小。执行器为此需要行数估计；估错了会把大表建成哈希表，性能反转——这是生产环境"查询突然变慢"的常见原因之一。

## 5. Grace Hash Join：内存装不下怎么办

### 5.1 问题与出路

build 侧 10GB，内存只有 100MB，哈希表建不下。两条出路：换算法（sort-merge 的外部版本天生不怕大输入），或者**把大问题切成若干个小到能放进内存的哈希连接**。Grace Hash Join 走的是后者。它的名字来自上世纪 80 年代日本 GRACE 数据库机项目，后来成了"外部哈希连接"的代名词。

### 5.2 两阶段

```text
阶段一 Partition（分区）:
  LEFT  --逐行读--> h1(key) % R --> 追加写入左分区文件 L0 .. L(R-1)
  RIGHT --逐行读--> h1(key) % R --> 追加写入右分区文件 R0 .. R(R-1)
  （两侧必须用同一个哈希函数 h1）

阶段二 Join（逐分区连接）:
  for i in 0 .. R-1:
      把 Li 整个读进内存，建哈希表（build，可换另一个哈希函数 h2）
      流式扫描 Ri，逐行 probe，命中即输出
```

### 5.3 为什么这样切是对的

关键性质：**能配对的行，连接键必然相等；键相等，`h1(key)` 必然相等；所以它们必然落进同一个分区号。**反过来，不同分区的行绝不可能匹配，于是每个分区可以独立做哈希连接、互不干扰。分而治之的正确性，完全由"两侧用同一个哈希函数"这一个约定保证。

这也解释了为什么"随便对整行做哈希"是错的（MiniOB 的教训，第 7 节细讲）：左行与右行即使键相等，整行内容通常不同，整行哈希会把同键的行分到不同分区，结果直接漏行。

### 5.4 分区数怎么定、还是装不下怎么办

- 分区数 R 的目标：**让每个 build 分区都装得进内存**。设 build 侧占 b 页、可用内存 M 页，再乘一个哈希表膨胀系数 f（工程上常取 1.2~1.4），则 R ≈ ⌈b·f / M⌉；
- R 也不能无限大：分区阶段每个分区至少要占一个输出缓冲页，R 超过可用缓冲页时连分区都写不开；
- 若数据倾斜导致某个分区仍超内存：**递归分区**——只对这个分区换一个新的哈希种子，再做一轮同样的切分，直到装得下；
- 倾斜极端时（海量行同键）递归也无效，只能对该分区退化为块嵌套循环，或干脆换 sort-merge join。

### 5.5 I/O 账

不触发递归时，Grace Hash Join 的页 I/O 大约是 3(b_L + b_R)：分区阶段读一遍 + 写一遍，连接阶段再读一遍。对比块 NLJ"内表被重扫 ⌈b/B⌉ 遍"，输入越大，Grace 的优势越明显——它把"乘法"的 I/O 变成了"加法"。

## 6. 连接顺序：先和谁连，差出一个量级

### 6.1 中间结果大小决定一切

三表连接 A⋈B⋈C，结合顺序不同、最终结果相同，但中间结果天差地别：

```text
A = 10000 行，B = 100 行，C = 10000 行
A⋈B 选择率 1%（出 100 行），其结果与 C 的连接选择率也是 1%

顺序一 (A⋈B)⋈C：中间结果 100 行 → 再与 C 连，规模约 1 万行
顺序二 (A⋈C)⋈B：A 与 C 无直接条件 → 笛卡尔积 10^8 行 → 再与 B 连
```

同一个查询，中间结果差了 4 个数量级。优化器选连接顺序，就是在所有"结合树"里找中间结果总量最小的那棵。

### 6.2 左深树与驱动表

工程上最常用的是**左深树**（left-deep tree）：每层连接的右输入都是一张基表，左输入是上一层连接的中间结果：

```text
        ⋈
       /  \
      ⋈    t3
     /  \
    ⋈    t2
   /  \
  t1  （驱动表）
```

好处有二：中间结果不用物化，边产生边喂给上一层（流水线）；右输入固定是基表，方便走索引做 Index NLJ。此时最左端的表叫**驱动表**（driving table）——驱动表越小、过滤越狠，整条流水线越轻。System R（Selinger 论文）以来的经典做法是用动态规划枚举"{表集合 → 最优左深计划}"，把阶乘级的枚举压到 2^n 个子问题的量级；表特别多时（十几张以上）再用贪心或启发式兜底。

### 6.3 MiniOB 的现状

MiniOB 不做连接重排序。`LogicalPlanGenerator::create_single_select_plan`（`src/observer/sql/optimizer/logical_plan_generator.cpp`）按 FROM 子句的书写顺序，把第 1、2 张表先连，结果再与第 3 张连……构造出一棵形状固定的左深树。同时，`WHERE`/`ON` 条件整体包在 Join 上方的 Predicate 算子里，**Join 算子本身拿不到等值连接键**——请记住这一点，它是下一节故事的伏笔。

## 7. MiniOB 源码精读：一个干净的 NLJ，一个"名不副实"的 Grace Hash Join

### 7.1 算子在执行树中的位置

优化阶段由 `PhysicalPlanGenerator::create_plan(JoinLogicalOperator&)`（`src/observer/sql/optimizer/physical_plan_generator.cpp:389`）把逻辑 Join 转成物理算子。两个候选算子都是严格的二元算子（`children_.size() != 2` 直接报错），输出统一是 `JoinedTuple`（`src/observer/sql/expr/tuple.h:476`）——它持有左右两个 tuple 的指针，对外表现为"拼接后的一行"。ON/WHERE 条件不在 Join 算子内判断，而是由上方的 `PredicatePhysicalOperator`（`src/observer/sql/operator/predicate_physical_operator.cpp`）对拼接行过滤。

### 7.2 `NestedLoopJoinPhysicalOperator`：教科书式的双重循环

`src/observer/sql/operator/join_physical_operator.cpp` 的结构与 2.1 节的伪代码一一对应：

- `open()`（第 20 行）：只打开左孩子，右孩子懒到第一次用到才开；
- `next()`（第 45 行）：一个状态机——右表扫到 `RECORD_EOF` 说明本轮结束，左表前进一步，右表重开新一轮；
- 真正的"内层循环重扫"在 `right_next()`（第 147 行）：

```cpp
RC NestedLoopJoinPhysicalOperator::right_next()
{
  RC rc = RC::SUCCESS;
  if (round_done_) {              // 上一轮右表已扫完
    if (!right_closed_) {
      rc = right_->close();       // 先关掉右孩子
      right_closed_ = true;
      if (rc != RC::SUCCESS) {
        return rc;
      }
    }
    rc = right_->open(trx_);      // 为新的左行重新打开：右侧从头再扫一遍
    ...
    round_done_ = false;
  }
  rc = right_->next();            // 取右表下一行
```

逐行看：`round_done_` 为真表示上一行左表已经"用完"；此时先 `close()` 右孩子、再 `open()` 右孩子——**每处理一行左表，右子树就完整重开一次**，右孩子可以是任意复杂的子计划，只要支持反复 open。源码注释也特别强调"右孩子必须支持反复 close/open；阻塞算子 reopen 状态不完整会造成多轮 Join 丢行"。这段代码就是"NLJ 代价 = 左表行数 × 右子树单次代价"的直接证据。

`current_tuple()` 返回 `JoinedTuple`，并把左右两侧的 base rids 合并（`join_physical_operator.cpp:95`），为后续 UPDATE、全文检索等需要定位原始行的功能保留信息。

小结：MiniOB 的 NLJ 是**纯元组级的朴素 NLJ**——没有块缓冲、没有索引特判，右子树每轮完整重开。对赛题 5 的小数据量足够正确；放到大表上，就是赛题 24 的灾难现场。

### 7.3 优化器如何选择：按 JOIN 树深度"拍脑袋"

`physical_plan_generator.cpp:389` 的 `create_plan(JoinLogicalOperator&)` 里，选择逻辑全部如下（第 429-446 行）：

```cpp
  // 计算当前JOIN的深度（包括自己）
  int join_depth = calc_join_depth(&join_oper);

  // 只有在检测到深度JOIN（至少3层，即4表）时才考虑使用外部JOIN
  const bool is_multi_table_join = (join_depth >= 3);

  if (is_multi_table_join) {
    // 多表JOIN：使用Grace Hash Join处理大数据集
    const size_t JOIN_MEMORY_LIMIT = 15 * 1024 * 1024;  // 为每个join分配15MB内存
    const size_t NUM_PARTITIONS = 32;                    // 32个分区
    join_physical_oper.reset(new GraceHashJoinPhysicalOperator(JOIN_MEMORY_LIMIT, NUM_PARTITIONS));
  } else {
    // 单表JOIN或简单两表JOIN：使用传统的Nested Loop Join（更快、更稳定）
    join_physical_oper.reset(new NestedLoopJoinPhysicalOperator());
  }
```

`calc_join_depth`（第 409-427 行）递归统计 JOIN 节点的嵌套层数：4 张表的左深树深度为 3，于是触发 Grace Hash Join；两表、三表连接一律 NLJ。注意这个判据**不看表有多大、有没有等值键、选择率多少，只看树深**——这是为 big-order-by（4 表）专门开的口子，而不是基于代价的选择。

### 7.4 GraceHashJoin 的设计意图：骨架确实是两阶段

单看结构，`src/observer/sql/operator/grace_hash_join_physical_operator.h/.cpp` 与第 5 节的教科书算法一致：

- `open()`（cpp 第 36 行）：同时打开左右孩子，立即执行 `partition_phase()`，然后关闭两个孩子——分区阶段把整个输入物化到磁盘，之后不再需要孩子算子；
- `partition_input()`（cpp 第 107 行）：为每侧创建 `num_partitions_` 个临时文件（`TempFileManager::create_temp_file("partition")`，见 `src/observer/sql/operator/external_sort/temp_file_manager.h`），逐行 `flatten_tuple()` 成 `ValueListTuple`，按 `hash_tuple(tuple) % num_partitions_` 选文件，用 `TupleSerializer::serialize`（`src/observer/sql/operator/external_sort/tuple_serializer.cpp`）追加写入，最后给每个文件补上"行数"文件头；
- `join_partition(i)`（cpp 第 351 行）：把左分区 i 的全部 tuple 反序列化进内存，打开右分区 i 的文件流；
- `next()`（cpp 第 270 行）：逐分区产出组合；`close()` 负责清理临时文件（`temp_file_mgr_.cleanup_all()`）。

构造参数看起来也一应俱全：`memory_limit`（planner 传 15MB）、`num_partitions`（planner 传 32，头文件 `GraceHashJoinPhysicalOperator(size_t memory_limit, size_t num_partitions = 16)` 默认 16）。

### 7.5 实际退化：三处证据

**证据一：`hash_tuple` 恒返回 0。** cpp 第 207-220 行，源码注释写得非常坦诚：

```cpp
size_t GraceHashJoinPhysicalOperator::hash_tuple(const Tuple *tuple) const
{
  // 这里应基于等值连接 key 计算稳定哈希；若无法取得真实 join key 而固定落入
  // 同一分区，算法会退化为落盘 Nested Loop，失去 Grace Hash Join 的意义。
  // 当前的 JOIN 逻辑是通过额外的 Predicate 算子来完成条件过滤，
  // 这里拿不到真正的等值连接键。如果继续按照整行内容做 hash，
  // 左右分区的 hash 值通常不同，会导致左右数据落在不同分区，
  // 最终整个分区 join 过程无法产出结果，从而出现笛卡尔积缺失。
  //
  // 为保证正确性，将所有 tuple 固定映射到同一个分区，这样整个
  // 算子会退化成"分区粒度为 1"的外部嵌套循环：仍然能够利用磁盘
  // 临时文件限制内存占用，同时避免分区不一致导致结果缺失。
  return 0;
}
```

前 6 行注释承认根因：条件过滤由外层 Predicate 完成，算子**拿不到 join key**；中间解释为什么不能 hash 整行——同键的左右行内容不同，会落进不同分区导致漏结果（正是 5.3 节的正确性论证）；最后 `return 0`：所有 tuple 进 0 号分区，其余 31 个分区文件永远是空的。

**证据二：build 侧根本没有哈希表。** `join_partition()` 把左分区整个读进 `std::vector<Tuple *> build_tuples_`（h 文件第 107 行）。头文件里虽然定义了 `HashEntry` 结构、声明了 `join_phase()` 和 `probe_hash_table()` 两个方法，但这两个方法在 .cpp 里**从未被定义，也从未被调用**（可以用 `grep -n "probe_hash_table\|join_phase" grace_hash_join_physical_operator.cpp` 验证，无输出）——它们只是设计意图的遗迹。

**证据三：分区内的连接就是嵌套循环。** `next()` 的核心（cpp 第 331-341 行）：

```cpp
    // 遍历left表（build side）
    if (build_index_ < build_tuples_.size()) {
      current_left_tuple_ = build_tuples_[build_index_];
      build_index_++;

      // 构造joined tuple
      joined_tuple_.set_left(current_left_tuple_);
      joined_tuple_.set_right(current_right_tuple_);

      return RC::SUCCESS;
    }
```

每读一个右行，就把 `build_tuples_` 从头到尾遍历一遍，输出全部组合（不做任何键比较，过滤照样交给上层 Predicate），复杂度 O(|L|×|R|)。此外，成员 `memory_limit_` 在整个 .cpp 里除了构造时赋值从未被读取——15MB 的内存预算只是个摆设，build 分区超内存时既不会递归分区也不会 fallback。

### 7.6 一个值得讲给面试官的工程案例

把三件事连起来看：

1. **它保证了正确性。** `return 0` 不是偷懒的 bug，而是权衡：拿不到 key 时，hash 整行会漏结果（错），全进一个分区只是慢（对）。正确性优先于性能，这个取舍方向是对的；
2. **但它名不副实。** 类名叫 `GraceHashJoinPhysicalOperator`，实际执行的是"单分区外部嵌套循环 + 一次全量序列化落盘"。名字会误导读代码的人、误导日志和 explain 的输出、误导后来者在此之上做优化——"算法名与实现不符"是真实工程里很伤的一类问题；
3. **病根在架构分层。** 连接条件放在 Join 之上的 Predicate 里，Join 算子自然拿不到 key。真正的修法是：在逻辑计划阶段把 ON 中的等值条件（如 `t1.a = t2.a`）拆出来，作为 join key 显式传给 Join 算子；然后 `hash_tuple` 只对 key 求哈希、build 侧建真哈希表、并用 `memory_limit_` 决定是否递归分区。这也是工业数据库的 Join 算子全都"自带" join key 参数的原因。

一句话总结：**MiniOB 的 GraceHashJoin 是一次"正确性保底"的退化实现——两阶段的骨架搭好了，但因为算子拿不到 join key，`hash_tuple` 恒返回 0，实际跑的是落盘版嵌套循环。**

## 8. 工业数据库怎么做

- **MySQL（InnoDB）**：8.0.18 之前只有嵌套循环家族——朴素 NLJ、用 join buffer 缓冲外表的 Block NLJ、以及批量收集键再做主键预取的 BKA（Batched Key Access）；8.0.18 引入 Hash Join，用于无索引可用的等值连接；8.0.20 起 Hash Join 全面取代 Block NLJ。连接顺序由基于代价的优化器决定，`EXPLAIN` 的输出顺序就是驱动顺序。
- **PostgreSQL**：三种算法全部实现，优化器依据 `ANALYZE` 收集的统计信息（直方图、distinct 计数等）估算代价后选择。Hash Join 要求等值条件；build 侧超过 `work_mem` 时自动切成多个 batch 落盘（hybrid hash join 的思路）。Merge Join 在输入已有序、或上游需要有序输出时经常胜出。
- **OceanBase**：代价优化器在 NLJ / Merge Join / Hash Join 之间选择，并支持并行 Hash Join（多线程分桶建表 + 探测）和 partition-wise join（分区表按分区就地连接，避免数据搬运），执行计划可用 `EXPLAIN` 查看。

三者的共同点值得背下来：**都是"多算法候选 + 统计信息 + 代价模型"**。没有任何一种连接算法在所有场景下最优，数据库内核的价值恰恰在于替用户做好这道选择题。MiniOB 目前"按树深二选一"，相当于把这道选择题简化成了一道判断题。

## 小结

| 算法 | 适用条件 | 时间复杂度（tuple 级） | 主要额外代价 | MiniOB 对应实现 |
|---|---|---|---|---|
| 朴素 NLJ | 任意连接条件 | O(M×N) | 内表反复扫描 | `NestedLoopJoinPhysicalOperator`，主力 |
| 块 NLJ | 任意连接条件 | O(M×N)，I/O 降 ⌈b/B⌉ 倍 | 缓冲页 | 未实现 |
| Index NLJ | 内表连接键有索引 | O(M·log N) 量级 | 索引维护 | 未实现为独立算法 |
| Sort-Merge Join | 等值 / 输入已排序 | 排序 + O(M+N) | 排序（可能外排） | 未实现 |
| Hash Join | 等值 + build 侧装得进内存 | O(M+N) | 内存哈希表 | 名义上由 GraceHashJoin 承担 |
| Grace Hash Join | 等值 + build 侧超内存 | 分区 + 各分区近似 O(M_i+N_i) | 约 3(b_L+b_R) 页 I/O | 骨架在，但退化为落盘 NLJ |

三条主线收束全章：

1. 连接贵在中间结果可能爆炸，一切连接算法都在避免真的枚举 M×N；
2. 等值条件 + 索引 / 哈希是两大加速器；内存装不下时，排序和哈希各有"外部版本"；
3. 优化器的职责是用统计信息选算法、选顺序——MiniOB 目前是"NLJ 保底 + 深树换 Grace 骨架"，离基于代价的优化还很远。

## 面试追问

**问 1：Hash Join 为什么只能用于等值连接？非等值连接怎么办？**

答：哈希表只回答"key 相等的行在哪个桶"，回答不了范围关系。`A.x < B.y` 这类条件没有相等键可以哈希，只能回到嵌套循环（或其块版本）；某些范围条件可以用 Sort-Merge Join 的变体（归并时维护滑动窗口），但工业实现里非等值连接的默认答案就是 NLJ。

**问 2：Hash Join 的 build 侧为什么选小表？选反了会怎样？**

答：哈希表占用的内存正比于 build 侧行数，build 侧越小内存压力越小，也越不容易触发落盘分区。选反了：哈希表超内存→分区或换页，性能从 O(M+N) 退化；外部哈希场景还会加深递归分区层数。优化器靠统计信息估计行数来选边，统计过期就会选反——这是生产环境"查询突然变慢"的常见原因。

**问 3：Nested Loop Join 一定是最差的吗？什么时候它反而是最优解？**

答：不是。两个典型场景 NLJ 最优：（1）外表过滤后行数极少、内表连接键上有索引，即 Index NLJ——OLTP 的点查式连接几乎全是这种；（2）外表整个能放进内存，内表只需扫一遍。另外 NLJ 是唯一能处理任意连接条件的算法，是非等值连接的保底。

**问 4：Grace Hash Join 分区之后，某个分区还是装不下内存怎么办？**

答：递归分区——对超限的分区换一个新的哈希种子再切一轮，直到每个分区都装得进内存。如果是数据倾斜（大量行同键）导致递归无效，就需要 fallback：对该分区改用块嵌套循环，或整体改走外部 sort-merge join。工程上还要限制递归深度，并在分区阶段统计各分区大小以便提前发现倾斜。

**问 5：为什么不能"对整行做哈希"来决定分区？**

答：分区的正确性依赖"能匹配的行必须落进同号分区"，而匹配只意味着**连接键**相等，不意味着整行相等。左右两表列数、列值通常完全不同，整行哈希会把同键的行分到不同分区，join 结果直接缺失。MiniOB 的 `hash_tuple` 恒返回 0 正是为保正确性：宁可全进 0 号分区退化，也不漏行。正确做法是把等值连接键从 ON 条件中提取出来传给算子，只对键做哈希。

**问 6：多表连接的顺序为什么重要？优化器一般怎么选？**

答：顺序不改变最终结果，但改变中间结果的大小，而连接代价约等于中间结果大小。好的顺序让高选择率的连接先做，中间结果始终很小。经典做法是 Selinger 风格的动态规划：枚举表集合的最优左深计划，配合行数与选择率估计，把阶乘级搜索压到 2^n 个子问题；表特别多时退化为贪心或启发式。MiniOB 目前不重排，按 FROM 书写顺序构造左深树。

## 回到赛题

- **赛题 5 join-tables**（解析见 [../02_problems_01_08.md](../02_problems_01_08.md) 第 5 节）：本题要求实现 INNER JOIN。MiniOB 的执行主力就是本章 7.2 节的 `NestedLoopJoinPhysicalOperator`：左深树 + 每行左表重开右孩子，ON/WHERE 条件由 Join 上方的 Predicate 过滤。吃透 NLJ 的状态机（`next()` / `left_next()` / `right_next()`）和 `JoinedTuple` 的拼装方式，这题的执行语义就通了。
- **赛题 24 big-order-by**（解析见 [../04_problems_17_24.md](../04_problems_17_24.md) 第 24 节）：4 张 20 行的表做笛卡尔积再 ORDER BY——正是 1.2 节算过的 20^4=16 万行、80 列的中间结果。planner 检测到 JOIN 深度 ≥ 3 时启用 `GraceHashJoinPhysicalOperator`（15MB、32 分区），设计意图是本章第 5 节的外部哈希连接；实际因 7.5 节的三处证据退化为单分区落盘 NLJ。它真实的收益只剩"把中间结果物化到临时文件，配合外部排序控制内存峰值"，而不是哈希加速。
- 排序侧的知识（外部排序、多路归并）与本章第 3 节 Sort-Merge Join 的预处理同源，可对照**赛题 15 order-by**（[../03_problems_09_16.md](../03_problems_09_16.md) 第 15 节）一起复习：排序既是连接算法的输入准备，也是 ORDER BY 本身的实现手段。
