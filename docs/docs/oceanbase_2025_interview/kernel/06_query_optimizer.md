# 查询优化：逻辑计划、重写规则与代价模型

## 本章导读

SQL 是声明式语言：你只说"要什么"（`SELECT ... WHERE ...`），不说"怎么取"。同一个查询，数据库可以用全表扫描逐行看，也可以走索引直接定位；可以先连接再过滤，也可以先过滤再连接。这些执行方式的结果完全一样，但代价可以差出几个数量级——一个跑 0.01 秒，另一个跑一小时。决定"怎么做"的组件就是**查询优化器**。

在 MiniOB 的 SQL 流水线（见 [项目架构](../01_project_architecture.md)）里，优化器对应 `OptimizeStage`：它把 `Stmt` 变成逻辑计划，用一组重写规则做等价变换，再生成物理计划交给执行层。本章按"直觉 → 原理 → 源码"的顺序讲清楚：

- 为什么优化器必不可少，代价差距从哪里来；
- 关系代数与逻辑计划的基本概念；
- RBO（基于规则的优化）：谓词下推、投影下推、常量折叠、表达式简化；
- CBO（基于代价的优化）：统计信息、选择率、代价模型、join order 枚举；
- MiniOB 优化器的真实实现：`OptimizeStage` 三步、四条重写规则、访问路径选择的启发式；
- EXPLAIN 如何暴露优化器的决策。

读完本章，你应该能回答面试中最常见的优化器问题，也能说清楚 MiniOB 做到了哪一步、没做哪一步、为什么。

## 1. 为什么 SQL 必须有优化器

### 1.1 一个生活类比

你要做三件事：去超市买菜、去银行取钱、去快递点取包裹。目标（"完成三件事"）是声明式的，但执行顺序由你决定。如果快递点 21:00 关门、超市 22:00 关门，聪明的顺序是先取快递再买菜；如果都不急，就按距离规划一条最省路的顺序。目标不变，执行代价（时间、路程）天差地别。

数据库面对的问题完全一样：用户给出"要的结果"（声明式），优化器负责在众多**等价**的执行方案里挑一个便宜的。

### 1.2 代价差距有多大：跟着算一遍

设两张表：

- `student(id, name, age)`：10 万行；
- `sc(id, sid, cid, score)`：选课记录，100 万行。

查询：

```sql
SELECT s.name, c.score
FROM student s, sc c
WHERE s.id = c.sid AND s.age = 20;
```

方案 A：**先连接，后过滤**（按 SQL 字面顺序天真执行，用嵌套循环连接）。先做 `student × sc` 的笛卡尔式匹配，比较次数是 `10^5 × 10^6 = 10^11`。假设每次比较 10 纳秒，仅连接判断就要约 1000 秒，之后才对结果过滤 `age = 20`。

方案 B：**先过滤，后连接**。`age = 20` 假设能从 10 万学生里筛出 100 人。先过滤再连接，比较次数变成 `100 × 10^6 = 10^8`，比方案 A 少 1000 倍，约 1 秒。

方案 C：**先过滤，再用索引连接**。如果 `sc.sid` 上有 B+ 树索引，对过滤出的每个学生，不需要扫 100 万行选课记录，而是一次索引查找：B+ 树高度 3~4 层，每次约 3~4 次页访问。总代价约 `100 × 4 = 400` 次页访问，毫秒级。

三个方案结果完全相同，代价从 1000 秒到毫秒，相差约 6 个数量级。这就是优化器存在的理由：**它不改变结果，只改变到达结果的路径**。

```text
同一个 SQL
   │
   ├─ 方案A 先Join后过滤      ~10^11 次比较   ~1000 s
   ├─ 方案B 先过滤后Join      ~10^8  次比较   ~1 s
   └─ 方案C 过滤 + 索引连接   ~400   次页访问  ~1 ms
```

### 1.3 优化器的两大流派

- **RBO（Rule-Based Optimization，基于规则）**：不管数据多少，只要计划匹配某个模式就做固定的等价改写，比如"过滤条件尽量往下推"。优点是简单、稳定、可预期；缺点是不看数据分布，可能改写后反而更慢。
- **CBO（Cost-Based Optimization，基于代价）**：用统计信息（表有多少行、列有多少不同值）估算每种方案的代价，枚举候选计划选最便宜的。优点是贴近真实数据；缺点是需要维护统计信息，估错了会选错计划。

工业数据库以 CBO 为主、RBO 为辅（先做规则改写，再在改写后的计划空间里做代价枚举）。MiniOB 主要实现了 RBO 的骨架，CBO 留空了——下文会逐层展开。

## 2. 关系代数速览：逻辑计划就是一棵代数树

关系数据库的理论底座是**关系代数**：把表（关系）看成集合，查询看成集合上的运算。常用运算：

| 运算 | 符号 | SQL 对应 | 含义 |
| --- | --- | --- | --- |
| 选择 Selection | σ | `WHERE` | 按谓词过滤行 |
| 投影 Projection | π | `SELECT` 列列表 | 只保留指定列/表达式 |
| 连接 Join | ⋈ | `JOIN` / 多表 FROM | 按条件组合两张表的行 |
| 并 Union | ∪ | `UNION` / `UNION ALL` | 合并两个结果集 |
| 聚合 Aggregation | γ | `GROUP BY` + 聚合函数 | 分组统计 |

