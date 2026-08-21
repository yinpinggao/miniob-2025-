# 向量检索：距离度量、精确搜索与 ANN

本章导读：前面章节讲的索引（B+ 树）回答的都是"等于谁、在哪个范围"这类问题。但 2025 赛题里有两道题（vector-basic、vector-search）要求 MiniOB 回答另一类问题："和谁最像"。这就是向量检索。本章从"为什么数据库要存向量"讲起，依次讲清三种距离度量（L2 / 内积 / 余弦）、精确检索的执行方式与复杂度、为什么高维空间让传统索引失效（维度灾难）、ANN（近似最近邻）如何用一点召回率换数量级的速度，重点精讲 IVF 索引，然后落到 MiniOB 源码：`VectorType`、builtin 距离函数、`IvfflatIndex`、`VectorIndexScanRewrite` 重写规则和 `VectorScanPhysicalOperator` 物理算子，最后对比 Milvus、pgvector 等工业实现。

## 1. 为什么数据库要存向量

### 1.1 Embedding：把语义变成一串数字

先看一个生活类比。假设你要给全班同学按"口味相似度"分组，每个人对"辣、甜、咸"各打一个 0 到 10 的分，于是一个人就变成了一个三维点，比如 `[8, 2, 5]`。口味相近的人，在这个三维空间里的点也靠得近。"按口味分组"就变成了"找离自己最近的点"。

Embedding（嵌入）做的就是同一件事，只是打分维度不是 3 维而是几百上千维：用一个训练好的神经网络模型，把一段文本、一张图片映射成一个高维浮点向量，使得**语义相近的对象在向量空间中距离也近**。例如官方赛题文档（`docs/docs/game/miniob-vectordb.md`）里给出的例子：

```text
"West Highland White Terrier": [0.0296700, 0.0231020, 0.0166550, 0.0642470, -0.0110980, ...]
```

"国王"和"王后"的向量会很接近，"国王"和"电饭煲"的向量会很远。于是"找意思相近的文档/图片"这个模糊的语义问题，被转化成了一个精确的几何问题：**在高维空间里找最近的 K 个邻居（KNN, K-Nearest Neighbors）**。

### 1.2 语义检索与 RAG

向量检索最典型的两个应用：

- 语义搜索：用户搜"如何哄小孩睡觉"，即使文档里没有出现这几个字，只要语义相近就能被召回，这是全文索引那类关键词匹配做不到的。
- RAG（检索增强生成）：大语言模型回答前，先用问题向量去向量数据库里查出最相关的几段资料，塞进 prompt 里让模型参考。向量数据库的召回精度和速度直接影响 RAG 系统的质量。

这就是为什么数据库要原生支持"向量"这种数据类型：向量本质上就是一行数据的一个列，它需要和标量列（id、标题、标签）一起存储、一起过滤、一起参与事务，而不是放在数据库外面的某个孤立系统里。

## 2. 三种距离度量

"找最近的邻居"首先要定义"近"。MiniOB 赛题要求实现三种，它们都在 `src/observer/sql/builtin/builtin.cpp` 中有对应实现。

### 2.1 L2 距离（欧氏距离）

公式（d 维向量 A、B）：

```text
D(A, B) = sqrt( (A1-B1)^2 + (A2-B2)^2 + ... + (Ad-Bd)^2 )
```

几何直觉：就是两点之间的直线距离，我们从小到大理解的"距离"。

数值例子：取 A = `[3, 4]`，B = `[4, 3]`（二维，方便画图）：

```text
D = sqrt( (3-4)^2 + (4-3)^2 ) = sqrt( 1 + 1 ) = sqrt(2) ≈ 1.41
```

特点：同时受"方向"和"长度"影响。模长差很多的两个向量，即使方向完全一样，L2 距离也不小。

### 2.2 内积（点积）

公式：

```text
IP(A, B) = A1*B1 + A2*B2 + ... + Ad*Bd
```

几何直觉：由公式 `IP = |A| * |B| * cos(θ)` 可知，内积同时奖励"方向一致"（cos θ 大）和"模长大"。所以它衡量的不是"距离"而是"相似度"——**越大越相似**，这一点在写 SQL 时非常关键（见第 5、6 节）。

数值例子：同样 A = `[3, 4]`，B = `[4, 3]`：

```text
IP = 3*4 + 4*3 = 24
```

### 2.3 余弦距离

先看余弦相似度：

