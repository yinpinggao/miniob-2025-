# OceanBase 2025 初赛 MiniOB 源码与推免面试指南

这套文档面向两个目标：

1. 从数据库基础开始，理解 `miniob_2025_problems.md` 中 24 道赛题在当前项目里的实现方式。
2. 面对推免面试追问时，不只会说“代码在哪”，还能够说明执行链、数据结构、算法复杂度、设计取舍和当前实现的边界。

原始赛题见：[miniob_2025_problems.md](../../../miniob_2025_problems.md)。

> 审计基线：2026-08-11，源码基于 `main` 分支提交 `2e609ac529`。后续若代码继续修改，应重新核对“当前实现”和实测结论。

> 阅读原则：源码中出现某个类，不等于功能已经完整实现。本文会区分“核心链路可用”“部分实现”和“赛题要求未完整满足”，并记录通过实际 SQL 验证出的行为。

本文统一使用三种口径，避免把不同层次混在一起：

- **题面要求**：以 `miniob_2025_problems.md` 的 24 题正文和官方附录为准；
- **当前实现**：只描述本仓库当前源码实际能走通的路径；
- **扩展实现**：题面没有要求、但仓库额外加入的功能，例如 IVF 向量索引。扩展功能存在不代表对应赛题完成度更高。

## 赛题口径校准

有几处题面本身容易误读，读源码前应先统一口径：

| 题目 | 应采用的口径 |
|---|---|
| 2. update | 题面只要求单字段 UPDATE；当前 parser 虽扩展到多个 SET，但这属于额外能力，且逐行表达式求值仍有 bug。 |
| 7. function | 文字说非法格式符“原样输出”，但示例把 `%z` 输出为 `z`。当前实现遵循示例，即去掉 `%`。面试时应主动说明这个题面歧义。 |
| 10. group-by | 题面前半段把 `SELECT COUNT(id) ... GROUP BY name HAVING ...` 列入 FAILURE 示例，后半段又把同一句作为必须支持的示例，前后矛盾。结合“必须支持 GROUP BY/HAVING”的明确要求，应按后半段处理：该 SQL 应成功。 |
| 16. vector-basic | 官方附录要求无括号 `VECTOR` 默认 2048 维、最大 16383 维，并接受 `TO_VECTOR/FROM_VECTOR/VECTOR_DISTANCE` 同义词；当前源码未完整满足这些细节。 |
| 18. vector-search | 正文只要求**无索引精确检索**。IVF/ANN 是仓库额外扩展，评估第 18 题时应先看全表扫描、距离计算、ORDER BY、LIMIT 是否正确。 |
| 22. create-view | 可更新性不能只看“单表/多表”，应严格按题面附录中的 INSERT/UPDATE/DELETE 映射规则判断。 |
| 24. big-order-by | 核心要求是 350 MB 限制下的大结果排序。Grace Hash Join 是为降低连接中间结果压力而加入的实现选择，不是题面单独要求。 |

## 文档导航

| 建议顺序 | 文档 | 内容 |
|---|---|---|
| 1 | [项目架构与一条 SQL 的完整执行链](01_project_architecture.md) | Parser、Resolver、逻辑计划、物理计划、火山模型、存储与事务 |
| 2 | [赛题 1—8：基础 SQL、类型、连接、表达式与复合索引](02_problems_01_08.md) | basic、update、drop-table、date、join、expression、function、multi-index |
| 3 | [赛题 9—16：唯一性、聚合、子查询、NULL、集合、排序与向量](03_problems_09_16.md) | unique、group-by、sub-query、alias、null、union、order-by、vector-basic |
| 4 | [赛题 17—24：TEXT、检索、ALTER、MVCC、视图与大数据算子](04_problems_17_24.md) | text、vector-search、alter、update-mvcc、complex-sub-query、view、全文、大排序 |
| 5 | [推免面试拷打题库](05_interview_drill.md) | 项目介绍模板、高频追问、回答框架、易踩雷说法 |
| 6 | [24 题源码索引与实测证据](06_source_and_test_index.md) | 每题关键文件、构建状态、代表性 SQL 与实测结果 |

## 推荐学习顺序

如果你几乎没有数据库基础，建议按下面顺序学习：