一条 SQL 可以翻译成一棵关系代数运算树，这就是**逻辑计划**。例如：

```sql
SELECT s.name
FROM student s, sc c
WHERE s.id = c.sid AND s.age = 20;
```

对应的逻辑计划（算子树，数据自底向上流动）：

```text
        π name                    ← PROJECTION：只要 name 列
        │
        σ s.id = c.sid ∧ age=20   ← PREDICATE：过滤
        │
        ⋈                         ← JOIN：两表连接
       / \
  TableGet(s)  TableGet(c)        ← 读取关系
```

注意这棵树只表达"做什么"：JOIN 没说用嵌套循环还是哈希连接，TableGet 没说全表扫还是走索引。**"怎么做"是物理计划的事**——同一个逻辑 JOIN，物理上可以是 `NestedLoopJoin`、`GraceHashJoin`；同一个逻辑 TableGet，物理上可以是 `TableScan` 或 `IndexScan`。逻辑/物理分离是优化器的基本架构：先在逻辑层做与算法无关的等价改写，再在物理层做算法和访问路径的选择。

MiniOB 的逻辑算子类型定义在 `src/observer/sql/operator/logical_operator.h` 的 `LogicalOperatorType` 枚举里，共 13 种：`TABLE_GET`、`PREDICATE`、`PROJECTION`、`JOIN`、`UNION`、`GROUP_BY`、`ORDER_BY`、`LIMIT`、`INSERT`、`DELETE`、`UPDATE`、`CALC`、`EXPLAIN`。每个 `LogicalOperator` 持有 `children_`（子算子）和 `expressions_`（本算子内的表达式，如谓词、投影列）。

## 3. 等价变换与启发式规则（RBO）

### 3.1 什么叫"等价"

两个计划**等价**，当且仅当对任何合法的数据库状态，它们产出相同的结果（对 bag 语义要连同重复行数也一致）。等价变换的可靠性来自关系代数的数学性质，例如：

- 选择的级联：`σ p∧q (R) = σ p (σ q (R))`——一个 AND 条件可以拆成两层过滤；
- 选择与连接交换：当谓词 p 只涉及 R 的列时，`σ p (R ⋈ S) = σ p(R) ⋈ S`——过滤可以先做；
- 连接交换律与结合律：`R ⋈ S = S ⋈ R`，`(R ⋈ S) ⋈ T = R ⋈ (S ⋈ T)`（对 INNER JOIN 成立）。

启发式规则就是把这些恒等式写成"见到模式 A 就改成模式 B"的代码。下面四条是最经典的，每条都给改写前/改写后。

### 3.2 谓词下推：尽早过滤

**规则**：把选择（过滤）操作尽可能推向树的底部、靠近数据源的地方。

改写前：

```text
  σ age = 20
      │
      ⋈
     / \
  student  sc
```

改写后：

```text
      ⋈
     / \
 σ age=20  sc
    │
 student
```

**为什么几乎总是赚的**：回到 1.2 节的数字。不过滤直接连接是 `10^5 × 10^6 = 10^11` 次比较；先把 `student` 过滤成 100 行再连接是 `100 × 10^6 = 10^8` 次。过滤让上层算子的输入行数变少，而数据库里几乎一切代价（I/O、CPU、内存、网络）都随行数单调上升。行数是"最贵的资源"，所以"能早就早"。

**什么时候不能无脑下推**（面试常考）：

- 谓词涉及两张表（如 `s.id = c.sid`），它是连接条件，只能留在 JOIN 层；
- OUTER JOIN 上对保留侧表的过滤会改变语义（`LEFT JOIN` 的 ON 与 WHERE 不等价）；
- 含易变函数（`RAND()`）或子查询的谓词不能随意移动。

### 3.3 投影下推：只搬需要的列

**规则**：把投影下推到扫描层，尽早丢掉上层用不到的列。

改写前：扫描读出整行（20 个列），到最顶层才投影出 2 列。
改写后：扫描时只取 `id, name` 两列。

**为什么赚**：数据库按行搬运数据，行越宽，I/O 和内存开销越大。假设每行 1000 字节，只取两列后有效载荷可能只有 50 字节——同样的缓冲池能装 20 倍的行，连接、排序的内存压力同步下降。列存数据库（以及 MiniOB 的 PAX 存储格式）把这条规则发挥到极致：查询只读的列根本不用从磁盘读出来。MiniOB 主线的逻辑计划里 `PROJECTION` 仍在顶层，这条规则体现的是教学骨架的取舍。

### 3.4 常量折叠：编译期能算的别留到运行期

**规则**：表达式里只含常量的部分，在优化期直接算出结果。

改写前：`WHERE score > 60 + 40 - 10 AND 1 + 1 = 2`
改写后：`WHERE score > 90 AND TRUE`