```text
cos(A, B) = (A·B) / (|A| * |B|)
```

几何直觉：只关心两个向量的**夹角**，完全不关心长度。cos = 1 表示方向完全相同，cos = 0 表示正交，cos = -1 表示方向相反。

数值例子：A = `[3, 4]`，B = `[4, 3]`，两者模长都是 5：

```text
cos(A, B) = 24 / (5 * 5) = 0.96
```

数据库里通常存"余弦距离"而不是相似度。MiniOB 的实现是 `1 - cosine_similarity`（见 `src/observer/sql/builtin/builtin.cpp` 中 `cosine_distance`，约 396-415 行），所以上例的余弦距离是 `1 - 0.96 = 0.04`，**越小越相似**。注意官方赛题文档题面里写的 `cosine_distance` 公式是相似度公式，而 MiniOB 实际返回的是"1 减相似度"，这是读题时容易混淆的一个细节。

一个体现余弦与 L2 语义差异的经典例子：`[1, 1]` 和 `[2, 2]` 方向完全相同（余弦距离为 0），但 L2 距离是 `sqrt(2) ≈ 1.41`。如果向量是文档的词频统计，"同一话题的长文和短文"应该用余弦距离判定为相似——这正是文本检索偏爱余弦的原因。

### 2.4 三种度量对比

| 度量 | 公式核心 | 相似方向 | 关注什么 | 典型场景 |
|------|----------|----------|----------|----------|
| L2 距离 | `sqrt(Σ(Ai-Bi)^2)` | 越小越近 | 方向 + 长度 | 图像特征、坐标 |
| 内积 | `Σ Ai*Bi` | 越大越像 | 方向 + 长度 | 推荐系统打分 |
| 余弦距离 | `1 - cos(θ)` | 越小越近 | 只看方向 | 文本 Embedding |

一个重要的数学事实：如果所有向量都预先做了 L2 归一化（模长为 1），那么余弦相似度就等于内积，L2 距离的排序也与它们一致（`L2^2 = 2 - 2*cos`）。很多工业系统（包括部分 OpenAI Embedding 模型）输出归一化向量，此时用内积即可，计算最省。

## 3. 精确检索：全表扫描 + top-k

### 3.1 执行流程

没有向量索引时，"找最相似的 K 条"只能这样做：

```text
对表中每一行（共 N 行）：
    1. 读出该行的向量列（d 维 float）
    2. 计算它与查询向量的距离          —— O(d)
    3. 维护一个大小为 K 的堆，保留目前最近的 K 条  —— O(log K)
最后输出堆中的 K 条结果
```

这就是精确检索（exact KNN / brute force），结果 100% 准确，因为每一行都被比较过。SQL 层面就是 `ORDER BY distance(...) LIMIT K`：先对全部 N 行算出距离并排序，再取前 K 条。

### 3.2 复杂度

- 距离计算：N 行 × 每行 O(d) = **O(N·d)**。
- 取 top-k：用大小为 K 的堆是 O(N·log K)；如果像 MiniOB 当前的 `OrderByPhysicalOperator` 那样先物化再全排序（见 `src/observer/sql/operator/order_by_physical_operator.cpp` 的 `fetch_and_sort_tables`，数据超阈值时还会走外部排序），则是 O(N·log N)。
- 总复杂度约 O(N·d)。代入一个工业级数字感受一下：N = 1 亿条、d = 768 维（BERT 类模型常见维度），一次查询约需 768 亿次乘加运算，秒级甚至更久的延迟，无法支撑在线服务。

### 3.3 维度灾难：为什么高维索引难做

低维空间里我们有 R 树等空间索引，为什么高维不行？这就是**维度灾难（Curse of Dimensionality）**，体现在两个方面：

1. 距离集中现象。维数 d 增大时，任意点到其最近邻和最远邻的距离之比趋近于 1——所有点看起来"差不多远"，"最近邻"这个概念本身的区分度在消失。
2. 空间划分失效。R 树靠矩形（MBR）层层划分空间，但高维下 MBR 之间严重重叠，一次查询几乎要下钻到所有子树；经验上维数超过十几维，树形空间索引就退化得接近顺序扫描。B+ 树更是一维有序结构，天然无法表达高维邻近关系。

所以高维向量检索的主流答案不是"更聪明的精确索引"，而是干脆放松要求——这就是下一节的 ANN。

## 4. ANN：用一点召回率换数量级的速度

