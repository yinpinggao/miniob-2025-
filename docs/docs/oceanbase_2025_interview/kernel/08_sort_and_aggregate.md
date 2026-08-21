# 排序与聚合：从内存排序到外部排序

## 本章导读

排序和聚合是 SQL 执行器里出现频率最高的两类"重计算"算子：`ORDER BY`、`GROUP BY`、`DISTINCT`、`UNION` 去重背后都是它们。在算法课上，排序是"给定一个数组，输出有序数组"；但在数据库里，问题变成了"给定一张比内存大得多的表，如何在内存受限的前提下输出有序结果"。这一个小变化，催生了数据库内核中最经典的一套工程方法——外部排序（External Sorting）。

本章假设你只学过数据结构的内排序，从零开始推导出外部排序的两阶段框架，用具体数字算清楚归并轮数和磁盘 I/O 次数，再回到 MiniOB 源码，看 `OrderByPhysicalOperator` 如何在内存排序和外部排序之间自适应切换、`ExternalSorter` 如何生成有序 run 并用最小堆归并、`HashGroupByPhysicalOperator` 为什么"名为 Hash 实为线性查找"。读完本章，你应当能在面试中徒手推导外排的 I/O 复杂度，并能讲清楚赛题 15（order-by）、10（group-by）、24（big-order-by）的实现链路。

## 排序：从整理扑克牌说起

### 生活类比

整理一副 54 张扑克牌：牌全在桌上，你可以随手交换任意两张——这是**内排序**，归并排序、快速排序都能用，时间 O(N log N)。

整理一座图书馆的一百万本书：书桌（内存）上一次只能摊开一千本，其余的书都在地下书库（磁盘）里。你没法"随手交换第 3 本和第 90 万本"，因为每去一次书库都要花几分钟（一次磁盘 I/O 比一次内存访问慢 4~5 个数量级）。可行的办法是：

1. 每次搬一千本到桌上，排好序，捆成一沓**有序的书堆（run）**，放回书库；
2. 然后同时翻开所有书堆的最上面一本，每次挑出全局最小的那本输出——这就是**归并**。

数据库的外部排序就是这个流程的严格化：**排序的代价不再由比较次数决定，而由读写磁盘的页数决定**。算法的优化目标从"减少比较"变成"减少 I/O 轮数"。

### 数据库场景与算法课的差异

| 维度 | 算法课内排序 | 数据库排序 |
|---|---|---|
| 数据规模 | 放得下内存 | 可能远大于内存 |
| 代价模型 | 比较次数 O(N log N) | 磁盘 I/O 页数 ≈ 2N×轮数 |
| 输入形式 | 数组 | 下层算子一行行吐出来的 tuple 流 |
| 输出时机 | 全部排完才有结果 | 阻塞算子：必须消费完输入才能输出第一行 |
| 额外要求 | 无 | 多键字典序、NULL 顺序、稳定性、内存预算 |

"阻塞算子（blocking operator）"这一点值得记住：在火山模型中，`ORDER BY` 的 `open()` 必须把子算子的所有行读完、排好，第一次 `next()` 才能返回结果。这和 `Filter` 这类流水线算子（来一行处理一行）有本质区别，也是优化器安排算子位置时的重要考量。

### 内排序回顾：为什么外排选择归并排序

算法课上的主流内排序里，快速排序平均最快，但它的分区步骤需要对数组做**随机访问**——支点左边一个元素、右边一个元素地交换。当数组放在磁盘上时，随机访问意味着随机 I/O，代价灾难。归并排序则不同：合并两个有序序列只需**从头到尾顺序扫描**，这与磁盘"顺序读写远快于随机读写"的物理特性天然契合。因此外部排序的骨架一定是归并排序：内存内的排序用什么算法（快排、堆排）不重要，跨 run 的合并一定是归并。

另一个会用到的内排序结构是**堆**：k 路归并要在 k 个候选中反复取最小值，大小为 k 的最小堆把每次选取的代价从 O(k) 降到 O(log k)。堆排序本身不稳定，外排要稳定性时需要在键里附加序号，MiniOB 的做法后文会看到。

## 外部排序精讲

### 两阶段框架

设数据共 N 页，内存缓冲区能容纳 B 页。外部归并排序（External Merge Sort）分两个阶段：

```text
Phase 0: 生成有序 run（run generation）
  重复：读入 B 页 → 内存排序 → 写出一个有序 run 文件
  结果：ceil(N/B) 个 run，每个长度 B 页（最后一个可能不满）

Phase 1..k: k 路归并（k-way merge）
  每次最多同时打开 B-1 个 run（留 1 页做输出缓冲），
  用最小堆/败者树不断取出各 run 当前最小记录，合并成更长的 run
  直到只剩 1 个 run，即排序完成
```

画成流程图（N = 1000 页、B = 100 页的例子）：