```text
第一遍：只读 01，先建立“一条 SQL 怎么跑”的整体地图
第二遍：按 02 → 03 → 04 顺序理解 24 题
第三遍：对照 06，从 SQL 一路追到存储层源码
第四遍：使用 05 进行不看资料的口述和模拟追问
```

每道题都建议按固定的八步法复述：

```text
1. 题目解决什么问题
2. 必要的数据库原理
3. SQL 从解析到执行经过哪些类
4. 核心数据结构是什么
5. 算法如何运行
6. 时间和空间复杂度如何
7. 当前实现有哪些简化或缺陷
8. 如果做成工业数据库，应如何改进
```

## 24 题实现状态速览

| 题号 | 赛题 | 当前源码结论 |
|---:|---|---|
| 1 | basic | 基础 SQL 和存储主链路可用，优化器仍以简单规则为主 |
| 2 | update | 主链路存在；多行 `SET c=c+1` 错误复用第一行计算结果 |
| 3 | drop-table | 能删除表及文件；DDL 文件操作不具备事务原子性 |
| 4 | date | `YYYYMMDD` 四字节存储和闰年校验可用；尾随字符校验宽松 |
| 5 | join-tables | 左深树和 Nested Loop Join 可用；混合 JOIN 语法及别名不完整 |
| 6 | expression | 标量表达式较完整；向量化除零与 NULL 语义不一致 |
| 7 | function | LENGTH、ROUND、DATE_FORMAT 主路径可用；早期年份 `%y` 可抛异常终止进程，部分语义与 MySQL 不同 |
| 8 | multi-index | 复合索引能创建和维护，但查询计划不会真正使用 |
| 9 | unique | 单事务主路径可用；MVCC 并发唯一性和 NULL comparator 有缺陷 |
| 10 | group-by | 聚合主链路可用；“Hash” 分组实际顺序查找，NULL 会与 0 混组 |
| 11 | simple-sub-query | scalar、IN、EXISTS 可执行；NOT IN 的首个 RHS NULL 崩溃与三值过滤已修复，子查询仍存在 MVCC 可见性问题 |
| 12 | alias | 基本表/列别名可用；遮蔽、歧义诊断和视图别名有边界 |
| 13 | null | 有物理 NULL 标志和简化语义；没有完整 SQL 三值逻辑 |
| 14 | union | UNION ALL 流式、UNION 哈希去重；NULL/浮点 hash-equality 有风险 |
| 15 | order-by | 内存排序和 LIMIT 可用；NULL 顺序、稳定性及 reopen 状态有问题 |
| 16 | vector-basic | `VECTOR(n)`、转换和距离主路径可用；默认维度、最大维度、同义词和比较限制不符合官方附录 |
| 17 | text | 128 KiB 页上的 65535 字节定长内联实现，不是通用 LOB |
| 18 | vector-search | 题面要求的无索引精确全扫路径成立；额外 IVF 路径仍有 DOT 顺序、过滤后 top-k 和 reopen 状态问题 |
| 19 | alter | ADD/DROP/CHANGE/RENAME 主流程存在；数据、元数据、索引切换非原子 |
| 20 | update-mvcc | 旧版失效、新版插入、提交/回滚链路存在；缺 UPDATE 日志、GC |
| 21 | complex-sub-query | 相关子查询可执行；没有去相关优化，NULL/MVCC 问题延续 |
| 22 | create-view | 查询视图可用；复杂视图及 MVCC 下 DML 路由不可靠 |
| 23 | full-text-index | jieba、倒排结构和 BM25 存在；查询仍全表评分，删除/MVCC 污染统计 |
| 24 | big-order-by | External Sort 主体存在；Grace Hash Join 实际退化为落盘 Nested Loop |

## 面试时最重要的态度

不要把项目说成“工业数据库”，也不要隐瞒源码缺陷。更稳妥的表达是：

> 这是一个教学数据库，我完成并分析了从 SQL 前端、执行器到存储和事务的一整条链路。部分功能实现了比赛要求的主路径，但在并发控制、异常恢复、NULL 三值逻辑和代价优化上仍有明显简化。我不仅能够定位代码，也能解释这些简化为什么会出错，以及进一步的工程改进方向。