### 4.1 核心思想

ANN（Approximate Nearest Neighbor，近似最近邻）的思路是：**不再保证返回的 K 条一定是全局最近的 K 条，只保证"大概率是"**。衡量质量的指标叫召回率（recall@K）：

```text
recall@K = ANN 返回的 K 条结果中，属于"精确检索真 top-K"的比例
```

业务上 recall 达到 0.90~0.99 通常就足够（搜索结果稍微差一点用户感知不到），而换来的往往是 10~100 倍的速度提升。2025 赛题的 ann-benchmarks 测试要求就是：recall ≥ 0.90 且 QPS ≥ 100。

### 4.2 IVF（Inverted File）精讲

IVF 的中文是"倒排文件"，它和全文索引的倒排是同一个思想：**先粗筛出一小撮候选，再在候选里精算**。IVF-Flat 分"建索引"和"查询"两个阶段。

建索引（离线，一次）：

```text
1. 用 k-means 把全部 N 个向量聚成 nlist 个簇，每个簇算出一个质心（centroid）
   - 随机选 nlist 个点当初始质心
   - 迭代：每个向量归入最近质心 → 每个簇取均值更新质心 → 直到收敛
2. 为每个质心维护一个"倒排桶"（posting list），存放归入该簇的
   所有向量（原始向量，不做压缩——这就是 "Flat" 的含义）
```

查询（在线，每次）：

```text
1. 计算查询向量与全部 nlist 个质心的距离          —— O(nlist · d)
2. 取最近的 nprobe 个质心（nprobe <= nlist）
3. 只在这 nprobe 个桶里逐条精算距离，取 top-K    —— O(nprobe · N/nlist · d)
```

用一个具体数字演算：N = 100 万条，d = 128，取 nlist = 1000、nprobe = 10。

- 精确检索：100 万 × 128 ≈ 1.28 亿次乘加。
- IVF：质心粗筛 1000 × 128 ≈ 12.8 万次；精算 10 个桶 × 每桶约 1000 条 × 128 ≈ 128 万次。合计约 140 万次，**约为精确检索的 1/90**，而 recall 通常仍有 0.9 以上。

代价是什么？看这张图：

```text
        簇A(质心a)      簇B(质心b)      簇C(质心c)
          · ·            · q·            · ·
         ·  ·           ·   ·           ·   ·
          · ·            · ·             · ·
                         ↑
                    查询向量 q 落在簇B边缘
```

q 离质心 b 最近，所以只探簇 B（nprobe=1）。但 q 真正的最近邻可能恰好落在簇 A 靠近边界的位置——这次查询就"漏招"了。把 nprobe 调大到 2~3，把相邻的簇 A 也探一遍，就能把它找回来。**IVF 的误差主要来自簇边界附近的点。**

### 4.3 nlist / nprobe 的权衡

- `nlist`（lists）：建索引时的簇数。越大则每个桶越小、查询时精算量越少，但质心粗筛的成本（O(nlist·d)）和边界效应都上升。经验法则是 `nlist ≈ sqrt(N)` 量级（赛题 ann-benchmarks 对约 6 万条数据用 `lists=245`，正是这个量级）。
- `nprobe`（probes）：查询时探测的桶数。**这是 recall 与延迟的直接旋钮**：nprobe=1 最快但 recall 最低；nprobe=nlist 时退化为精确检索（recall=1，但失去了索引意义）。赛题要求 `probes=5`。

| probes 调大 | recall | 延迟 |
|-------------|--------|------|
| 方向 | 升高 | 升高 |
| 原因 | 探的桶多，边界点漏得少 | 精算的候选变多 |

### 4.4 HNSW 一句话对比

另一类主流 ANN 索引是 HNSW（分层可导航小世界图）：把向量组织成多层图，查询时从顶层稀疏图"贪心游走"到底层，逐步逼近最近邻。相比 IVF，HNSW 通常召回率更高、查询更快，但内存占用更大、建索引更慢。pgvector 0.5.0 起同时支持 IVFFlat 和 HNSW；MiniOB 目前只实现了 IVF-Flat。

## 5. 向量如何进入 SQL

以 MiniOB 支持的语法为例（来自 `docs/docs/game/miniob-vectordb.md`），看向量数据完整的一生：

