# 推免面试拷打题库

## 1. 60 秒项目介绍模板

> 这个项目基于 MiniOB 教学数据库，功能覆盖 SQL 解析和表达式绑定、逻辑和物理计划、火山模型执行器、B+Tree 索引、MVCC、多表连接、聚合、子查询、视图、向量检索、全文检索和外部排序。一条 SELECT 会经过 Parser、Resolver、Logical Planner、Optimizer 和 Physical Planner，最后由 SqlResult 以 open-next-close 方式驱动物理算子。项目完成了多数赛题的核心链路，但我在源码和 SQL 验证中也发现了复合索引无法用于查询、UPDATE 表达式没有逐行计算、GROUP BY 的 NULL 比较错误，以及 Grace Hash Join 实际退化等问题。我能够说明这些问题的根因，以及向工业实现演进需要补充的事务、优化器和恢复机制。

不要机械背诵。至少要能顺着老师打断的位置继续展开。

## 2. 三分钟讲解框架

如果老师让你详细介绍，可以按四层展开：

### 第一层：SQL 前端

```text
SQL 字符串 → AST/ParsedSqlNode → Stmt/绑定后的表达式
```

说明 parser 只管语法，binder 才确认表、字段、作用域和类型。

### 第二层：计划与执行

```text
Stmt → Logical Operator → Rewrite → Physical Operator
```

说明逻辑计划描述“做什么”，物理计划决定“用什么算法做”。查询采用火山模型。

### 第三层：存储与索引

```text
Tuple → Record → RID → Page
field key → B+Tree → RID → 回表
```

说明索引只给出候选 RID，MVCC 下还要回表做可见性判断。

### 第四层：你的深入分析

选择两个你最熟的功能展开，例如：

- MVCC UPDATE 的旧版本结束、新版本插入；
- External Sort 的 run 生成和 k 路归并；
- 全文倒排结构和 BM25；
- 复合索引和最左前缀。

最后主动说一个缺陷及改进方案，证明你不是停留在调用链层面。

## 3. 高频追问与参考回答

### Q1：为什么查询结果不是 ExecuteStage 一次性计算出来的？

因为 SELECT 被编译成物理算子树。`SqlResult` 对外提供 `open/next_tuple/next_chunk/close`；其 `next_tuple` 内部再调用物理算子的 `next/current_tuple`。ExecuteStage 更多负责分派或直接执行 DDL 等不产生行流的语句。

### Q2：火山模型有什么优缺点？

优点是接口统一、算子易组合、部分算子能够流水执行。缺点是逐 tuple 虚函数调用开销较大，排序、聚合等阻塞算子仍需大量内存；所有算子还必须正确重置 reopen 状态。

### Q3：逻辑计划和物理计划的区别？

逻辑计划描述关系代数语义，例如过滤、连接、聚合。物理计划指定算法，例如 TableScan 还是 IndexScan、Nested Loop 还是 Hash Join、内存排序还是外部排序。

### Q4：为什么 UPDATE 要先收集目标记录？

为了避免 Halloween Problem。若一边扫描索引一边更新索引字段，记录可能移动到尚未扫描的位置并被再次更新。先 materialize 目标 RID/Record，再更新可以固定目标集合。

### Q5：当前 UPDATE 的正确性问题是什么？

`SET` 表达式只在第一条记录上求值，然后把结果应用到所有目标行。正确实现必须为每一条旧记录重新设置 tuple 并计算表达式。

### Q6：B+Tree 为什么适合数据库索引？

B+Tree 分支因子大、树高低，节点大小可与磁盘页匹配；叶子有序，既支持等值查询，也支持范围扫描。相比二叉树，每次查询需要的随机 I/O 更少。

### Q7：复合索引 `(a,b)` 为什么不能只用 `b`？

索引按 `(a,b)` 字典序排列。不同 `a` 下的 `b=常量` 分散在整棵树中，不能构成一个连续范围。若先限定 `a`，相同 `a` 的记录连续，才能进一步使用 `b`。

### Q8：复合索引 key 后为什么还要拼 RID？