**为什么赚**：原计划要对每一行都算一次 `60+40-10`，10 万行就是 10 万次加法；折叠后每行只做一次比较。更重要的是，折叠出 `TRUE`/`FALSE` 常量会给其他规则创造机会（见 3.6 的不动点循环）。

### 3.5 表达式简化：AND/OR 的恒真恒假传播

**规则**：利用布尔代数的吸收律化简逻辑表达式：

- `X AND TRUE → X`，`X AND FALSE → FALSE`（AND 中有一个恒假，整体恒假）；
- `X OR FALSE → X`，`X OR TRUE → TRUE`（OR 中有一个恒真，整体恒真）。

改写前：`WHERE age = 20 AND TRUE AND FALSE`
改写后：整个谓词折叠为 `FALSE`——优化器甚至可以让计划直接返回空结果集，一行都不用扫。

### 3.6 为什么规则要跑到"不动点"

单条规则每跑一次只能改一处。但规则之间会互相创造机会：常量折叠把 `1=1` 变成 `TRUE` → 表达式简化把 `x>5 AND TRUE` 收成 `x>5` → 谓词下推把 `x>5` 推进扫描层。所以优化器通常把全部规则**反复执行，直到一整轮没有任何改写发生**——这个状态叫**不动点（fixpoint）**。它要求每条规则都收敛（改写后的计划不会再被同一规则改回去），否则就是死循环。

**复杂度**：设算子树有 N 个节点、表达式总规模 E。单轮改写是对树的遍历，约 O(N+E)；轮数取决于规则间触发链的长度，实践中是个小常数，所以 RBO 整体接近线性，非常便宜。这也是 RBO 适合做 CBO 前置步骤的原因。

## 4. 基于代价的优化（CBO）

RBO 不问数据长什么样。可很多时候"该不该走索引、谁先和谁连接"恰恰取决于数据量。CBO 的思路是：**统计信息 → 选择率估计 → 代价模型 → 枚举候选计划取最小代价**。

### 4.1 统计信息与选择率：跟着算一遍

优化器为每张表/每列维护统计信息，最基本的三样：

- **行数（cardinality）**：表有多少行；
- **NDV（Number of Distinct Values）**：列有多少个不同值；
- **直方图（histogram）**：列值在各区间的分布，用来修正"均匀分布"假设。

**选择率（selectivity）** 是谓词过滤后剩余行数占比。均匀分布假设下：

- 等值条件：`sel(age = 20) = 1 / NDV(age)`。若 `NDV(age) = 50`，表有 10 万行，估计剩 `10^5 / 50 = 2000` 行；
- 范围条件：`sel(age BETWEEN 18 AND 22)`，若年龄均匀分布在 0~100，约为 `5/100 = 0.05`，估计剩 5000 行；
- 合取条件：假设各谓词独立，`sel(p ∧ q) = sel(p) × sel(q)`。

直方图解决的是均匀假设失效的场景：如果 90% 的学生都是 20 岁，`age = 20` 的真实选择率是 0.9 而不是 0.02，靠 NDV 会低估 45 倍。直方图记录每个值/区间的频次，让估计贴近真实分布。

### 4.2 代价模型

有了各算子的估计行数，还要一个公式把计划折算成一个数字。教科书式的代价模型：

```text
cost = w_io × 页 I/O 数 + w_cpu × 处理的行数
```

以"`age = 20` 是否走索引"为例（表 10 万行，约 1300 个数据页）：

- 全表扫描：读 1300 页，CPU 过滤 10 万行；
- 索引扫描：B+ 树下探约 3~4 页，加上回表取每个匹配行。若选择率 0.02（2000 行），回表最坏 2000 次随机页访问——**比全表扫描还贵**；若选择率 0.0001（10 行），索引扫描只花十几页，远胜全表。

结论：**"有索引就走索引"是错的**，选择率高（匹配行多）时全表扫描的顺序 I/O 反而便宜。这正是必须靠代价模型定量判断、不能靠拍脑袋规则的地方。

### 4.3 join order 枚举：左深树、稠密树与动态规划

三表以上的连接，连接顺序剧烈影响代价。沿用 1.2 节风格的数字：表 A、B 各 1000 行，表 C 有 10^6 行；`A ⋈ B` 的结果只有 10 行（高度选择），而 `A ⋈ C`、`B ⋈ C` 的结果都是 10^6 行。用嵌套循环、代价按"外层行数 × 内层行数"的元组比较次数粗算：

- `(A ⋈ B) ⋈ C`：`1000×1000 + 10×10^6 = 1.1×10^7`；
- `(A ⋈ C) ⋈ B`：`1000×10^6 + 10^6×1000 = 2×10^9`。

**同样的三表连接，只是顺序不同，代价差约 180 倍**——而且胜出的方案是让中间结果最小的那个，不是让"小表先做"那么简单。

枚举所有顺序的搜索空间有多大？

- **左深树（left-deep）**：右子树永远是基表，形如 `((A⋈B)⋈C)⋈D`。顺序数 = 表的排列数 `n!`（n=10 时是 362 万）。
- **稠密树（bushy）**：左右子树都可以是连接结果，形如 `(A⋈B)⋈(C⋈D)`。数量是 Catalan 树形 × 叶子排列，比 `n!` 还多一个指数级因子。