```sql
-- 1. 建表：vector(3) 声明一个 3 维向量列
CREATE TABLE items (id int, embedding vector(3));

-- 2. 插入：向量以字符串字面量 '[1,2,3]' 的形式写入，由数据库解析成 float 数组
INSERT INTO items VALUES (1, '[1,2,3]');

-- 3. 标量计算：逐元素加/减/乘、字典序比较
SELECT embedding + '[1.5,2.3,3.3]' FROM items WHERE embedding > '[0,0,0]';

-- 4. 距离表达式
SELECT l2_distance(embedding, '[1,2,3]') FROM items;

-- 5. 邻近检索（KNN 查询）：按距离排序取前 K
SELECT * FROM items ORDER BY l2_distance(embedding, '[1,2,3]') LIMIT 5;

-- 6. 建向量索引
CREATE VECTOR INDEX vec_idx ON items (embedding)
WITH (distance=l2_distance, type=ivfflat, lists=245, probes=5);
```

几个关键点：

- `VECTOR` 类型：定长 float 数组，维度在建表时写死。赛题要求最大支持 16000 维——MiniOB 在 `src/observer/storage/table/table.cpp`（约 128 行）建表时检查 `att.length > 16000 * sizeof(float) + 1` 来卡这个上限。
- 字符串与向量的互转由内置函数完成：`string_to_vector` / `vector_to_string`（`src/observer/sql/builtin/builtin.cpp` 第 427、440 行）。这套函数命名与 MySQL 9.0 引入的向量函数一致；插入语句里的 `'[1,2,3]'` 之所以能直接写进向量列，靠的是类型系统里 CHARS → VECTORS 的隐式 cast。
- 距离函数作为普通标量表达式参与计算，因此 `ORDER BY l2_distance(embedding, '[...]') LIMIT k` 在语义上就是"全表算距离 + 排序 + 取前 k"。**优化器能不能把这个模式识别出来并改走向量索引，是精确检索与 ANN 的分水岭**，也是第 6.4 节的重写规则要干的事。
- 排序方向要与度量匹配：L2 / 余弦距离越小越好，用 `ORDER BY ... ASC`（默认）；内积越大越好，必须用 `DESC`。写反了查出来的是"最不像"的 K 条。

## 6. MiniOB 实现

### 6.1 `VectorType`：定长 float 数组

`src/observer/common/type/vector_type.h` 定义了 `VectorType : public DataType`（类型枚举 `AttrType::VECTORS`），`vector_type.cpp` 里实现了几个核心操作：

- `compare`：先比维度（元素个数），维度相同再逐元素比较，全部相等才相等——这就是赛题要求的"字典序比较"，例如 `[1,2,3] < [1,2,4]`。
- `add` / `subtract` / `multiply`：逐元素运算；两个向量维度不一致时返回 `RC::VECTOR_DIM_MISMATCH`。
- `set_value_from_str`：调用 `parse_vector_from_string`（`src/observer/common/utils.h:24`）把 `'[1,2,3]'` 文本解析成 float 数组；`to_string` 则把 float 数组拼回 `[x,y,z]` 文本。

内存布局上，一个 `vector(d)` 值就是连续存放的 d 个 4 字节 float，`Value` 里存裸指针加长度，取第 i 维就是把指针当 `float*` 用下标访问——这也是后面距离计算能跑得快的基础。

### 6.2 builtin 距离函数

`src/observer/sql/builtin/builtin.cpp` 里有两层实现：

第一层面向 SQL 表达式求值，统一入口是匿名命名空间里的 `vector_distance::distance(args, result, type)`（约 290-366 行）：先检查两个向量维度一致，再按 `NormalFunctionType` 分派：

- `L2_DISTANCE`：累加 `(v0-v1)^2`，最后开方；
- `COSINE_DISTANCE`：一趟循环同时累加点积 `dot_product` 和两个模长平方 `norm_v0`、`norm_v1`，返回 `1 - dot/(sqrt(norm_v0)*sqrt(norm_v1))`；任一向量是零向量（模长小于 `EPSILON`）时返回 NULL，避免除零；
- `INNER_PRODUCT`：累加 `v0*v1`。

外层再包出三个 SQL 函数 `l2_distance` / `cosine_distance` / `inner_product`（第 368-381 行）。一趟循环算完全部中间量（而不是先算点积再各算一遍模长），是这里值得注意的小优化。

第二层是面向索引的 `std::vector<float>` 重载版本（第 385、396、418 行），逻辑相同但直接操作 float 数组，供 `IvfflatIndex` 在构建和查询时高频调用，避免 `Value` 包装的开销。