```text
原始数据 1000 页
   │  每次读 100 页 → 内存排序 → 写盘
   ▼
┌──────┬──────┬──────┬─────┬──────┐
│run 0 │run 1 │run 2 │ ... │run 9 │   10 个 run，各 100 页，内部有序
└──┬───┴──┬───┴──┬───┴─────┴──┬───┘
   │      │      │            │      归并扇入 99 ≥ 10，一轮搞定
   ▼      ▼      ▼            ▼
        最小堆（10 个节点，各 run 的头记录）
                │  反复弹出最小者输出、从同 run 补记录
                ▼
        有序输出 1000 页（写盘一遍）
```

### 数字演算一：跟着算一遍

设 N = 1000 页，B = 100 页。

- **Phase 0**：每次读 100 页排序后写出，得到 ceil(1000/100) = **10 个 run**。这一阶段把全部数据读一遍、写一遍，I/O = 2N = 2000 页。
- **Phase 1**：归并扇入（fan-in）= B - 1 = 99，即一次最多归并 99 个 run。现在只有 10 个 run，10 ≤ 99，**一轮归并就完工**。再读写一遍，I/O = 2N = 2000 页。

总 I/O = 2N × 2 轮 = 4000 页。注意：如果换成内存排序，比较次数约 N log N ≈ 1000 × 10 = 1 万次页内比较，代价远小于 I/O——外排的成本账本上，I/O 才是大头。

### 归并轮数与 I/O 复杂度

一般公式：

```text
run 数 R = ceil(N / B)
归并轮数 = ceil(log_{B-1}(R))          （每轮把 run 数缩小为原来的 1/(B-1)）
总轮数   = 1（生成 run）+ 归并轮数
总 I/O   ≈ 2N × 总轮数                  （每轮读写各一遍）
```

多算几组感受 B 的影响（N = 1,000,000 页）：

| N（页） | B（页） | run 数 R | 归并轮数 ceil(log_{B-1} R) | 总轮数 | 总 I/O（页） |
|---:|---:|---:|---:|---:|---:|
| 1,000,000 | 100 | 10,000 | 2（99² = 9801 ≈ 10000，实际需 2 轮多一点，向上取整为 3 轮的场景见下注） | 3~4 | 6N~8N |
| 1,000,000 | 1000 | 1000 | 2（999¹ < 1000 ≤ 999²） | 3 | 6N = 6,000,000 |
| 1,000 | 100 | 10 | 1 | 2 | 4N = 4,000 |

注：第一行精确计算 log_99(10000) ≈ 2.0009，向上取整为 3，总 4 轮 8N；这正说明缓冲区略大一点（比如能让 R ≤ B-1）就能省掉整轮 I/O。**面试中记住量级结论即可：总 I/O ≈ 2N × 轮数，轮数对数级增长，实践中 2~3 轮足够**。

时间上，生成 run 阶段的内排序总计 O(N log B)，归并阶段每轮用大小为 k 的堆做 O(N log k)，总比较次数仍是 O(N log N) 量级——外排并没有改变时间复杂度的阶，改变的是常数因子从"内存操作"变成了"磁盘 I/O"。

### 最小堆与败者树

k 路归并每次要从 k 个 run 的"当前头记录"里挑最小者。两种经典实现：

- **最小堆（min-heap）**：堆中每个节点附带"来自哪个 run"的标记。弹出堆顶输出后，从同一个 run 补读下一条压入堆。每次操作 O(log k)。MiniOB 用的就是 `std::priority_queue` 实现的最小堆。
- **败者树（loser tree）**：完全二叉树，内部节点记录两两比较的"败者"，冠军一路向上。每次输出冠军后，只需让对应 run 的下一条沿冠军路径重新比较，关键字比较次数更少、更规则，缓存行为更好，因此 PostgreSQL 等工业实现偏好败者树。面试能画出败者树并说出"内部节点存败者、冠军上升"即可。

### 赛题 24 的数字代入

赛题 24（big-order-by）的题面数据：4 张表，每表约 20 行、20 个字段，笛卡尔积 20⁴ = 160,000 行，题面预估中间/最终结果约 51.2 MB，内存限制 350 MB。

代入 MiniOB 的实现参数（下文源码一节会逐一对应）：外排缓冲区 5 MB，且每个 run 最多 1000 条 tuple，因此约产生 160,000 / 1000 = **160 个 run**。MiniOB 的归并阶段一次性打开所有 run 的 reader，用 160 个节点的最小堆做"一轮"归并——没有分批归并，也就回避了多轮 I/O，但代价是同时打开 160 个文件描述符、160 个 reader 对象同时在内存。这就是工程上的取舍，面试中被追问"run 太多怎么办"时，标准答案是**限制扇入、分多轮归并**。

## 排序在数据库里的其他用途

排序不只是为 `ORDER BY` 服务，它是执行器的"通用工具"：