n 稍大就不能蛮力。System R（1979 年，Selinger 等人的经典论文）给出的答案是**动态规划**，直觉是：

> 最优计划里，任何一个子连接的子计划，必定也是该子连接的最优计划——否则换掉它整体更优。

于是按集合大小递推：先算出每个两表连接 `{A,B}` 的最优代价并记下来，三表连接 `{A,B,C}` 的最优代价 = 枚举拆法（如 `{A,B} ⋈ {C}`），用已缓存的两表最优代价加上本次连接的代价。n 张表只有 `2^n` 个子集，DP 把 `n!` 级别的空间压到 `O(2^n)` 个子问题（n=10 时是 1024），总转移约 `O(3^n)`。实践中优化器还常只枚举左深树（空间更小、且便于利用索引嵌套循环连接），进一步压缩搜索量。

```text
DP 表（以 3 表为例）：
  子集 {A} {B} {C}          → 代价 = 扫描代价
  子集 {A,B} {A,C} {B,C}    → 由单表子集 + 连接代价推出
  子集 {A,B,C}              → min{ {A,B}⋈C, {A,C}⋈B, {B,C}⋈A }
```

### 4.4 为什么 MiniOB 没做 CBO

看 `src/observer/sql/optimizer/optimize_stage.cpp` 里 `OptimizeStage::optimize` 的实现——函数体是空的，直接 `return RC::SUCCESS`，注释写明"当前没有成熟的基于代价优化器（CBO）……MiniOB 主要依赖规则与物理计划生成器中的启发式选择"。原因很务实：

- MiniOB 是教学项目，存储层没有收集行数、NDV、直方图的机制（没有 `ANALYZE TABLE` 之类的入口），CBO 没有数据可吃；
- 赛题的重点在算子实现（连接、排序、向量索引等），优化器只需保证计划结构正确、常见模式被改写；
- 空函数本身是个清晰的教学锚点：它标出了 CBO 应该插入的位置——逻辑计划重写之后、物理计划生成之前。

## 5. MiniOB 的优化器实现

### 5.1 OptimizeStage：三步流水线

`src/observer/sql/optimizer/optimize_stage.cpp` 的 `OptimizeStage::handle_request` 把整个优化阶段组织成三步：

```cpp
rc = create_logical_plan(sql_event, logical_operator);   // 1. Stmt -> 逻辑计划
rc = rewrite(logical_operator);                          // 2. 规则重写到不动点
rc = optimize(logical_operator);                         // 3. CBO 占位（空实现）
rc = generate_physical_plan(logical_operator, physical_operator, ...);  // 4. 物理计划
```

（源码中 `optimize` 是独立一步，故严格说是"逻辑计划 → 重写 → （空）代价优化 → 物理计划"四段，口头常合并为三步。）

- `create_logical_plan`：`Stmt` 为空的命令（如 `CREATE TABLE`）返回 `RC::UNIMPLEMENTED`，由 `ExecuteStage` 直接执行；查询类语句交给 `LogicalPlanGenerator`。
- `rewrite`：一个 do-while 循环，反复调 `Rewriter::rewrite` 直到 `change_made` 为 false——正是 3.6 节的不动点。源码注释明确要求"每条 rewrite rule 都能收敛，否则会在这里形成无限循环"。
- `generate_physical_plan`：按会话的执行模式二选一——`CHUNK_ITERATOR`（向量化批量执行）且算子支持时走 `PhysicalPlanGenerator::create_vec`，否则走火山模型逐行迭代的 `create`。

### 5.2 逻辑计划生成：按 SQL 子句叠算子

`src/observer/sql/optimizer/logical_plan_generator.cpp` 的 `create_single_select_plan` 是自底向上"叠"出逻辑树的：

1. FROM 的每张表生成 `TableGetLogicalOperator`（`READ_ONLY`），多表用二叉 `JoinLogicalOperator` **按 FROM 书写顺序左深串起来**——不做任何 join 重排；
2. WHERE 的条件包成一个 AND 的 `ConjunctionExpr`，生成 `PredicateLogicalOperator`；
3. 有聚合/GROUP BY 时叠 `GroupByLogicalOperator`（还会校验"非聚合列必须出现在 GROUP BY 中"）；
4. HAVING 再叠一个 `PredicateLogicalOperator`；
5. 有 ORDER BY 叠 `OrderByLogicalOperator`，有 LIMIT 叠 `LimitLogicalOperator`；
6. 最顶层永远是 `ProjectLogicalOperator`。

`SELECT s.name FROM student s, sc c WHERE s.id=c.sid AND s.age=20` 生成的初始逻辑树：

```text
PROJECT(s.name)
└─PREDICATE(s.id = c.sid AND s.age = 20)
  └─JOIN
    ├─TABLE_GET(s)
    └─TABLE_GET(c)
```

UNION 在 `create_plan(SelectStmt*)` 开头单独处理：各分支独立生成计划后，用 `UnionLogicalOperator`（带 `union_all` 标志）从左到右两两合并。

### 5.3 Rewriter：四条规则的装配