表达式侧，`src/observer/sql/expr/expression.h:478` 的 `is_vector_distance_func()` 用来判断一个函数表达式是不是这三种距离函数之一——重写规则正是靠它筛选查询模式的。

### 6.3 `IvfflatIndex`：内存态 IVF-Flat

`src/observer/storage/index/ivfflat_index.h` 定义的 `IvfflatIndex` 继承自 `Index` 基类，核心成员如下：

```cpp
NormalFunctionType distance_fn_;   // 建索引时指定的距离度量
std::vector<Vector> centroids_;    // nlist 个质心
// 每个质心一个倒排桶，桶内存 (向量, RID) 对
std::vector<std::vector<std::pair<Vector, RID>>> centroids_buckets_;
int lists_  = 1;                   // nlist
int probes_ = 1;                   // nprobe
```

`RID`（Record ID）是记录在堆表里的物理地址，桶里存它是为了查询命中后能回表取整行。注意 `create()` 里的注释"暂时先不支持持久化"：与 B+ 树索引不同，这是一个**内存态索引**（`sync()` 是空实现），重启后需要重建。

构建在 `build_index()`（`ivfflat_index.cpp` 第 152-208 行），就是第 4.2 节那套 k-means 的直白实现：

- 参数从 `options` 里取 `lists` 和 `probes`（即建索引语句 `WITH (lists=..., probes=...)`）；
- 用 `std::mt19937` 随机数从初始数据里随机抽 `lists_` 个点当初始质心；
- Lloyd 迭代：每轮把每个向量归入最近质心（`find_centroid`）、各簇取均值作为新质心（`find_centroids`），若所有质心移动距离都不超过 0.01 或达到 `MAX_ITERATIONS`（定义为 5）就停止；
- 最后把每个向量分配到最近质心的桶 `centroids_buckets_[index]`。

构建复杂度是 O(迭代次数 × N × nlist × d)，因为每轮每个向量都要和全部质心算一次距离。

查询在 `ann_search()`（第 210-255 行），完整对应第 4.2 节的查询流程：

```cpp
// 1. 与所有质心算距离
for (size_t i = 0; i < centroids_.size(); i++) {
  float dist = compute_distance(base_vector, centroids_[i], distance_fn_);
  centroid_distances.emplace_back(dist, i);
}
// 2. 排序，取最近的 probes_ 个簇，扫桶内向量作为候选
size_t probe_count = std::min(static_cast<size_t>(probes_), centroid_distances.size());
for (size_t i = 0; i < probe_count; i++) {
  size_t cluster_index = centroid_distances[i].second;
  for (const auto &[vec, rid] : centroids_buckets_[cluster_index]) {
    float dist = compute_distance(base_vector, vec, distance_fn_);
    candidates.emplace_back(dist, rid);
  }
}
// 3. 候选排序，取前 limit 个 RID 返回
```

逐行看：`compute_distance` 按 `distance_fn_` 分派到 `builtin::l2_distance` 等三个 float 版本；`is_better_distance`（第 46-52 行）处理排序方向——内积越大越好，其余越小越好，这一处差异贯穿建索引和查询的所有比较；最终结果只返回 RID 数组，取记录是上层算子的事。查询复杂度 O(nlist·d + nprobe·(N/nlist)·d)，与第 4.2 节分析一致。

增量维护上，`insert_entry` 把新向量直接塞进最近的桶（空索引时第一条向量自成唯一质心），**不会重新跑 k-means**——所以持续插入后簇划分会逐渐偏离最优，recall 缓慢下降，工业系统靠定期重建索引解决。`delete_entry` 则按 RID 线性扫所有桶找到并删除。

### 6.4 `VectorIndexScanRewrite`：把 ORDER BY + LIMIT 识别为向量扫描

这是全章最能体现"优化器思维"的部分。第 5 节说过，`ORDER BY distance(...) LIMIT k` 的默认语义是"全表算距离 + 排序 + 取前 k"。如果向量列上建了 IVF 索引，我们希望优化器把这个**特定查询模式**整体替换为一次索引扫描。MiniOB 用一条重写规则实现：`src/observer/sql/optimizer/vector_index_scan_rewrite.cpp` 的 `VectorIndexScanRewrite`，它在 `src/observer/sql/optimizer/rewriter.cpp:28` 被注册进规则列表，由 `OptimizeStage::rewrite` 循环应用到收敛。