- **ORDER BY**：最直接的用途，多键按字典序比较。
- **GROUP BY（Sort Group By）**：按 group key 排序后，同组的行物理相邻，扫描一遍即可分组聚合，下文详述。
- **DISTINCT**：`SELECT DISTINCT a, b` 等价于按 (a, b) 分组；排序后去重只需比较相邻行。
- **UNION 去重**：`UNION`（不带 ALL）要求结果无重复行，可以排序去重，也可以哈希去重（MiniOB 走的是哈希路径，见 [赛题 14 解析](../03_problems_09_16.md)）。
- **其他**：sort-merge join 要求两侧先按 join key 排序；B+ 树索引批量构建（bulk loading）也是先排序再自底向上建页。

一个贯穿性的思想是：**"有序"是一种可以被下游复用的物理属性**。如果数据已经按 (a, b) 排好序，那么 `GROUP BY a, b`、`DISTINCT a, b`、`ORDER BY a, b` 都可以免排序直接流式完成——优化器选择算子实现时会考虑这种"interesting order"。

## 聚合执行

### 标量聚合与分组聚合

```sql
-- 标量聚合（scalar aggregation）：全表聚成一行
SELECT COUNT(*), AVG(score) FROM exam;

-- 分组聚合（grouped aggregation）：每组聚成一行
SELECT dept, COUNT(*), AVG(score) FROM exam GROUP BY dept;
```

两者共用同一个核心机制：**聚合状态（aggregation state）的累积与定值**。每个聚合函数不保存全部输入，只维护一个固定大小的中间状态，扫到一行就更新一次（`accumulate`），输入耗尽后算一次最终结果（`evaluate`/`finalize`）：

| 聚合函数 | 中间状态 | accumulate | evaluate |
|---|---|---|---|
| COUNT | count | count++ | 返回 count |
| SUM | sum | sum += v | 返回 sum |
| AVG | **sum + count** | sum += v; count++ | 返回 sum / count |
| MIN/MAX | 当前最小/大值 | 比较并替换 | 返回该值 |

**AVG 为什么要存 sum 和 count 两个值？** 因为平均不满足"可增量合并"——知道 A 组的平均值和 B 组的平均值，算不出合并后的平均值；但知道两组各自的 (sum, count)，合并就是 (sum₁+sum₂, count₁+count₂)。这个性质（可交换、可结合的状态合并）也是分布式/并行聚合能成立的基础：各分区先算部分聚合状态，汇总节点再合并。

### Hash Group By

分组聚合的主流实现是哈希法：维护一张哈希表，key 是 group key 的值向量，value 是该组的聚合状态集合。每来一行：

```text
1. 计算 group key，如 (dept)
2. 在哈希表中查找该 key
   - 找到：用当前行更新对应聚合状态
   - 没找到：插入新 key，并初始化一组聚合状态
3. 输入耗尽后，对每组做 evaluate，逐组输出
```

用一个具体例子跟着走一遍。执行 `SELECT dept, COUNT(*), AVG(score) FROM exam GROUP BY dept`，输入五行：

```text
(CS, 80) (EE, 70) (CS, 90) (EE, NULL) (CS, 60)
```

哈希表状态逐步演化（AVG 的状态记为 sum+cnt）：

| 读到行 | 动作 | 哈希表内容（key → count, sum, cnt） |
|---|---|---|
| (CS, 80) | 未命中，新建组 | CS → 1, 80, 1 |
| (EE, 70) | 未命中，新建组 | CS → 1, 80, 1；EE → 1, 70, 1 |
| (CS, 90) | 命中 CS，更新 | CS → 2, 170, 2；EE 不变 |
| (EE, NULL) | 命中 EE，聚合忽略 NULL | CS 不变；EE → 2, 70, 1 |
| (CS, 60) | 命中 CS，更新 | CS → 3, 230, 3；EE 不变 |

finalize 后输出：CS → 3, 76.67；EE → 2, 70。注意第四行：`COUNT(*)` 计所有行，所以 EE 的 count 变 2；而 `AVG(score)` 忽略 NULL，其内部计数 cnt 仍是 1——这就是为什么 AVG 必须自己维护一份 count，不能复用 `COUNT(*)` 的结果。