`src/observer/sql/optimizer/rewriter.cpp` 的构造函数按顺序注册了四条规则：

```cpp
rewrite_rules_.emplace_back(new ExpressionRewriter);
rewrite_rules_.emplace_back(new PredicateRewriteRule);
rewrite_rules_.emplace_back(new PredicatePushdownRewriter);
rewrite_rules_.emplace_back(new VectorIndexScanRewrite);
```

`Rewriter::rewrite` 先把四条规则依次作用于当前节点，再递归处理所有子节点；任何一条规则报告 `sub_change_made`，外层不动点循环就会再来一轮。规则接口极简（`src/observer/sql/optimizer/rewrite_rule.h`）：`rewrite(oper, change_made)`，规则只负责"匹配模式就改写并置标志"。

### 5.4 规则一：ExpressionRewriter——表达式级简化

`src/observer/sql/optimizer/expression_rewriter.cpp` 自己不直接改算子树，而是遍历每个算子的 `expressions()`，对其中的表达式应用两条子规则，再递归子算子：

- **ComparisonSimplificationRule**（`comparison_simplification_rule.cpp`）：对一个比较表达式调 `try_get_value`——若两边都是常量（如 `1 + 1 = 2`），直接算出布尔结果，把整个比较换成 `ValueExpr`。这就是 3.4 节的常量折叠。
- **ConjunctionSimplificationRule**（`conjunction_simplification_rule.cpp`）：对 AND/OR 表达式应用 3.5 节的吸收律——AND 中删掉恒真子项、遇恒假整体置假；OR 中删掉恒假子项、遇恒真整体置真；只剩一个子项时上提替换父表达式。

`rewrite_expression` 再按表达式类型递归（`CAST` 进子表达式，`COMPARISON` 递归左右，`CONJUNCTION` 递归每个子项），保证深层嵌套的常量也能被折叠。

### 5.5 规则二：PredicateRewriteRule——恒真/恒假谓词剪枝

`src/observer/sql/optimizer/predicate_rewrite.cpp` 处理表达式简化之后的算子级收尾。匹配模式：某算子只有一个孩子，孩子是 `PREDICATE`，且其唯一表达式已是常量 `ValueExpr`：

- 常量恒真：谓词过滤等于不过滤，**删掉这个 PREDICATE 节点**，把它的孩子们（孙子节点）直接接到祖父节点上；
- 常量恒假：结果必为空，**直接清空子树**（`child_opers.clear()`），下层扫描根本不会执行。

`WHERE 1 = 0` 之类的查询因此零扫描返回。这条规则是典型的"其他规则喂饭"：它自己不算常量，靠规则一先把比较折叠成 `ValueExpr`，它再收割。

### 5.6 规则三：PredicatePushdownRewriter——谓词下推到 TableGet

`src/observer/sql/optimizer/predicate_pushdown_rewriter.cpp` 实现 3.2 节的谓词下推，但形态很克制：只处理 `PREDICATE → TABLE_GET` 这一个模式（即把单表过滤推进扫描算子），不做跨 JOIN 的谓词推导。

`get_exprs_can_pushdown` 的判定逻辑：

- AND 合取：逐个子项尝试下推，能推的从合取中删除；
- OR 合取：源码注释"或 操作的比较，太复杂，现在不考虑"，整体不动；
- 比较表达式：左右至少一边是 `FIELD`（列），另一边是 `FIELD` 或 `VALUE`（常量）即可下推——对应 `age = 20`、`s.id = c.sid` 这类简单比较；含算术运算的复杂表达式不推。

下推的表达式通过 `TableGetLogicalOperator::set_predicates` 挂到表算子上（`src/observer/sql/operator/table_get_logical_operator.h` 的 `predicates_` 成员），执行时由扫描器边读边过滤。若整个谓词被推空，规则给原 PREDICATE 塞一个 `ValueExpr(true)` 占位——因为框架里节点不便就地删除；这个"恒真谓词"恰好又被规则二在下一轮不动点循环中清除，是规则协作的标准例子。

改写效果：

```text
改写前                          改写后
PREDICATE(age=20 AND id>10)     PREDICATE(TRUE)   ← 下一轮被规则二删掉
└─TABLE_GET(student)       →    └─TABLE_GET(student)
                                   predicates_: [age=20, id>10]
```

### 5.7 规则四：VectorIndexScanRewrite——为向量检索定制的模式改写

`src/observer/sql/optimizer/vector_index_scan_rewrite.cpp` 是 2025 赛题扩展加入的规则，展示 RBO 的另一面：**不只是化简，还可以识别特定查询模式整体换成专用算法**。

它匹配的模式是 `LIMIT → ORDER_BY → TABLE_GET`，要求：ORDER BY 只有一个键，且该键是向量距离函数（`is_vector_distance_func()`）；排序方向与距离语义兼容——`is_order_compatible` 规定 L2 距离、余弦距离必须升序（越小越近），内积必须降序（越大越近）；距离函数一侧是列、一侧是常量向量；且该列上存在对应距离类型的向量索引（`Table::find_vector_index`，见 `src/observer/storage/table/table.cpp:1741`）。