规则要做的是严格的模式匹配，`rewrite()` 里层层设卡：

```text
PROJECT
 └── LIMIT                      ← 必须有 LIMIT
      └── ORDER_BY              ← LIMIT 下必须是 ORDER BY
           └── TABLE_GET        ← 排序键来自一次普通表扫描
```

具体条件（任何一个不满足就原样返回，不做改写）：

1. ORDER BY 只有一个排序键，且该键是普通函数表达式；
2. 该函数是向量距离函数（`is_vector_distance_func()`）；
3. 排序方向与度量兼容——`is_order_compatible()`（第 20-28 行）规定：L2 / 余弦必须 ASC，内积必须 DESC，写反了就不走索引（因为索引只会按"更相似"的顺序给结果）；
4. 距离函数的两个参数一边是字段（`FieldExpr`），另一边是能直接求值的常量向量；赛题也只要求支持"向量列 vs 常量向量"；
5. 该字段所在的表上存在距离度量类型匹配的向量索引——`Table::find_vector_index(distance_fn, field_name)`（`src/observer/storage/table/table.cpp:1741`）遍历表上所有索引，找到 `is_vector_index()` 且 `distance_fn` 一致、字段名一致的那个。

全部命中后，规则把索引、查询向量、limit 值塞进 `TableGetLogicalOperator`（`set_index` / `set_base_vector` / `set_limit`），然后执行关键一行：

```cpp
oper->children()[0] = std::move(table_scan_oper);
```

直接把 LIMIT 和 ORDER_BY 两个逻辑节点**整棵摘掉**，因为它们的工作（排序、截断）已经内化到索引扫描里。改写前后对比：

```text
改写前：                     改写后：
PROJECT                    PROJECT
 └── LIMIT                      └── TABLE_GET(带向量索引参数)
      └── ORDER_BY                   ↓ 物理计划阶段
           └── TABLE_GET             VECTOR_INDEX_SCAN
```

这种"识别特定查询子树模式、整体替换为更高效的等价物"是数据库优化器里非常典型的思路（和谓词下推、常量折叠同属规则优化），面试时可以主动点出这一层。

### 6.5 `VectorScanPhysicalOperator`：拿 RID、回表、过 MVCC

物理计划生成阶段，`PhysicalPlanGenerator::create_plan(TableGetLogicalOperator&)`（`src/observer/sql/optimizer/physical_plan_generator.cpp:166`）发现 `table_get_oper.is_vector_scan()` 为真，就创建 `src/observer/sql/operator/vector_scan_physical_operator.h` 定义的 `VectorScanPhysicalOperator`（算子类型 `PhysicalOperatorType::VECTOR_INDEX_SCAN`）。

它的 `open()` 只有一步核心动作：

```cpp
rids_ = index_->ann_search(base_vector_, limit_);
record_handler_ = table_->record_handler();
```

即打开算子时一次性向索引要到 top-limit 的候选 RID 数组。之后每次 `next()` 从数组取下一个 RID，做三件事：

1. 回表：`record_handler_->get_record(rid, current_record_)` 按 RID 取回整条记录；
2. 过滤：执行下推下来的普通谓词（`filter()`），比如 KNN 之外的 `WHERE tag = 'book'`；
3. 可见性判断：`trx_->visit_record(...)` 做 MVCC 检查，对本事务不可见的版本跳过。

`param()` 返回"索引名 ON 表名"，所以 `EXPLAIN` 里能看到官方文档示例中的计划：

```text
Query Plan
OPERATOR(NAME)
PROJECT
└─VECTOR_INDEX_SCAN(V_I ON TEST)
```

对比第 3 节的无索引路径（`TableScan` → 每行算距离 → `OrderBy` 全量物化排序 → `Limit`），有索引时整条流水线坍缩成了一个扫描算子，这就是 ANN 快的原因。

### 6.6 当前实现的边界

读源码时要诚实面对边界，这些恰恰是面试追问的好素材：

- **索引只取 limit 个候选，过滤后不补取**。`open()` 里源码注释写得很明白：如果候选记录之后被 WHERE 谓词或 MVCC 过滤掉若干条，最终返回会**少于 LIMIT 条**，而不是向索引多要一批补上。这是 ANN 与关系过滤组合的经典陷阱，工业界称为 filtered ANN 问题，pgvector 等实现用"迭代式扫描（overscan）"缓解。
- 索引是纯内存结构，不落盘、不写 WAL，重启重建。
- k-means 初始质心随机选取且只迭代最多 5 轮，簇划分质量有随机性；增量插入不重算质心。
- 重写规则只认"单表 + 单排序键 + 列与常量向量的距离 + 方向匹配 + 有索引"这一种模式，join、多列排序、两个列之间的距离查询都会落回精确检索路径。