普通索引允许不同记录具有相同用户 key。RID 作为最后一部分可以让 B+Tree entry 唯一，并允许同 key 的多个记录和多个 MVCC 版本共存。

### Q9：唯一索引在 MVCC 下为什么难做？

因为索引中可能存在不可见历史版本和其他事务未提交版本。只做快照可见性检查会让两个并发事务都忽略对方的未提交同 key 记录。需要 key lock、唯一意向记录或提交阶段冲突校验。

### Q10：NULL 为什么不能简单当成 0 或空字符串？

NULL 表示未知或缺失，不是某个普通值。SQL 比较需要三值逻辑，分组时 NULL 与 NULL 属于同一组，但不能和数字 0 合并；唯一性、排序和集合去重还需要各自一致的 NULL 规则。

### Q11：`NOT IN (NULL)` 为什么不是 TRUE？

`x NOT IN (NULL)` 的标准语义是 UNKNOWN，在 WHERE 中应被过滤。项目曾因首个 RHS NULL 未检查而在 `IntegerType::compare` 触发断言；现已改为先记录 RHS 是否含 NULL，只比较非 NULL 值，无匹配且含 NULL 时以 false 实现 predicate 的 UNKNOWN 过滤。

### Q12：为什么 Hash Group By 通常是 O(N)？

每条输入记录计算 group key 的 hash，通过哈希表平均 O(1) 找到对应聚合状态，所以总体平均 O(N)。当前项目顺序遍历已有组，最坏接近 O(N²)。

### Q13：COUNT(*) 和 COUNT(column) 有何区别？

COUNT(*) 统计行数，包括列值为 NULL 的行；COUNT(column) 只统计该表达式不为 NULL 的行。项目中 COUNT(*) 通过聚合常量 1 实现。

### Q14：相关子查询为什么慢？

它是一个参数化子计划。当前实现对每一条外层 tuple 都重新 open、扫描和 close 内层计划，复杂度约为外层行数乘以内层成本。成熟优化器会尝试改写成 Semi Join、Anti Join 或普通 Join。

### Q15：UNION 和 UNION ALL 的区别？

UNION ALL 直接拼接结果，可以流式输出；UNION 还要按整行去重，需要 hash set 或排序，消耗额外内存和 CPU。

### Q16：为什么浮点比较和 hash 必须一致？

哈希容器要求“相等的对象必须具有相同 hash”。如果比较使用 epsilon 容差，而 hash 使用精确浮点值，两个被认为相等的值可能进入不同 bucket，导致去重失败。

### Q17：External Sort 如何工作？

内存只能容纳一部分数据时，先把每个内存块排序并写成 run 文件，再使用最小堆进行 k 路归并。若 run 太多，还需要限制 fan-in 并进行多轮 merge。

### Q18：External Sort 的 I/O 复杂度如何理解？

每一轮会顺序读写全部数据。若内存可产生 `M` 个输入页规模的 run、每次归并 `F` 路，归并轮数约为 `log_F(number_of_runs)`，总 I/O 是数据量乘以轮数的量级。

### Q19：真正的 Grace Hash Join 是什么？

对左右输入使用相同的 join-key hash 分区，使可能连接的记录进入同编号分区；随后逐分区把较小一侧装入内存建立 hash table。当前项目拿不到 join key，所有 tuple hash 到 0，所以并未获得该算法的复杂度优势。

### Q20：MVCC UPDATE 为什么不原地覆盖？

原地覆盖会让旧快照看不到旧值。MVCC 让旧版本的可见区间结束，并插入一个新版本，使不同快照根据 begin/end xid 读取相应版本。

### Q21：MVCC 为什么需要垃圾回收？

当没有任何活跃快照需要旧版本时，应删除历史物理记录及其索引 entry。否则表、索引和 Buffer Pool 工作集会持续膨胀，扫描和维护成本越来越高。

### Q22：ALTER TABLE 为什么难保证原子性？

它可能同时切换新数据文件、新元数据和多个索引文件。任一阶段崩溃都会产生版本不一致。需要 DDL journal、phase marker、原子 rename 边界和恢复流程。

### Q23：普通视图和物化视图有什么区别？