全部满足时，规则把索引、查询向量、limit 值设置到 `TableGetLogicalOperator` 上（`set_index` / `set_base_vector` / `set_limit`），然后直接用 TABLE_GET 替换掉 `LIMIT + ORDER_BY` 整棵子树——因为 IVF 向量索引本身就按距离从小到大产出前 K 个结果，排序和限量都被索引"吸收"了：

```text
改写前                              改写后
LIMIT(K)                            TABLE_GET(items)
└─ORDER_BY(l2_distance(v, ?) ASC)     index_ = ivfflat_idx
  └─TABLE_GET(items)           →      base_vector_ = 查询向量
                                      limit_ = K
                                      → 物理阶段成为 VECTOR_INDEX_SCAN
```

对照代价：不改写要全表扫描 + 对每行算距离 + 全量排序再取前 K（O(N·d + N log N)，d 是维度）；改写后走 IVF 索引只探查少数簇，代价与子表规模近似无关。这是"模式匹配 → 算法替换"的教科书式 RBO。

### 5.8 访问路径选择：TableScan 还是 IndexScan

逻辑层的 `TABLE_GET` 到物理层要落地成具体扫描方式，决策在 `src/observer/sql/optimizer/physical_plan_generator.cpp` 的 `create_plan(TableGetLogicalOperator&)`：

1. 若 `table_get_oper.is_vector_scan()`（即规则四设置了 `base_vector_`），直接生成 `VectorScanPhysicalOperator`（EXPLAIN 中显示为 `VECTOR_INDEX_SCAN`）；
2. 否则遍历下推来的谓词，找**单列等值**条件（`EQUAL_TO`，一边 `FIELD` 一边 `VALUE`），用 `Table::find_index_by_field` 查该列有没有索引；
3. 找到就生成 `IndexScanPhysicalOperator`，左右边界都设为该常量值、均闭区间——即一个点查；剩余谓词继续作为过滤条件挂在算子上；
4. 找不到：普通表生成 `TableScanPhysicalOperator`，视图生成 `ViewScanPhysicalOperator`。

注意 `find_index_by_field`（`src/observer/storage/table/table.cpp:1727`）的实现：线性遍历 `indexes_`，返回**第一个**字段匹配的**单列**索引。也就是说：

- 只认等值，`age > 20` 这类范围条件不会走索引；
- 多列（复合）索引直接跳过；
- 同列有多个索引时先到先得，不比较代价。

这套启发式够用且确定性强，但正是第 8 题 multi-index 要补的短板（见"回到赛题"）。

### 5.9 连接算法选择：另一个启发式

`create_plan(JoinLogicalOperator&)` 里，MiniOB 用"JOIN 树深度"做启发式：递归计算逻辑 JOIN 树的最大深度，深度 ≥ 3（约 4 张表以上）时认为可能遇到大数据量，生成 `GraceHashJoinPhysicalOperator`（每个 join 15MB 内存上限、32 个分区，内存装不下可落盘）；否则生成 `NestedLoopJoinPhysicalOperator`（简单稳定）。

**复杂度小结**：逻辑计划生成是对 `SelectStmt` 的单趟遍历，O(N)；重写每轮 O(N+E)，轮数受规则收敛性约束；物理计划生成是对逻辑树的一次递归，每个节点只做一次局部决策，O(N)。整套流程是"多项式、近线性"的——它没有 DP、没有枚举，这就是放弃 CBO 换来的简单性。

## 6. EXPLAIN：把优化器的决策亮出来

优化器是数据库里最"不透明"的组件，EXPLAIN 是它的观察窗：**不真正执行查询，只展示生成的计划**。用途有三：

- 验证重写是否生效（谓词有没有下推、有没有被折叠）；
- 检查访问路径（走了 `INDEX_SCAN` 还是 `TABLE_SCAN`）；
- 定位慢查询（连接顺序、连接算法是否符合预期）。

MiniOB 中 `EXPLAIN` 本身也是一个语句：`logical_plan_generator.cpp` 的 `create_plan(ExplainStmt*)` 先生成子查询的完整计划，再在外面套一个 `ExplainLogicalOperator`；执行时 `ExplainPhysicalOperator`（`src/observer/sql/operator/explain_physical_operator.cpp`）递归打印物理算子树，表头是 `OPERATOR(NAME)`，节点用 `├─` / `└─` 画树形线，每个节点打印 `name()` 和非空的 `param()`。算子名来自 `physical_operator.cpp` 的 `physical_operator_type_name`（如 `TABLE_SCAN`、`INDEX_SCAN`、`NESTED_LOOP_JOIN`、`ORDER_BY`、`VECTOR_INDEX_SCAN`）；`TableScanPhysicalOperator::param()` 返回表名，`IndexScanPhysicalOperator::param()` 返回 `索引名 ON 表名`。例如对第 1 节的查询，可能看到：

```text
OPERATOR(NAME)
PROJECT
└─NESTED_LOOP_JOIN
  ├─INDEX_SCAN(idx_age ON student)
  └─TABLE_SCAN(sc)
```