## 7. 工业界怎么做

| 系统 | 向量能力 | 索引 | 特点 |
|------|----------|------|------|
| pgvector（PostgreSQL 扩展） | `vector` 类型，操作符 `<->`（L2）、`<#>`（负内积）、`<=>`（余弦距离） | IVFFlat（lists/probes）与 HNSW（m、ef_construction、ef_search） | 复用 PG 的 WAL、MVCC、查询优化器，与标量过滤自由组合 |
| Milvus | 专用向量数据库 | IVF_FLAT、IVF_PQ（量化压缩）、HNSW、DiskANN 等 | 分布式、支持 GPU，面向十亿级向量 |
| MySQL | 9.0 起提供原生 `VECTOR` 类型与 `STRING_TO_VECTOR` / `VECTOR_TO_STRING` / `DISTANCE` 函数 | 无内置 ANN 索引（HeatWave 服务另有一套） | MiniOB 的向量函数命名正是对齐这套接口 |
| OceanBase | 4.3.3 版本起支持向量索引 | HNSW 系列 | 与关系数据一体化存储、参与事务 |

可以看到两条路线：以 pgvector 为代表的"在成熟关系库上加向量类型和 ANN 索引"，和以 Milvus 为代表的"专用向量数据库"。MiniOB 赛题走的是第一条路的教学简化版：类型系统加 `VECTOR`、表达式加距离函数、优化器加一条重写规则、执行器加一个扫描算子——麻雀虽小，每一层都动到了，这正是它适合面试展开讲的原因。

## 小结

- 向量检索的本质：用 Embedding 把语义映射为高维空间中的几何邻近，"找最相似的 K 个"即 KNN 查询。
- 三种度量：L2（直线距离，越小越近）、内积（越大越像）、余弦距离（1 − cos，只看方向，越小越近）；归一化向量上三者排序等价。
- 精确检索 = 全表算距离 + top-k，复杂度 O(N·d)；维度灾难（距离集中、空间划分失效）使高维下传统索引不可用。
- ANN 牺牲少量 recall 换数量级加速。IVF = k-means 聚类（nlist 个质心）+ 倒排桶 + 查询只探最近 nprobe 个桶；nprobe 是 recall-延迟的旋钮。HNSW 是图索引路线，召回和速度更好、内存更大。
- MiniOB 实现链路：`VectorType`（定长 float 数组、字典序比较、逐元素运算）→ builtin 距离函数（一趟循环算点积与模长）→ `IvfflatIndex`（内存态，`centroids_` / `centroids_buckets_` / `ann_search`，Lloyd 迭代最多 5 轮）→ `VectorIndexScanRewrite`（识别 PROJECT-LIMIT-ORDER_BY-TABLE_GET 模式并整体替换）→ `VectorScanPhysicalOperator`（`ann_search` 拿 RID、回表、过谓词和 MVCC）。
- 边界要记牢：候选被过滤后不补取可能少返回、索引不落盘、增量插入不重聚类。

## 面试追问

**Q1：余弦距离和内积是什么关系？什么时候可以互相替代？**

内积同时受方向和模长影响，余弦只看方向（相当于先把两个向量归一化再算内积）。如果所有向量在写入前都做了 L2 归一化（模长为 1），内积就等于余弦相似度，且 L2 距离的排序也与之一致，此时用内积最划算——少算两个模长和一次除法开方。很多 Embedding 模型输出就是归一化向量，工业系统常利用这一点。

**Q2：为什么 B+ 树 / R 树索引不了高维向量？**

B+ 树是一维全序结构，高维向量之间没有天然全序。R 树靠矩形划分空间，但高维下有两个致命问题：一是距离集中，最近邻与最远邻的距离比趋近 1，"近"失去区分度；二是各层矩形严重重叠，一次查询几乎要遍历所有子树，经验上十几维以上就退化到接近全表扫描。这就是维度灾难，所以高维检索转向 ANN。

**Q3：IVF 的 recall 和延迟由什么决定？probes 和 lists 分别怎么调？**