普通视图保存查询定义，每次查询时重新执行；物化视图保存查询结果，需要解决刷新、一致性和增量维护。当前项目属于普通逻辑视图。

### Q24：为什么可更新视图很难？

必须判断每个输出列能否唯一映射回基表字段，并保存基表 RID。聚合、DISTINCT、表达式、多表连接通常不具有唯一逆映射，还要保证事务原子性和 WITH CHECK OPTION 等规则。

### Q25：全文索引的核心结构是什么？

倒排索引，即 `token → posting list(RID, term frequency)`，同时维护文档长度等统计信息。查询应从 posting list 得到候选文档，而不是扫描全表。

### Q26：BM25 比简单词频好在哪里？

它同时考虑词在当前文档中的频率、词在整个语料中的稀有程度和文档长度，并通过饱和函数避免词频无限线性增加得分。

### Q27：精确向量检索和 ANN 有何区别？

精确检索计算所有向量，保证 top-k；ANN 只搜索部分候选，牺牲 recall 换取速度。IVF 先把向量聚类到多个 list，查询只探测距离最近的若干 list。

### Q28：TEXT 为什么不应该总是内联？

定长内联会让短文本也占用最大空间，降低页内记录数。工业实现通常在主记录里保存长度和 LOB locator，把大对象存到 overflow pages 或独立文件。

### Q29：这个项目最严重的三类问题是什么？

可以回答：

1. 语义一致性：NULL、NOT IN、浮点 hash/equality、逐行 UPDATE。
2. 事务恢复：ALTER、视图 DML、MVCC UPDATE 日志和唯一键并发。
3. 算法名实不符：HashGroupBy 顺序查找、GraceHashJoin 全部 hash 到 0、全文索引没有驱动候选扫描。

### Q30：如果只给一周时间，你会先修什么？

建议按正确性优先：

1. 修复 `Value` 的 NULL 比较、hash 和排序契约。
2. 修复 UPDATE 对每行重新求值。
3. 给 NOT IN、子查询传递事务和三值逻辑。
4. 补复合索引最左前缀访问路径。
5. 再优化 Hash Aggregate、Hash Join 和全文 top-k。

## 4. 容易被老师抓住的错误说法

不要说：

> 我们实现了 Hash Group By，所以分组复杂度是 O(N)。

正确说法：类名叫 HashGroupBy，但源码使用 `groups_` 顺序查找，最坏 O(N²)。

不要说：

> 有 GraceHashJoin 类，所以四表连接使用了 Grace Hash Join。

正确说法：物理计划会选择该类，但 hash key 没有传入，`hash_tuple()` 固定为 0，实际退化。

不要说：

> 支持创建复合索引，因此查询会使用复合索引。

正确说法：存储和维护已实现，优化器和 IndexScan key 构造没有实现。

不要说：

> TEXT 已经实现了跨页大对象。

正确说法：页被扩大到 128 KiB，TEXT 仍是 65535 字节定长内联。

不要说：

> 全文索引查询不需要扫描表。

正确说法：倒排数据结构存在，但 MATCH 当前仍随表扫描逐行按 RID 计算分数。

不要说：

> `NOT IN (NULL)` 只是三值逻辑未完善，最多多返回几行。

正确说法：这曾是会杀死 observer 的崩溃级 bug，现在已修复首值 NULL 检查和 UNKNOWN 过滤；还应说清修复前的根因与回归用例。

不要说：

> 所有测试都通过。

正确说法：项目完成构建；关闭 LeakSanitizer 后确认 27 项单测通过，最后一项长耗时日志测试未完成；另外做了针对性 CLI SQL 验证。

## 5. 自测方法

对每道题，合上文档后回答：

```text
1. SQL 从哪个 Stmt 进入？
2. 逻辑计划长什么样？
3. 物理算子是哪一个？
4. 最终调用 Table/Index/Trx 的哪个接口？
5. 核心数据结构是什么？
6. 正确性依赖什么不变量？
7. 时间和空间复杂度是什么？
8. 已知最严重的 bug 是什么？
9. 如何改成工业级实现？
```

如果只能回答类名，说明还停留在“源码导航”；能够回答不变量、失败路径和复杂度，才接近面试需要的理解深度。
