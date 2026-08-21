# 数据库内核知识教材（配合 MiniOB 源码）

这套教材写给"几乎没有数据库基础、准备拿 MiniOB 项目参加预推免面试"的同学。它回答的问题是上层那套[面试指南](../README.md)刻意略去的部分：**在谈某道题"怎么实现的"之前，你需要先懂的通用数据库内核知识**。

每章的写法统一为：直觉类比 → 严格概念 → 可以跟着算的小例子 → 一般原理 → MiniOB 源码实现（真实文件路径与类名）→ 复杂度 → 工业数据库对比 → 面试追问 → 回到赛题。

与上层文档的分工：

- **本目录（kernel/）**：通用原理，像教材一样系统讲，不绑定某一道题；
- **[上层 01](../01_project_architecture.md)**：MiniOB 的架构与一条 SQL 的执行链，把各章知识在项目里串一遍；
- **[上层 02-04](../02_problems_01_08.md)**：24 道赛题逐题解析，含执行链、数据结构、取舍与当前实现缺陷；
- **[上层 05](../05_interview_drill.md) / [06](../06_source_and_test_index.md)**：面试拷打题库与源码实测索引。

## 章节目录

| 章 | 标题 | 一句话内容 | 主要支撑的赛题 |
|---|---|---|---|
| 00 | [数据库内核全景](00_overview.md) | 为什么需要 DBMS、关系模型、ACID、分层架构、学习路线 | 全部 |
| 01 | [磁盘、页与 Buffer Pool](01_buffer_pool.md) | 页式存储、pin/unpin、LRU 淘汰、double write | 17、24 |
| 02 | [堆文件、RID 与行列存储](02_record_layout.md) | 定长记录、页内组织、行存/PAX、NULL 物理表示 | 13、17、24 |
| 03 | [索引与 B+ 树](03_bplus_tree.md) | B+ 树结构与操作、复合索引与最左前缀、crabbing 并发 | 1、8、9 |
| 04 | [SQL 编译](04_sql_frontend.md) | flex/bison、AST、语义绑定、表达式与类型系统 | 4、5、6、7、12 |
| 05 | [查询执行](05_execution_model.md) | 火山模型、阻塞算子与物化、向量化执行 | 1、15、24 |
| 06 | [查询优化](06_query_optimizer.md) | 关系代数、RBO 重写规则、CBO 概念、访问路径选择 | 5、8、15、18 |
| 07 | [连接算法](07_join_algorithms.md) | Nested Loop / Sort-Merge / Hash Join / Grace Hash Join | 5、24 |
| 08 | [排序与聚合](08_sort_and_aggregate.md) | 外部排序、k 路归并、Hash Group By、HAVING | 10、15、24 |
| 09 | [事务与 MVCC](09_transaction_mvcc.md) | ACID、隔离级别、2PL、MVCC 可见性推演 | 2、9、20 |
| 10 | [日志与崩溃恢复](10_logging_recovery.md) | WAL、LSN、group commit、checkpoint、ARIES | 3、19、20 |
| 11 | [全文检索与 BM25](11_fulltext_bm25.md) | 分词、倒排索引、BM25 公式逐项推导 | 23 |
| 12 | [向量检索](12_vector_search.md) | 距离度量、精确检索 top-k、IVF/HNSW 近似最近邻 | 16、18 |

## 推荐阅读顺序

完全零基础：

```text
00 → 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 → 11 → 12
```

时间紧、按面试频率优先：

```text
00（全景）→ 03（B+ 树）→ 09（MVCC）→ 05（火山模型）→ 08（排序聚合）→ 06（优化器）→ 其余
```

每读完一章，去上层对应的赛题解析里做一次"知识 → 代码"的对照，最后用 [05_interview_drill.md](../05_interview_drill.md) 的自测九问合上书本口述。