内存占用 O(G × (key + 状态）)，G 是组数；时间理想 O(N)。当 G 大到哈希表放不下内存时，工业系统会把哈希表分区落盘（spill），逐分区聚合。

### Sort Group By 与对比

排序法：先按 group key 排序（数据大就走上一节的外部排序），然后顺序扫描——同组的行相邻，每当 key 变化就切组、输出上一组的聚合结果。仍用上面的五行输入，排序后变成：

```text
(CS, 80) (CS, 90) (CS, 60) | (EE, 70) (EE, NULL)
         ↑ 扫描到 key 从 CS 变为 EE 时，切组并输出 CS 组的聚合结果
```

扫描过程只需要 O(1) 的额外状态（当前组的聚合器），不需要哈希表——这是 Sort Group By 内存可控的根本原因。

| 维度 | Hash Group By | Sort Group By |
|---|---|---|
| 时间复杂度 | 平均 O(N) | O(N log N)（含排序） |
| 内存 | O(G)，G 大时需 spill | 排序缓冲区可控（外排） |
| 输出顺序 | 无保证 | 按 group key 有序 |
| 与 ORDER BY 协同 | 无 | 下游 `ORDER BY group key` 可免排序 |
| 流式性 | 阻塞（建完表才能输出） | 阻塞在排序，聚合阶段流式 |

工业优化器会按代价选择：组数少、无需有序输出时偏 Hash；输入已有序或下游需要有序时偏 Sort。

### 聚合下推思想

聚合本身不能"下推"过任意算子，但有一类重要优化叫**聚合下推/预聚合**：如果 `GROUP BY a` 之后还要和别的表按 a 连接，可以先在基表上按 (a, join 相关列） 做局部聚合减小数据量，再做连接和最终聚合；在分布式数据库里，则是每个分区节点先算部分聚合状态（partial aggregate），汇总节点合并（final aggregate）——这正是"AVG 存 sum+count"的直接应用。面试点到这一层，说明你知道聚合状态的可合并性不只是理论性质。

### HAVING 与 WHERE 的区别

SQL 的逻辑执行顺序（也是 MiniOB 逻辑计划的组装顺序）：

```text
FROM/JOIN → WHERE → GROUP BY + 聚合 → HAVING → SELECT → ORDER BY → LIMIT
```

- **WHERE 过滤的是"行"**：在分组聚合之前执行，此时还不存在"组"和聚合结果，所以 WHERE 里**不能出现聚合函数**。
- **HAVING 过滤的是"组"**：在聚合之后执行，每个组只剩一行，因此 HAVING 里可以引用聚合函数和 group key。

```sql
SELECT dept, AVG(score)
FROM exam
WHERE score >= 0          -- 先扔掉缺考标记为负分的行（行级过滤）
GROUP BY dept
HAVING AVG(score) > 60;   -- 再扔掉平均分不及格的系（组级过滤）
```

跟着算一遍：exam 表有 (CS, 80)、(CS, 40)、(EE, 55) 三行。WHERE 不过滤任何行；GROUP BY 后 CS 组 AVG = 60、EE 组 AVG = 55；HAVING `> 60` 判定 CS 组 60 > 60 为假，最终只剩零行——把条件改成 `>= 60` 才输出 CS 组。这个例子也提示：HAVING 里的聚合表达式会基于**同一批组内行**独立求值。

## MiniOB 源码实现

### OrderByPhysicalOperator：内存/外部排序自适应切换

入口在 `src/observer/sql/operator/order_by_physical_operator.cpp`。`open()` 的决策流程：

1. `estimate_input_rows()` 递归估算输入行数：表扫描固定估 1000 行、索引扫描 500 行；JOIN 取两侧乘积（封顶 200000），且 join 深度 ≥ 3 时抬到至少 150000 行；
2. `estimate_tuple_size()` 按 `96 + 32 × 列数` 估算单行字节数，join 每深一层加 128；
3. `get_sort_memory_threshold()` 返回固定阈值 **50 MB**；
4. 估算内存 = 行数 × 单行大小，超过阈值则走外部排序；此外还有启发式兜底：列数 ≥ 40 且 join 深度 ≥ 2、或行数 ≥ 80000 且列数 ≥ 32 时也强制外排。

对赛题 24 的四表 80 列场景，可以把决策数字具体算一遍：四表笛卡尔积是左深 join 树，join 深度为 3，因此行数估计被抬到 150000；单行大小估计为 96 + 32 × 80 + 3 × 128 = 3040 字节；估算内存 ≈ 150000 × 3040 ≈ 435 MB，远超 50 MB 阈值，`prefer_external` 直接成立；即便估算失灵，列数 80 ≥ 40 且 join 深度 3 ≥ 2 的启发式也会兜底判定走外排。两条独立的判定路径保证这种大查询稳定进入外排路径。

内存排序路径 `fetch_and_sort_tables()` 的核心片段（`order_by_physical_operator.cpp` 第 70-85 行）：

```cpp
std::stable_sort(sorted_entries_.begin(), sorted_entries_.end(), [this](const OrderEntry &a, const OrderEntry &b) {
  for (size_t i = 0; i < order_by_.size(); i++) {
    int cmp = a.keys[i].compare(b.keys[i]);          // 逐排序键字典序比较
    if (cmp != 0) {
      return order_by_[i].is_asc ? (cmp < 0) : (cmp > 0);
    }
  }
  int row_cmp = 0;
  RC  rc      = a.tuple->compare(*b.tuple, row_cmp); // 键全同再比整行
  if (rc == RC::SUCCESS && row_cmp != 0) {
    return row_cmp < 0;
  }
  return a.sequence < b.sequence;                    // 最后用输入序号兜底
});
```

每个 `OrderEntry` 保存预计算的排序键 `keys`、物化的整行 `tuple`（`ValueListTuple`）和全局递增的 `sequence`。比较器先按键字典序、再按整行、最后按输入序号——注意这意味着同 key 不同行的相对顺序由整行内容决定，**不是教科书意义的稳定排序**（尽管函数名叫 `stable_sort`），这是当前实现的一个边界，面试被问稳定性时要能指出来。

### external_sort 目录：ExternalSorter / TempFileManager / TupleSerializer

外部排序实现位于 `src/observer/sql/operator/external_sort/`，三个组件分工清晰：

- `ExternalSorter`（`external_sorter.h/.cpp`）：两阶段算法本体；
- `TempFileManager`（`temp_file_manager.h/.cpp`）：临时文件生命周期管理；
- `TupleSerializer`（`tuple_serializer.h/.cpp`）：tuple 的二进制序列化/反序列化。

**Phase 1 生成 run**：`generate_runs()` 循环从子算子 `next()` 取行，`flatten_tuple()` 把行深拷贝成 `ValueListTuple`（并预计算排序键缓存进 `order_keys`，归并时就不必重复求值表达式），攒进 `buffer_`。触发刷盘的阈值有两个（`external_sorter.cpp` 第 91-93 行）：

```cpp
const size_t MAX_TUPLES_PER_RUN = 1000;  // 限制每个run最多1000个tuple，降低内存峰值
bool should_flush = (current_memory_ + tuple_size > memory_limit_ && !buffer_.empty()) ||
                    (buffer_.size() >= MAX_TUPLES_PER_RUN);
```

`memory_limit_` 由 `OrderByPhysicalOperator::get_available_memory()` 给出，固定 **5 MB**。`flush_buffer_to_run()` 先用 `std::sort` 把缓冲区排成升序，再经 `TupleSerializer::serialize` 逐条写入新 run 文件，文件开头写入 tuple 条数。

**Phase 2 多路归并**：`merge_runs()` 为每个 run 文件创建一个 `RunReader`，读出各 run 的第一条压入最小堆；此后每次 `next()` 弹出堆顶输出，并从同一 reader 补读一条压回（`external_sorter.cpp` 第 213-224 行）：

```cpp
HeapNode node = merge_heap_->top();        // 堆顶即全局最小
merge_heap_->pop();
tuple = node.tuple;
Tuple *next_tuple = nullptr;
RC     rc         = readers_[node.reader_index]->next(next_tuple);
if (rc == RC::SUCCESS && next_tuple != nullptr) {
  merge_heap_->push(HeapNode(next_tuple, node.reader_index));  // 从同一个 run 补充
}
```

堆节点 `HeapNode{tuple, reader_index}` 中的 `reader_index` 就是"k 路归并必须记住元素来自哪一路"的具体实现。比较器 `compare_tuples()` 优先使用缓存的 `order_keys`，键全同再比整行，最后用指针地址打破平局——用指针地址意味着外排结果的同 key 顺序完全不确定，比内存路径更不稳定。

**临时文件管理**：`TempFileManager::create_temp_file()` 生成 `/tmp/<prefix>_pid<进程号>_<计数器>.tmp` 并登记在 `temp_files_` 中；`cleanup_all()` 逐个 `std::remove`，析构函数自动调用——这是 RAII 清理模式，保证算子销毁时临时文件不泄漏。注意文件名不含随机成分、创建非原子，并发场景下有理论上的冲突风险，教学实现可以接受。

**序列化**：`TupleSerializer` 提供 `serialize / deserialize / estimate_size`，逐 cell 写入类型与值；`estimate_tuple_memory()` 用 `estimate_size(tuple) + sizeof(Tuple *)` 估算内存占用，作为刷盘阈值的依据。当前不支持 `TEXTS` 类型，外排遇到 TEXT 会失败——这是赛题 17 与 24 交叉处的一个已知边界。

### ScalarGroupBy 与 HashGroupBy

聚合算子的基类 `GroupByPhysicalOperator`（`src/observer/sql/operator/group_by_physical_operator.h`）定义了关键类型：

```cpp
using AggregatorList = std::vector<std::unique_ptr<Aggregator>>;
using GroupValueType = std::tuple<AggregatorList, CompositeTuple>;
```

每个组保存两样东西：一组聚合器（`AggregatorList`）和一个 `CompositeTuple`（缓存该组第一条原始 tuple 加最终结果——缓存首行是为了支持 `SELECT a, b, SUM(a) ... GROUP BY a` 中读取未分组列 `b` 这种"不标准但常见"的写法）。

**ScalarGroupBy**（`scalar_group_by_physical_operator.cpp`）处理无 GROUP BY 的标量聚合：所有输入属于一个隐式组，`open()` 中逐行 `aggregate()`，输入耗尽后 `evaluate()` 一次。它还专门处理了**空输入**：一行都没有时仍要输出一行，COUNT 为 0，SUM/AVG/MIN/MAX 为 NULL（`next()` 中构造默认值的逻辑，第 121-125 行）——这是 SQL 标准语义，也是赛题 10 的测试点。

**HashGroupBy**（`hash_group_by_physical_operator.cpp`）处理有 GROUP BY 的情况。名字叫 Hash，但看 `find_group()` 的实现（第 152-164 行）：

```cpp
for (GroupType &group : groups_) {              // groups_ 是 std::vector<GroupType>
  int compare_result = 0;
  rc = group_by_evaluated_tuple.compare(get<0>(group), compare_result);
  if (OB_FAIL(rc)) { ... }
  if (compare_result == 0) {                    // 逐组线性比较 group key
    found_group = &group;
    break;
  }
}
```

它把所有组放在 `std::vector` 里，每来一行就**顺序扫描所有已存在的组**做 `ValueListTuple::compare`。设输入 N 行、G 个组、group key 有 K 列，找组总代价 O(N × G × K)，最坏（每行一个新组）退化到 O(N² × K)。换成真正的哈希表（O(1) 查找）即可降到平均 O(N × K)——这是面试中"你如何改进 MiniOB 的 group by"的标准答案，改动点也很明确：给 `ValueListTuple` 写 hash 函数，把 `groups_` 换成 `unordered_map`。

代码里其实已有真正的哈希表骨架，但服务于向量化执行路径：`src/observer/sql/expr/aggregate_hash_table.h` 定义了抽象基类 `AggregateHashTable`（接口是批量式的 `add_chunk(Chunk&, Chunk&)` 和 `Scanner`），其派生类 `StandardAggregateHashTable` 用 `std::unordered_map<std::vector<Value>, std::vector<Value>, VectorHash, VectorEqual>` 实现 "group key 向量 → 聚合值向量" 的映射；`#ifdef USE_SIMD` 下还有参考论文 *Rethinking SIMD Vectorization for In-Memory Databases* Algorithm 5 的 `LinearProbingAggregateHashTable`（线性探测哈希表）。需要注意 `StandardAggregateHashTable::add_chunk` 当前仍是 `exit(-1)` 占位（`aggregate_hash_table.cpp` 第 15-18 行），即向量化聚合链路留作练习；火山模型路径实际使用的是上面线性查找的 HashGroupBy。

### aggregator.h：聚合状态的实现

`src/observer/sql/expr/aggregator.h` 中 `Aggregator` 基类只有两个虚函数，正好对应前面讲的 accumulate / evaluate 两阶段：

```cpp
class Aggregator {
public:
  virtual RC accumulate(const Value &value) = 0;   // 扫到一行，更新中间状态
  virtual RC evaluate(Value &result)        = 0;   // 输入耗尽，算最终结果
protected:
  Value value_ = Value(NullValue());
};
```

`AvgAggregator` 是"AVG 存 sum + count"的直接体现：`value_` 累加存 sum，另有成员 `count_` 计行数，`evaluate()` 里做除法。所有聚合器的 `accumulate` 开头都跳过 `value.is_null()` 的输入，对应 SQL 语义"聚合函数忽略 NULL"。两个可指出的简化：AVG 的 `evaluate` 通过 `get_float()` 转 float 做除法，定点精度有损失；SUM 用 `Value::add` 累加，没有溢出检查。

### MemTracer 350MB 限制下的内存预算意识

赛题 24 明确内存上限 350 MB，由 MemTracer（`deps/memtracer/`，通过 `LD_PRELOAD` hook `malloc/free` 统计进程内存）强制执行，超限直接 `exit(-1)`。这要求开发者有"内存预算"意识，把 350 MB 当成要分配的账本：

```text
350 MB 总预算
├── Buffer Pool、WAL、事务等存储层开销
├── JOIN 中间结果（四表笛卡尔积 160,000 行 × 80 列，才是最危险的部分）
├── ORDER BY：外排 buffer 5 MB + 160 个 RunReader + 归并堆（160 节点）
└── 结果集物化、表达式临时对象等
```

MiniOB 的应对体现为几个具体常量：内存/外排切换阈值 50 MB（`get_sort_memory_threshold()`）、外排缓冲区 5 MB（`get_available_memory()`）、单 run 1000 条上限（`MAX_TUPLES_PER_RUN`）。设计意图是"宁可多产生 run 文件，也要压低内存峰值"——用磁盘 I/O 换内存安全。工程上的遗留风险也要心里有数：切换依据是**估算**而非实际内存计数（单表扫描固定估 1000 行，真实大表可能误走内存排序）；归并时所有 run reader 同时打开，run 数极多时文件描述符和 reader 对象本身也占内存。更稳妥的做法是动态统计 `memtracer::allocated_memory()`，达到预算就 flush run，归并限制扇入分多轮进行。

## 复杂度分析汇总

| 操作 | 时间 | 空间/内存 | 磁盘 I/O |
|---|---|---|---|
| 内存排序（`fetch_and_sort_tables`） | O(N log N) | O(N × 行长） | 无 |
| 外排生成 run | O(N log B) 比较 | B 页缓冲区 | 读写各一遍，2N 页 |
| 外排 k 路归并 | 每轮 O(N log k) | k 个 reader + k 节点堆 | 每轮 2N 页，共 ceil(log_{B-1}(N/B)) 轮 |
| Hash Group By（理想） | O(N × K) | O(G × (key+状态）) | G 超内存时 spill |
| MiniOB HashGroupBy（线性查找） | O(N × G × K) | O(G) | 无 |
| Sort Group By | O(N log N) | 排序缓冲区 | 同外排 |

## 工业数据库怎么做

- **MySQL InnoDB**：`ORDER BY` 走 filesort（`sql/filesort.cc`）。排序数据能放进 `sort_buffer_size`（默认 256 KB 量级，会话可调）就内存排序，否则分成若干块排序后写临时文件再做归并。MySQL 对 `ORDER BY ... LIMIT n` 有专门的优先队列优化：只维护大小为 n 的堆，避免全量排序。能走索引有序扫描时则完全跳过 filesort（`EXPLAIN` 中不出现 `Using filesort`）。
- **PostgreSQL**：`src/backend/utils/sort/tuplesort.c`。内存阈值由 `work_mem` 控制。生成 run 时用**置换选择（replacement selection）**：配合优先队列，平均能产出长度约 2 倍内存的 run，直接减少 run 数和归并轮数；归并用败者树，多路归并策略上是 polyphase merge 的变体。聚合侧提供 HashAgg 与 GroupAgg（基于排序）两种节点，由优化器按代价选择，哈希表超过 `work_mem` 时分批 spill 到磁盘。
- **OceanBase**：分布式 MPP 架构下，排序和聚合天然拆成两层：各分区节点先做局部排序/部分聚合（partial），汇总节点做全局归并/最终聚合（final）——这正是"AVG 存 sum+count 可合并"性质的工程落地。单机层面同样有内存配额管理（按租户），算子内存超配额时落盘。

对照 MiniOB 可以看到清晰的演进线：MiniOB 的"固定 5 MB buffer + 每 run 1000 条"相当于工业实现的极简版；置换选择、败者树、LIMIT 堆优化、哈希 spill、两阶段分布式聚合，都是在这套骨架上生长的优化。

## 小结

- 数据库排序的代价模型是磁盘 I/O：外排总 I/O ≈ 2N × 轮数，轮数 = 1 + ceil(log_{B-1}(ceil(N/B)))，实践中 2~3 轮。
- 外排两阶段：内存排序生成有序 run → 最小堆/败者树 k 路归并。MiniOB 用 `std::priority_queue` 最小堆，一次性打开全部 run。
- 排序是通用工具：ORDER BY、GROUP BY、DISTINCT、UNION 去重、sort-merge join、索引构建都能复用"有序"这一物理属性。
- 聚合 = 固定大小中间状态的 accumulate + evaluate；AVG 必须存 sum+count；分组聚合有 Hash 与 Sort 两种实现，按组数、有序性需求选择。
- WHERE 过滤行（聚合前，不能用聚合函数），HAVING 过滤组（聚合后）。
- MiniOB 实现要点：`OrderByPhysicalOperator` 按 50 MB 阈值 + 启发式在内存/外排间切换；`external_sort/` 目录的 `ExternalSorter`/`TempFileManager`/`TupleSerializer` 组成外排链路；`HashGroupByPhysicalOperator` 名为哈希实为 O(N×G) 线性查找；`aggregate_hash_table.h` 的向量化哈希表骨架尚留 `exit(-1)` 占位。
- 350 MB MemTracer 限制是赛题 24 的硬约束：5 MB 外排 buffer、每 run 1000 条、50 MB 切换阈值，都是用 I/O 换内存安全的预算决策。

## 面试追问

**问：外部排序的 I/O 复杂度为什么是 2N × 轮数？轮数怎么算？**

答：每一轮（包括生成 run 和每一轮归并）都要把全部数据读一遍、写一遍，即 2N 页 I/O。生成 run 产生 ceil(N/B) 个 run，每轮归并最多把 run 数缩为 1/(B-1)，所以归并轮数是 ceil(log_{B-1}(ceil(N/B)))，总轮数再加 1。举例：N = 1000 页、B = 100 页时 10 个 run 一轮归并完成，共 2 轮 4N；N = 10⁶ 页、B = 1000 时 1000 个 run 需 2 轮归并，共 3 轮 6N。

**问：归并时最小堆和败者树有什么区别？为什么工业实现常用败者树？**

答：最小堆每次 pop/push 是 O(log k)，实现简单，MiniOB 用 `std::priority_queue` 就够了。败者树让冠军沿树上升、内部节点只记录败者，新元素只需与冠军路径上的节点比较，比较次数更少且访存模式更规则，对长关键字和 CPU 缓存更友好，所以 PostgreSQL 的 tuplesort 用败者树。两者渐进复杂度相同，差别在常数。

**问：AVG 的中间状态为什么要存 sum 和 count 两个值，不能只存一个"当前平均值"吗？**

答：不能。已知两组各自的平均值 avg₁、avg₂，无法推出合并后的平均值——平均值不满足可结合性；但 (sum₁+sum₂)/(count₁+count₂) 可以。存 (sum, count) 使聚合状态可增量更新、可跨组合并，这是并行/分布式两阶段聚合（partial + final）的基础。MiniOB 的 `AvgAggregator` 正是用 `value_` 存累加和、`count_` 存行数，evaluate 时做除法。

**问：Hash Group By 和 Sort Group By 各适合什么场景？**

答：Hash 平均 O(N)，组数不太大、不需要有序输出时首选；但组数超内存要 spill。Sort 要先排序（O(N log N) 或外排），内存占用可控，且输出天然按 group key 有序——如果下游有 `ORDER BY group key` 或输入已有序，Sort 更划算。优化器按代价二选一，PostgreSQL 的 HashAgg/GroupAgg 就是这样。

**问：HAVING 和 WHERE 的本质区别是什么？为什么 WHERE 里不能写聚合函数？**

答：执行顺序不同：WHERE 在分组聚合之前过滤行，HAVING 在聚合之后过滤组。WHERE 执行时"组"还不存在，聚合值自然无从谈起，所以标准禁止 WHERE 引用聚合函数；HAVING 阶段每组只剩一行，可以引用聚合函数和 group key。能提前用 WHERE 过滤的条件不要放进 HAVING——先减行再分组，聚合量更小。

**问：MiniOB 的 HashGroupBy 其实没有哈希表，你怎么看？怎么改？**

答：`find_group` 是对 `groups_` 这个 vector 的线性扫描，逐组比较 group key，复杂度 O(N×G×K)，组多时退化到 O(N²)。改进是把 `groups_` 换成以 group key 向量为键的哈希表：为 `ValueListTuple` 实现 hash 与 equals（要严格处理 NULL 语义——GROUP BY 中 NULL 与 NULL 应同组，以及类型一致性和浮点 NaN），查找/插入降到平均 O(K)。代码里向量化路径的 `StandardAggregateHashTable` 已经是这个思路的骨架，可以参照。组数超内存时还要考虑分区 spill。

**问：350 MB 内存限制下，如何设计外排的内存预算？**

答：原则是"预算先于实现"：先给各组件分账（存储层、JOIN、排序、结果集），排序侧再拆成 run 缓冲区、归并 reader 数 × 单 reader 缓冲、归并堆。MiniOB 的选择是固定 5 MB run buffer + 每 run 1000 条上限，宁可多写 run 文件也压低峰值；遗留风险是切换靠估算而非实测、归并扇入无上限。更稳的方案是运行时用 MemTracer 的 `allocated_memory()` 动态统计，逼近预算立即 flush，归并限制扇入分多轮，并给 JOIN 中间结果同样设预算。

## 回到赛题

本章知识直接支撑三道赛题，详细题面解析与实测证据见对应章节：

- **赛题 15 order-by**（[赛题 9—16 解析](../03_problems_09_16.md)）：多键字典序比较、ASC/DESC、排序稳定性都落在 `OrderByPhysicalOperator::fetch_and_sort_tables()` 的内存排序路径上。本章"MiniOB 源码实现"一节的比较器片段就是本题核心；注意 sequence 兜底在整行比较之后，不是真正的稳定排序。
- **赛题 10 group-by**（[赛题 9—16 解析](../03_problems_09_16.md)）：`ScalarGroupByPhysicalOperator` 的空输入语义（COUNT=0、其余 NULL）、`HashGroupByPhysicalOperator` 的线性查找退化、`aggregator.h` 的 NULL 跳过与 AVG 状态设计，分别对应本章"聚合执行"和"MiniOB 源码实现"两节。HAVING 与 WHERE 的执行顺序是本题 HAVING 子句实现的理论依据。
- **赛题 24 big-order-by**（[赛题 17—24 解析](../04_problems_17_24.md)）：本章外部排序的全部内容——两阶段、run 生成阈值（5 MB / 1000 条）、最小堆归并、`TempFileManager` 的 RAII 清理、350 MB 预算意识——都是为本题服务的；四表笛卡尔积 160,000 行的数字演算见"数字演算"小节。实现指南还可参考仓库根的 [BIG_ORDER_BY_README.md](../../../../BIG_ORDER_BY_README.md)。

执行模型与 SQL 流水线的整体背景（火山模型 open/next/close、算子树如何组装）见 [项目架构与一条 SQL 的完整执行链](../01_project_architecture.md)；官方向量化聚合设计文档见 [miniob-aggregation-and-group-by.md](../../design/miniob-aggregation-and-group-by.md)。