工业数据库的 EXPLAIN 信息量大得多：MySQL 的 EXPLAIN 给出 `type`（`ALL`/`range`/`ref`/`eq_ref`/`const` 等访问类型）、`key`（实际选用的索引）、`rows`（估计扫描行数）、`Extra`（如 `Using index condition`）；PostgreSQL 的 `EXPLAIN (ANALYZE, BUFFERS)` 连估计代价、实际执行时间、命中缓冲池的页数都给出来。面试中被问"这条 SQL 为什么慢"，标准动作就是要 EXPLAIN 输出。

## 7. 工业数据库怎么做

- **MySQL（InnoDB）**：CBO 优化器。统计信息由 `ANALYZE TABLE` 采样收集（默认持久化，采样页数由 `innodb_stats_persistent_sample_pages` 控制，默认 20），记录表的行数与每列的基数（NDV 估计）；MySQL 8.0 起支持直方图。代价模型内置在 `server_cost`/`engine_cost` 两张系统表里（区分内存中读页与磁盘读页的代价），join order 用带剪枝的穷举搜索，`optimizer_search_depth` 控制搜索深度。`EXPLAIN` 的 `type` 列从 `const` 到 `ALL` 是面试官最爱问的访问路径阶梯。
- **PostgreSQL**：教科书式 CBO。代价公式与默认常数公开：`cost = seq_page_cost×顺序页 + random_page_cost×随机页 + cpu_tuple_cost×行数 + ...`（默认 1.0 / 4.0 / 0.01）——`random_page_cost` 是顺序页的 4 倍，正是"高选择率时宁可全表扫"的定量化。统计信息存在 `pg_statistic`（高频值 MCV + 直方图），由 `ANALYZE` 或 autovacuum 维护。join 枚举用 System R 式动态规划；表超过 `geqo_threshold`（默认 12）时切换基因算法（GEQO）避免组合爆炸。
- **OceanBase**：完整的"改写 + 代价"两级优化器。改写层远比 MiniOB 丰富：子查询展开（unnesting）、外连接简化、谓词移动（predicate move-around）、视图合并等；代价层依赖自动收集的统计信息（也可用 `DBMS_STATS` 手动收集），并额外考虑分布式因素——分区裁剪（partition pruning）、数据分布与并行度。另有计划缓存（plan cache）：同模板 SQL 复用已生成的计划，避免重复优化。

对照之下，MiniOB 优化器是一个"骨架完整、血肉从简"的教学版：三级流水线（逻辑 → 重写 → 物理）与工业数据库同构，重写规则是真实可跑的，但统计信息、代价模型、join 枚举整体留空。

## 小结

- SQL 是声明式的，同一查询的执行代价可差几个数量级，优化器负责在等价计划中选便宜的。
- 逻辑计划是关系代数树（σ/π/⋈/∪/γ），只描述"做什么"；物理计划决定"怎么做"（扫描方式、连接算法）。
- RBO 是模式匹配式等价改写：谓词下推（行数是最贵的资源，尽早过滤）、投影下推（只搬需要的列）、常量折叠、AND/OR 吸收律；规则要跑到不动点，且每条规则必须收敛。
- CBO 用统计信息（行数/NDV/直方图）估选择率，用代价模型折算计划，用动态规划在 `O(2^n)` 个子问题里枚举 join order（左深树空间 n!，稠密树更大）。
- MiniOB 的 `OptimizeStage`：逻辑计划生成 → `Rewriter` 不动点循环（ExpressionRewriter、PredicateRewriteRule、PredicatePushdownRewriter、VectorIndexScanRewrite 四条规则）→ 空的 `optimize()` → 物理计划生成。访问路径是启发式：单列等值有索引就 `INDEX_SCAN`，否则 `TABLE_SCAN`；连接按 JOIN 深度在嵌套循环与 Grace Hash Join 之间切换。
- EXPLAIN 是观察优化器决策的窗口；工业数据库（MySQL/PostgreSQL/OceanBase）都是"改写 + 代价"两级，MiniOB 保留了骨架、省略了代价。

## 面试追问

**问：RBO 和 CBO 的区别是什么？各自的优缺点？**

答：RBO 按预设规则做等价改写，不看数据分布，优点是稳定、可预期、实现简单、优化本身开销小；缺点是"一刀切"——比如无脑走索引，在选择率很高时反而比全表扫描慢。CBO 用统计信息估算每个候选计划的代价再选最优，能随数据分布自适应；缺点是需要维护统计信息（过期会选错计划），优化本身有开销，结果不易预测。工业数据库以 CBO 为主，但保留 RBO 做前置改写，缩小代价枚举的搜索空间。

**问：谓词下推为什么几乎总是有效？什么情况下不能下推？**

答：数据库一切代价（I/O、CPU、内存）都随行数增长，过滤越早，上层算子处理的行数越少，下游的连接、排序、聚合全部受益。不能下推的典型情况：谓词涉及多张表（只能当连接条件）；OUTER JOIN 中对保留侧的过滤会改变语义（ON 和 WHERE 在 LEFT JOIN 下不等价）；含易变函数或子查询的谓词。MiniOB 的实现更保守：只处理 `PREDICATE → TABLE_GET` 模式，OR 条件整体不下推，复杂表达式不下推（见 `predicate_pushdown_rewriter.cpp` 的 `get_exprs_can_pushdown`）。