probes（nprobe）是查询时的直接旋钮：探的桶越多，簇边界附近的真邻居越不容易漏，recall 越高，但精算候选变多、延迟线性上升；probes=nlist 时退化为精确检索。lists（nlist）影响桶的粒度：lists 越大每桶越小、精算越少，但质心粗筛成本 O(nlist·d) 上升，边界效应也更明显，经验取 sqrt(N) 量级。赛题 ann-benchmarks 用的就是 lists=245、probes=5，要求 recall ≥ 0.90。

**Q4：MiniOB 里一条 SQL 满足什么条件才会走向量索引？为什么要检查 ORDER BY 的方向？**

`VectorIndexScanRewrite` 要求：查询是"PROJECT → LIMIT → ORDER BY → TABLE_GET"结构；ORDER BY 只有一个键且是三种距离函数之一；参数一边是向量列、一边是常量向量；该列上有距离度量匹配的 `IvfflatIndex`。方向检查（`is_order_compatible`）是因为索引按"更相似优先"给出结果：L2/余弦越小越相似所以要求 ASC，内积越大越相似所以要求 DESC，写反了用索引会给出语义错误的结果，只能回退精确路径。

**Q5：ANN 查询叠加 WHERE 过滤会有什么问题？MiniOB 是怎么表现的？**

问题在于"先近似后过滤"：索引只按 limit 取出候选 top-K，这些候选再被 WHERE 或 MVCC 过滤掉一部分后，结果就不足 K 条了。MiniOB 的 `VectorScanPhysicalOperator::open()` 只在打开时调用一次 `ann_search(base_vector_, limit_)` 取候选，源码注释明确说明被过滤后不会向索引补取，因此可能返回少于 LIMIT 条。工业界的做法包括过滤感知建索引（按标量分区）或迭代式扩大候选（overscan），pgvector 的迭代扫描就是后者。

**Q6：精确 top-K 检索的复杂度是多少？还能怎么优化？**

N 条 d 维向量：距离计算 O(N·d)，维护大小为 K 的最大堆取 top-K 是 O(N·log K)，总体 O(N·d)。MiniOB 当前的无索引路径是物化全部排序键后 `std::stable_sort`（O(N·log N)，超阈值走外部排序），没有做成堆式 top-K，这是一个可以指出的优化点。工程上还可以用 SIMD 批量算距离、PQ 量化压缩向量减少内存带宽，或者干脆上 ANN 索引。

## 回到赛题

本章直接对应两道 2025 赛题：

- **16 vector-basic**（解析见 [赛题 09-16](../03_problems_09_16.md)）：要求实现 `VECTOR` 类型、字符串与向量互转、逐元素算术/字典序比较、三个距离函数，对应本章第 2、5 节的概念与 6.1、6.2 节的实现（`src/observer/common/type/vector_type.cpp`、`src/observer/sql/builtin/builtin.cpp`）。官方题面见 [MiniOB 向量数据库](../../game/miniob-vectordb.md) 的"题目一：向量类型基础功能"。
- **18 vector-search**（解析见 [赛题 17-24](../04_problems_17_24.md)）：这里要分清两层要求。**题面要求**的是精确检索能力——"在没有索引的场景下支持向量检索（即精确检索），并支持完整的向量索引功能"，也就是本章第 3 节的 `ORDER BY 距离 + LIMIT K` 全表扫描路径必须先正确；**仓库扩展的 ANN 能力**（IVF-Flat 索引与计划改写）则对应官方题面的"向量索引（一）（二）"：第 4 节的 IVF 原理、6.3 节 `IvfflatIndex` 的 `centroids_` / `centroids_buckets_` / `ann_search`、6.4 节 `VectorIndexScanRewrite` 的模式识别、6.5 节 `VectorScanPhysicalOperator` 的回表执行。ann-benchmarks 测试（recall ≥ 0.90、QPS ≥ 100、内存 ≤ 1GB）正是用第 4.3 节的 recall-延迟权衡来评判的。

把这两题串起来的一条面试叙述线是：类型系统（`VectorType`）→ 表达式（距离函数）→ 优化器（重写规则识别 ORDER BY + LIMIT 模式）→ 执行器（向量扫描算子）→ 存储/索引（IVF-Flat 内存索引），一条 SQL 的向量检索改造贯穿了数据库内核的每一层，各层职责可参考 [项目架构与一条 SQL 的完整执行链](../01_project_architecture.md)。