**问：MiniOB 的重写为什么要循环到不动点？举一个规则互相触发的例子。**

答：单条规则一次只能改一处，而改写会互相创造匹配机会，所以 `OptimizeStage::rewrite` 用 do-while 反复执行全部规则，直到一轮下来 `change_made` 为 false。例子：`PredicatePushdownRewriter` 把谓词全部下推后，会在原 PREDICATE 里塞一个 `ValueExpr(true)` 占位；下一轮 `PredicateRewriteRule` 识别到"恒真谓词"，把整个 PREDICATE 节点从树上摘掉。再如 `ComparisonSimplificationRule` 把 `1+1=2` 折叠成常量 TRUE，`ConjunctionSimplificationRule` 随之把 `x>5 AND TRUE` 化简为 `x>5`，下推规则才得以把它推进 TableGet。前提是所有规则收敛，否则死循环。

**问：join order 为什么重要？左深树和稠密树有什么区别？动态规划怎么枚举？**

答：不同连接顺序的中间结果大小差异巨大，代价可差几个数量级。左深树要求每个连接的右侧是基表，n 张表有 n! 种顺序；稠密树允许两侧都是中间结果，空间更大（Catalan 数级别）。动态规划利用最优子结构——最优计划中的子计划必是最优的——按子集大小递推：先缓存所有两表连接的最优代价，再组合出三表、四表……子问题数从 n! 降到 2^n。实践中常只枚举左深树加索引嵌套循环，进一步压缩空间；PostgreSQL 表数超过 geqo_threshold（默认 12）时还会切换基因算法。

**问：EXPLAIN 里看到全表扫描（TABLE_SCAN / ALL），一定是坏事吗？**

答：不一定。两种情况全表扫描反而最优：一是表很小（几页数据，索引下探的开销都省不回来）；二是选择率很高（谓词匹配大比例的行），索引扫描要回表逐行随机读，代价超过顺序全表扫——PostgreSQL 里 `random_page_cost` 默认是 `seq_page_cost` 的 4 倍，定量反映这一点。只有当谓词选择率低、表又大，EXPLAIN 却显示全表扫时，才说明缺索引或统计信息过期。MiniOB 没有代价模型，选择是启发式的：有单列等值索引就走 `INDEX_SCAN`，不看选择率。

**问：统计信息过期会发生什么？怎么解决？**

答：优化器按过期统计估算选择率和行数，可能严重误判——例如表已从 1 万行涨到 1000 万行，统计还是旧的，优化器仍按小表选嵌套循环连接或误判索引扫描便宜，导致计划劣化。解法：`ANALYZE TABLE`（MySQL）/ `ANALYZE`（PostgreSQL）手动刷新；依赖自动收集（autovacuum、OceanBase 的定时统计任务）；对分布不均的列建直方图。面试可补一句：这也是"同一条 SQL 昨天快今天慢"的最高频原因之一。

## 回到赛题

- **第 5 题 join-tables**（[赛题 1–8 解析](../02_problems_01_08.md)）：多表连接的逻辑计划由 `create_single_select_plan` 按 FROM 顺序左深拼接，MiniOB 不做 join 重排；物理层用 JOIN 树深度在 `NestedLoopJoinPhysicalOperator` 与 `GraceHashJoinPhysicalOperator` 之间启发式切换（深度 ≥ 3 走哈希连接，见 `physical_plan_generator.cpp` 的 `create_plan(JoinLogicalOperator&)`）。面试问"为什么不做 join order 优化"，就回到本章 4.3/4.4：没有统计信息，枚举没有意义。
- **第 15 题 order-by**（[赛题 9–16 解析](../03_problems_09_16.md)）：`OrderByLogicalOperator` 在逻辑树中是一个显式排序节点，物理层总是物化排序，没有"利用 B+ 树索引序免排序"的优化。对照本章 5.7：`VectorIndexScanRewrite` 能消掉 ORDER BY，恰恰是因为向量索引的输出序天然满足排序要求——"索引序换排序"正是这条规则的推广形式。
- **第 18 题 vector-search**（[赛题 17–24 解析](../04_problems_17_24.md)）：`VectorIndexScanRewrite` 是本章 RBO 的活样本：模式匹配 `LIMIT → ORDER_BY → TABLE_GET`，校验距离函数与排序方向（L2/余弦升序、内积降序）、确认 IVF 索引存在后整体改写为向量索引扫描，把 O(N·d + N log N) 的"全表算距离再排序"换成索引级近邻搜索。
- **第 8 题 multi-index**（[赛题 1–8 解析](../02_problems_01_08.md)）：本章 5.8 的反面教材——`find_index_by_field` 只匹配单列等值、先到先得，范围条件与复合索引用不上，多个候选索引之间不做代价比较。这题的本质就是给 MiniOB 补上缺失的"访问路径选择"，即在物理计划生成处引入更接近 CBO 的多索引评估。
