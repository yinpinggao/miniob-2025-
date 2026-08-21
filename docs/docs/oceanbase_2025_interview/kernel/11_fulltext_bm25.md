# 全文检索：分词、倒排索引与 BM25

**本章导读**：前面的章节讨论了 B+ 树索引如何加速"等值/范围"查询。但现实中还有一类查询——"找出内容里提到'数据库索引'的文章，并按相关程度排序"——B+ 树无能为力。这就是**全文检索（Full-Text Search）**。MiniOB 2025 赛题 23（full-text-index）要求给数据库加上 `MATCH ... AGAINST` 全文检索能力：用 jieba 对中文分词，建立倒排索引，用 BM25 算法给每行打一个相关性分数。本章从零讲清三件事：倒排索引是什么、BM25 分数怎么算、MiniOB 是怎么把它接进 SQL 执行引擎的，最后分析当前实现的取舍与风险。本章涉及的 SQL 执行链背景可参考 [项目架构与执行链](../01_project_architecture.md)。

## 为什么 `LIKE '%xx%'` 救不了搜索

假设表里有一千万篇文档，你写：

```sql
SELECT * FROM docs WHERE content LIKE '%数据库%';
```

这条 SQL 有两个致命问题。

**第一，索引用不上。** B+ 树索引加速的是"按 key 的前缀/等值定位"。`LIKE '数据库%'`（前缀匹配）尚可转化为 B+ 树的范围扫描；但 `%数据库%` 前后都有通配符，意味着"key 的任意位置包含子串"，B+ 树的有序性完全帮不上忙，只能**全表扫描**，对每行做一次子串匹配。一千万行 × 每行几 KB 文本，就是一次全量 I/O 加一千万次字符串查找。

**第二，没有"相关性"概念。** `LIKE` 的答案是布尔值：匹配或不匹配。它无法区分"通篇讨论数据库的论文"和"顺手提了一句数据库的广告"。搜索引擎的核心诉求是**排序**——把最相关的文档排在前面，这需要为每篇文档计算一个数值分数，而 `LIKE` 给不了。

用一句话概括本章动机：

```
LIKE '%xx%'  :  行 → 逐个检查"这行含不含子串"        （无索引、无分数）
全文检索      :  词 → 直接查出"哪些行含这个词"并打分   （倒排索引 + BM25）
```

全文检索要回答两个全新的问题：文本怎么变成可索引的"词"（分词），以及"相关程度"怎么量化（BM25）。下面依次展开。

## 信息检索速成：文档、词项、分词与停用词

信息检索（Information Retrieval, IR）里有几个基本名词，先把术语表立起来：

| 术语 | 含义 | 在 MiniOB 里的对应物 |
| --- | --- | --- |
| 文档（Document） | 被检索的基本单元 | 表里的一行（用 `RID` 标识） |
| 词项（Term） | 索引的最小单位，分词后的产物 | 倒排索引的 key（`std::string`） |
| 语料库（Corpus） | 全部文档的集合 | 整张表 |
| 词频 TF（Term Frequency） | 词项在某文档中出现的次数 | `PostingEntry::term_freq` |
| 文档频率 DF（Document Frequency） | 包含某词项的文档篇数 | posting list 的长度 |
| N | 文档总数 | `doc_stats_.size()` |

### 中文为什么要分词

英文天然用空格分词："database is great" 切一刀就是三个词项。中文没有空格：

```
全文索引是数据库的重要功能
```

这串字符到底该切成"全文 / 索引 / 是 / 数据库 / 的 / 重要 / 功能"，还是"全 / 文索 / 引是 / ..."？切错了检索就全错。所以中文检索的第一步必须是**分词**（tokenization）：把连续字符流切成有意义的词项序列。

MiniOB 用的是 **jieba**（C++ 移植版 cppjieba）。它的核心思路一句话就能说清：**先按前缀词典把句子所有可能的切法构成有向无环图（DAG），再用动态规划找联合概率最大的那条切分路径；对词典里没有的未登录词，用 HMM（隐马尔可夫模型）按字序列标注补出来**。面试里能说出"词典 + DAG + 动态规划求最大概率路径，HMM 处理未登录词"这一句就足够了。

### 停用词

"的、是、在、我、了"这类词几乎出现在每一篇文档里，对区分文档毫无帮助，却会霸占存储、拖慢检索。检索系统通常维护一张**停用词表（stop words）**，分词后直接丢弃。MiniOB 的实现复用了 cppjieba 自带的停用词表，后面源码部分会看到它是怎么拿到的。

## 倒排索引：词典与 Posting List

### 直觉：书的附录索引

普通索引（B+ 树）是"**行 → 值**"：给我主键/行号，我找到这行的某个字段值。全文检索需要的是反过来——"**值 → 行**"：给我一个词，告诉我哪些行包含它。这就像一本书末尾的"术语索引"：术语 → 出现的页码列表。这就是**倒排索引（Inverted Index）**，"倒排"正是相对"正排（行 → 值）"而言。

倒排索引由两部分组成：

- **词典（Term Dictionary）**：所有词项的集合，通常组织成有序结构或哈希表，支持"这个词在不在、它的 posting list 在哪"。
- **Posting List（倒排列表）**：每个词项对应一条列表，每个条目（posting）记录 `(docID, 词频, 位置列表)`。位置信息用于短语查询（"数据库系统"要求两词相邻）；MiniOB 的简化实现只存了 `docID` 和词频。

```
B+ 树索引:   key(字段值) ──► RID            "值 → 行"，但 key 是结构化等值
倒排索引:     term(词项) ──► [(RID,tf),...]  "值 → 行"，且值来自文本内部
```

### 手工建一次倒排表

用 5 篇小文档（假设已经分好词，词与词之间用空格隔开）：

- D1：`数据库 系统 概论`
- D2：`数据库 索引 数据库 索引 索引`
- D3：`操作 系统 原理`
- D4：`计算机 网络 概论`
- D5：`数据库 恢复 技术`

逐篇扫描，每遇到一个词就往它的 posting list 里记一笔（同一文档内重复出现则累加词频），得到的倒排索引是：

| 词项 | df | posting list（docID, tf） |
| --- | --- | --- |
| 数据库 | 3 | (D1,1) (D2,2) (D5,1) |
| 系统 | 2 | (D1,1) (D3,1) |
| 概论 | 2 | (D1,1) (D4,1) |
| 索引 | 1 | (D2,3) |
| 操作 | 1 | (D3,1) |
| 原理 | 1 | (D3,1) |
| 计算机 | 1 | (D4,1) |
| 网络 | 1 | (D4,1) |
| 恢复 | 1 | (D5,1) |
| 技术 | 1 | (D5,1) |

同时维护每张表的**文档统计**：每篇文档的长度（词数）——D1=3, D2=5, D3=3, D4=3, D5=3，总长 17，平均文档长度 avgdl = 17/5 = 3.4。这些数字下一节算 BM25 时全部要用。

查询"数据库"时，不再需要扫描任何一篇文档的原文：直接查词典，取出 posting list，就知道 D1、D2、D5 命中，且 D2 里出现了 2 次。**检索的代价从"全表行数"降到了"命中词项的 posting list 长度"**，这就是全文索引的本质加速。

## 相关性评分：从 TF-IDF 到 BM25

命中文档可能有几千篇，谁排前面？需要一个评分函数。

### TF-IDF 的直觉

最朴素的加权思想是 TF-IDF，两个直觉相乘：

- **TF（词频）**：一个词在本文档里出现越多，文档越可能和它相关。
- **IDF（逆文档频率）**：一个词在越多文档里出现，它越"没个性"，区分度越低（"的" vs "区块链"）。

```
IDF(t) = log( N / df(t) )        score(D, Q) = Σ TF(t, D) × IDF(t)
```

TF-IDF 抓对了方向，但有两个明显缺陷：

1. **词频线性增长不合理**。一个词出现 100 次的文档并不比出现 10 次的相关 10 倍——重复灌水会刷分。合理的曲线应该**边际递减、趋于饱和**。
2. **文档长度没归一**。长文档天然词多、TF 高，短文档吃亏。同样出现 1 次，10 个词的文档应该比 1000 个词的文档得分高。

### BM25 公式逐项推导

BM25（Okapi BM25，源于概率检索模型，上世纪 90 年代在 TREC 评测中脱颖而出）就是针对这两点缺陷的修正版，也是 Lucene/Elasticsearch 的默认排序算法。完整公式：

```
score(D, Q) = Σ  IDF(qi) ×  TF(qi,D) × (k1 + 1) / ( TF(qi,D) + k1 × (1 - b + b × |D| / avgdl) )
              ─┬─          ────────────────────┬────────────────────────
            词项权重                        该词在本文档中的饱和词频项
```

逐项拆开看：

**IDF 项**。BM25 的 IDF 取：

```
IDF(qi) = log( (N - df + 0.5) / (df + 0.5) )
```

两个 `+0.5` 是概率推导中来的平滑项，同时防止 `df = 0` 或 `df = N` 时出现除零和 `log(0)`。注意一个性质：当 `df > N/2`（词出现在超过一半文档中）时，分子小于分母，**IDF 是负数**；`df = N/2` 时恰好为 0。这不是 bug，是公式的数学性质，工程上要专门处理（下面讲 epsilon）。

**词频饱和项与 k1**。只看 TF 部分（暂令 b = 0，分母退化为 `TF + k1`）：

```
f(TF) = TF × (k1 + 1) / (TF + k1)
```

这是一条饱和曲线：TF 从 0 涨到 k1 附近时增长很快，之后越来越平，**渐近线是 k1 + 1**——一个词无论出现多少次，它的词频项贡献都不会超过 `k1 + 1`。参数 **k1 控制饱和速度**：k1 越小饱和越快（k1 = 0 时退化为"只看出没出现"），越大越接近线性。`TF` 上乘的 `(k1 + 1)` 是归一化系数：它让"平均长度文档里恰好出现 1 次"的词项得分恰好等于 1（代入 TF = 1、b 任意、|D| = avgdl 时分母为 `1 + k1`，分子为 `k1 + 1`），于是此时 `score = IDF × 1`，数值上非常整齐。

**长度归一与 b**。分母里的 `k1 × (1 - b + b × |D| / avgdl)` 是长度惩罚：

- `|D|` 是本文档长度，`avgdl` 是全语料平均文档长度；
- **b = 0**：完全不归一，长短文档一视同仁；
- **b = 1**：完全归一，长度影响拉满；
- b 在 0~1 之间调节惩罚力度。

**k1 = 1.5、b = 0.75 是什么含义？** 这是大量语料上实验调出的经验值（TREC 时代的"魔法数字"）：词频出现约 1.5 次后增益开始明显放缓；长度归一启用 75% 的强度。Lucene 的默认是 k1 = 1.2、b = 0.75，Python 参考库 rank_bm25 与 MiniOB 都取 k1 = 1.5、b = 0.75。**面试被问"为什么取这两个值"，诚实回答是"经验值，可随语料调参"，并能说清调大调小各自的语义**。

**为什么要 epsilon？** 如上所述，`df > N/2` 时 IDF 为负。Python 参考实现 rank_bm25 的 `BM25Okapi`（赛题评测用它做对拍）的处理是：把负 IDF 替换为 `epsilon × average_idf`（epsilon 默认 0.25，average_idf 是**词典全部词项**的平均 IDF），让超高频词贡献一个微小的正分而不是负分，避免"常见词命中反而扣分"的怪异排序。MiniOB 逐项复刻了这个行为。

### 跟着算一遍

沿用上节 5 篇文档的语料（N = 5，avgdl = 3.4），查询 `数据库 索引`（分词后两个词项）。

第一步，算各词项 IDF：

| 词项 | df | IDF = log((N − df + 0.5)/(df + 0.5)) |
| --- | --- | --- |
| 数据库 | 3 | log(2.5/3.5) = log(5/7) ≈ **−0.3365** |
| 索引 | 1 | log(4.5/1.5) = log 3 ≈ **+1.0986** |
| 系统、概论 | 2 | log(3.5/2.5) = log(1.4) ≈ +0.3365 |
| 其余 6 词 | 1 | 均 ≈ +1.0986 |

"数据库"出现在 3/5 的文档里（超过一半），IDF 为负，触发 epsilon 处理。先算全词典平均 IDF：

```
average_idf = (−0.3365 + 2×0.3365 + 7×1.0986) / 10 ≈ 0.8027
eps = 0.25 × 0.8027 ≈ 0.2007        ⇒  IDF(数据库) 用 0.2007 替代
```

第二步，对每篇候选文档累加各词项的 `IDF × 词频项`（k1 = 1.5，b = 0.75，k1 + 1 = 2.5）：

D1（长 3，"数据库"×1，无"索引"）：

```
词频项 = 1 × 2.5 / (1 + 1.5 × (0.25 + 0.75 × 3/3.4)) = 2.5 / 2.3676 ≈ 1.0559
score(D1) = 0.2007 × 1.0559 ≈ 0.2119
```

D2（长 5，"数据库"×2，"索引"×3）：

```
数据库: 词频项 = 2 × 2.5 / (2 + 1.5 × (0.25 + 0.75 × 5/3.4)) = 5 / 4.0294 ≈ 1.2409
        贡献 = 0.2007 × 1.2409 ≈ 0.2490
索引:   词频项 = 3 × 2.5 / (3 + 1.5 × (0.25 + 0.75 × 5/3.4)) = 7.5 / 5.0294 ≈ 1.4912
        贡献 = 1.0986 × 1.4912 ≈ 1.6385
score(D2) ≈ 0.2490 + 1.6385 ≈ 1.8875
```

D5（长 3，"数据库"×1）：与 D1 同分，≈ 0.2119。D3、D4 不含任何查询词，0 分。

最终排序：**D2 (1.8875) ≫ D1 = D5 (0.2119)**。这个演算同时展示了三个效应：

- **饱和**：D2 里"索引"出现 3 次，词频项 1.4912；若只出现 1 次是 0.8252；即使出现 10 次也只有 2.0782——永远压不过渐近线 2.5。
- **长度归一**：同样出现 1 次"数据库"，长 3 的 D1 得 1.0559；假如某文档长 6 同样只出现 1 次，词频项降为 2.5/3.3603 ≈ 0.7440。
- **epsilon**：超高频词"数据库"的 IDF 被替换为 0.2007 的小正数，而稀有词"索引"保有 1.0986 的高权重——排序由"索引"主导，符合直觉。

`test/bm25_score_validation.sql` 里全是这种带手工演算的对拍用例，自己实现时照着算一遍是最有效的调试手段。

## 查询处理：从查询词到 Top-K

工业界全文检索引擎执行一次查询的标准流水线：

```
查询串 ──► 分词（与建库同款分词器！） ──► 查词典取各词项 posting list
        ──► 布尔组合（AND 求交 / OR 求并）得到候选文档集
        ──► 对每篇候选文档累加 BM25 分
        ──► 最小堆维护 top-k，输出有序结果
```

两个值得记住的细节：

- **查询侧和索引侧必须用同一个分词器**。建库时切出的词项和查询时切出的词项若不一致（比如一边切"数据库"、一边切"数据/库"），posting list 就永远查不中。MiniOB 两侧都走 `JiebaUtil` 单例，天然一致。
- **top-k 用堆不用全排序**。候选集有 C 篇文档、只要前 k 篇时，维护一个大小为 k 的最小堆，复杂度 O(C log k)，优于全排序的 O(C log C)。C 很大时差异显著。

另外注意布尔语义的选择：`search`（求交，AND）适合"必须同时包含"；`search_with_scores`（求并，OR）适合"命中越多词排越前"——BM25 的分数累加天然奖励多词命中，所以现代引擎默认 OR + 打分排序。

## MiniOB 实现精读

现在对照源码。四个关键文件：

| 文件 | 职责 |
| --- | --- |
| `src/observer/common/fulltext/jieba_util.h` / `.cpp` | 分词单例 `JiebaUtil` |
| `src/observer/storage/index/fulltext_index.h` / `.cpp` | 内存倒排索引 `FullTextIndex` + BM25 |
| `src/observer/sql/expr/expression.cpp` | `MatchAgainstExpr`：SQL 表达式求值 |
| `src/observer/storage/table/table.cpp` | 建索引、DML 维护、重启重建 |

### 分词组件：`JiebaUtil`

`JiebaUtil` 是一个 **Meyers 单例**（`src/observer/common/fulltext/jieba_util.cpp:105`）：

```cpp
JiebaUtil &JiebaUtil::instance() { static JiebaUtil instance; return instance; }
```

用 pimpl（`class Impl`）把 cppjieba 的头文件隔离在 `.cpp` 内部。实现里有两个"工程味"很浓的细节，面试很可能被追问：

1. **词典定位靠 `__FILE__`**：`jieba_util.cpp:28` 用 cppjieba 的默认构造函数 `new cppjieba::Jieba()`，由库内部按源文件路径推导词典位置；构造期间还把 stderr 重定向到 `/dev/null`（`jieba_util.cpp:20-34`），压住 limonp 日志库找不到词典时的致命输出。代价：部署路径/库版本变了就可能初始化失败。
2. **停用词靠"偷"私有成员**：分词用 `jieba_->Cut(text, tokens)`（精确模式），随后要过滤停用词，但 cppjieba 的停用词表是 `KeywordExtractor` 的私有成员 `stopWords_`，于是用 `common/utils/private_accessor.h` 里的宏硬开私有访问（`jieba_util.cpp:9` 与 `:84-90`）：

```cpp
auto &stopWords_ = *GET_PRIVATE(cppjieba::KeywordExtractor, &jieba_->extractor,
                                KeywordExtractor, stopWords_);
tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
                            [&stopWords_](const std::string &word) {
                              return stopWords_.find(word) != stopWords_.end();
                            }),
             tokens.end());
```

功能正确，但对 cppjieba 的内部结构是强耦合——库一改成员名就编译失败。这是一个可以主动跟面试官聊的"已知风险"。

`JiebaUtil` 还提供 `format_tokens_as_json`，把分词结果格式化为 JSON 数组，支撑调试函数 `SELECT TOKENIZE('你好世界', 'jieba')`（实现于 `src/observer/sql/builtin/builtin.cpp:484` 的 `builtin::tokenize`）——写赛题时用它可以快速确认分词和停用词过滤是否符合预期。

### 索引结构：`FullTextIndex`

核心数据结构就是前面理论部分的直译（`src/observer/storage/index/fulltext_index.h:25-55`）：

```cpp
struct PostingEntry {
  RID doc_rid;                   // 文档ID
  int term_freq;                 // 词频 TF
  std::vector<int> positions;    // 位置（简化实现不使用）
};
using PostingList   = std::vector<PostingEntry>;
using InvertedIndex = std::unordered_map<std::string, PostingList>;   // 词典 → posting list
struct DocumentStats { RID doc_rid; int doc_length; };                // 每篇文档的长度
```

`FullTextIndex` 的成员（`fulltext_index.h:136-140`）：`inverted_index_`（词典 + posting list）、`doc_stats_`（`RID → 文档长度`，哈希器用 `record.h:88` 的 `RIDHash`）、`total_tokens_`（全部文档词数之和，配合文档数即得 avgdl）、以及 `average_idf_` 缓存。注意三点：

- 词典用 `unordered_map`，不是有序结构——**MiniOB 的词典不支持前缀/模糊查询，只能精确命中**，这对赛题足够；
- `positions` 字段存在但从不填充，因此**不支持短语查询**（"数据库系统"必须相邻这种需求做不到）；
- posting list 是普通 `vector`，增删条目时线性扫描——MiniOB 的取舍是"正确优先"。

### 建索引：`ALTER TABLE ... ADD FULLTEXT INDEX`

语法定义在 `src/observer/sql/parser/yacc_sql.y:382`：

```
ALTER TABLE ID ADD FULLTEXT INDEX ID LBRACE ID RBRACE WITH PARSER ID
```

即 `ALTER TABLE texts ADD FULLTEXT INDEX idx (content) WITH PARSER jieba;`。解析结果装进 `parse_defs.h:271` 的 `FullTextIndexConfig { index_name, column_name, parser }`。随后：

1. `src/observer/sql/stmt/alter_table_stmt.cpp:72-85` 校验三个字段非空，且 **parser 只认 `"jieba"`**；
2. `src/observer/sql/executor/alter_table_executor.cpp:27` 调 `Db::alter_table`，分发到 `db.cpp:261-264`，进入 `Table::create_fulltext_index`（`table.cpp:1381`）；
3. `create_fulltext_index` 做三件事：校验列存在且类型是 `TEXTS`/`CHARS`（`table.cpp:1402`）；用 `RecordFileScanner` **全表扫描**，对每行取文本字段（NULL 跳过）调 `add_document`；把建好的索引挂到 `fulltext_indexes_[column_name]`——注意这是 `Table` 里一个**独立**的 map，和 B+ 树的 `indexes_` 不是同一套，索引元信息则以 `IndexType::FullTextIndex` 记入表元数据。

`add_document`（`fulltext_index.cpp:19-79`）的流程：拿到文本 → 若调用方没给分词结果就调 `JiebaUtil::tokenize` → 统计每个词项的 TF → 把 `(RID, tf)` 追加进对应 posting list → 登记 `doc_stats_[rid]` 与 `total_tokens_` → 让 average_idf 缓存失效。若该 RID 已存在，先 `remove_document` 再加——这保证了"同 RID 重复加入"是幂等的。

重启恢复也在 `Table::open` 里：发现元数据中有 `IndexType::FullTextIndex` 就重新全表扫描重建内存倒排索引（`table.cpp:332-401`）。代码注释说得很直白：`in-memory only, will be rebuilt on table open`（`table.cpp:1478`）。

### 查询执行：`MatchAgainstExpr`

`MATCH(content) AGAINST('数据库')` 的语法在 `yacc_sql.y:1215`，要求括号里恰好一个字段表达式，产物是 `MatchAgainstExpr`（`expression.h:602`）。它的 `get_value`（`expression.cpp:990-1120`）干的事：

1. 对搜索串求值并**分词**（`expression.cpp:1011`）；
2. 确认左操作数是 `FieldExpr`，拿到字段名和 `Table` 指针；
3. 拿当前行的 RID：优先把 tuple 向下转型为 `RowTuple` 再取 `record().rid()`（`expression.cpp:1040-1047`），失败则退到 `base_rids()` 里按表查找；
4. `table->get_fulltext_index(field_name)` 找到该字段的 `FullTextIndex`，**直接调 `calculate_bm25(query_tokens, rid)`** 把分数作为表达式值返回（`expression.cpp:1093-1104`）；

```cpp
FullTextIndex *ft_idx = table->get_fulltext_index(field_name);
if (ft_idx != nullptr) {
  double score = ft_idx->calculate_bm25(query_tokens, rid);
  value = Value(static_cast<float>(score));
  return RC::SUCCESS;
}
```

找不到索引就直接报错返回 `INVALID_ARGUMENT`（`expression.cpp:1117`）——**没有回退成 LIKE 之类的简化计算**，这是刻意的：分数必须来自真索引。

`calculate_bm25`（`fulltext_index.cpp:174-259`）严格对齐 rank_bm25 的 `BM25Okapi`：`epsilon = 0.25`；`average_idf_` 惰性计算并缓存（索引变更时失效）；查询词的 IDF 为负就替换成 `eps`；最后逐词累加 `idf × term_score`。词频项在 `calculate_term_bm25`（`fulltext_index.cpp:343-360`）：

```cpp
const double k1 = 1.5;
const double b  = 0.75;
double numerator   = term_freq * (k1 + 1.0);
double denominator = term_freq + k1 * (1.0 - b + b * (doc_length / avg_doc_length));
return numerator / denominator;   // 分母过小（<1e-9）时返回 0 兜底
```

IDF 则是 `log((N − df + 0.5) / (df + 0.5))`（`calculate_idf`，`fulltext_index.cpp:329-341`），与上文推导逐项对应。

### DML 维护与重建

- **INSERT**：`Table::insert_entry_of_indexes`（`table.cpp:1645-1686`）在更新 B+ 树索引后，遍历 `fulltext_indexes_`，对新行文本调 `add_document`。
- **UPDATE**（VacuousTrx 原地更新路径，`table.cpp:1604` 起）：内部调 `insert_entry_of_indexes(新行)`，由于 RID 不变，`add_document` 会先删旧文档再插入，结果正确。
- **DELETE**：这里有个坑。`delete_entry_of_indexes`（`table.cpp:1688`）确实实现了全文索引清理（`remove_document`），但物理删除走的 `Table::delete_record`（`table.cpp:1589-1602`）**只遍历 `indexes_`（B+ 树），根本不调它**——被删文档的 posting 条目、文档统计会残留在内存索引里，继续污染 N、df 和 avgdl。赛题解析文档对此有详细分析（见"回到赛题"）。

`remove_document`（`fulltext_index.cpp:81-114`）本身也有特点：它要**遍历全部词项**的 posting list 才能删掉一个文档（因为索引是"词 → 文档"，没有"文档 → 词"的反向表），并且**不清理删空了的词项**——这些 df=0 的"幽灵词"之后还会被 `calculate_average_idf` 算进平均值，使 epsilon 偏离 rank_bm25。

### 当前实现的取舍与风险

这是面试里最值得展开的一段。当前实现是"**功能正确的最小闭环**"，与工业实现差距主要在四处：

1. **索引没有驱动候选扫描**。`MatchAgainstExpr` 是表达式，执行计划仍是**全表扫描**：每扫到一行，取 RID，调一次 `calculate_bm25`。倒排索引本可以先求出候选集（`search` 求交 / `search_with_scores` 求并已就位），再由专用 scan 算子只访问候选行——但这条链路没有接进优化器/执行器，`search_with_scores` 在现行查询路径上**没有调用者**。换句话说，索引此刻只是"按 RID 快速算分的评分机"，省掉了对原文的重复分词，却没有省下扫描本身。
2. **每行重复分词查询串**。`get_value` 对每一行都调一次 `JiebaUtil::tokenize(search_text)`（`expression.cpp:1011`），同一查询串被分词 N 次，纯属浪费——查询词项本应在语句级算一次缓存下来。
3. **纯内存、不落盘、不走 WAL**。索引只在内存，重启靠全表重建；与 clog/redo 体系完全无关。数据量大时启动重建成本高，崩溃恢复的语义也全靠"重建"兜底。
4. **并发与一致性欠账**。`FullTextIndex` 内部没有任何锁，并发 DML 与 MATCH 同时进行是未定义行为；MVCC 的 UPDATE 会插入新版本行，但旧版本的全文条目不清理；上文的 DELETE 残留、幽灵词污染 average_idf 也都在这一类。

说清楚这些取舍，比背公式更能体现你读懂了代码。

## 复杂度分析

设文档数 N、总词数 T（token 总数）、词典大小 V、查询词数 q、候选文档数 C：

| 操作 | 当前 MiniOB | 理想的索引驱动实现 |
| --- | --- | --- |
| 建索引/重启重建 | O(T) | O(T) |
| 插入一篇文档 | O(本文档词数) | O(本文档词数) |
| 删除一篇文档 | O(总 posting 条目数)（遍历所有词项） | O(本文档不同词数 × log)（正排表辅助） |
| MATCH 查询 | O(N × (分词 + q × posting 查找))，全表逐行评分 | O(Σ\|posting(qi)\| + C log k)，posting 归并 + 堆 top-k |
| 空间 | O(T + N)（posting 条目 + 文档统计） | 同量级，但落盘分块 |

关键差异就在查询行：理想实现的工作量是"命中条目数"级别，与表大小无关；当前实现是"表行数"级别。**倒排索引的价值在 MiniOB 里只兑现了一半**——兑现了"按 RID 查 tf 和统计信息"，没兑现"按词直接圈定候选行"。

## 工业界怎么做

**Lucene / Elasticsearch**：教科书级实现，三个与本章直接对应的设计：

- **段（Segment）**：索引不是一整块，而是不可变的段。写入生成新段，删除只是在段上打墓碑标记，后台定期 merge 小段为大段并物理清除墓碑。换来的是写放大可控、并发读无锁（段不可变）、崩溃恢复简单。对照 MiniOB：`remove_document` 原地改 `vector`、删完还留空词项，正是"没有段概念"的直接后果。
- **FST 词典**：Lucene 的词典用有限状态转移机（FST）压缩存储，内存占用极小且支持前缀、通配、模糊匹配。对照 MiniOB：`unordered_map` 只够精确查找。
- **跳表与按块计分**：posting list 按 docID 排序并配跳表（新版本用按块组织的 BLOCK-MAX / impact ordering），求交时可以跳过不可能命中的区间，打分也可按块提前剪枝 top-k。这是"查询复杂度与命中数相关"真正落地的机制。

**MySQL InnoDB**：`FULLTEXT INDEX` 以一组隐藏的辅助表实现（`FTS_<表ID>_INDEX_1..6`），存 `(word, ilist)`，ilist 里带 docID 与位置信息，因此支持短语查询；被索引表自动加隐藏列 `FTS_DOC_ID`；删除走 `FTS_*_DELETED` 记账，`OPTIMIZE TABLE` 时才物理合并——思想与 Lucene 的段/墓碑一脉相承。中文场景 InnoDB 内置 ngram 分词器（按字二元切分，不需要词典），语法上的 `WITH PARSER` 子句正是 MiniOB 赛题语法模仿的对象。

**PostgreSQL**：走另一条路——先把文本预处理成 `tsvector`（词位 + 归一化词元），用 `tsquery` 表达查询，索引用 GIN（本质是"词元 → posting list"的倒排，与本章结构同构）或 GiST（签名式、有损但更小）；排序用 `ts_rank` / `ts_rank_cd`（后者基于覆盖密度而非 BM25）。词干提取、停用词由可配置的字典体系完成。

**OceanBase**：在 MySQL 兼容租户中提供 FULLTEXT INDEX 能力，语法风格与 MySQL 一致；作为分布式数据库，其索引数据随表分区分布在多机上，检索时要合并各分区的局部结果——这是在 MiniOB 单机实现之上额外要解决的问题。MiniOB 赛题 23 可以看作把这套能力压缩到"单机、内存、单字段"的最小可教学版本。

## 小结

- `LIKE '%xx%'` 无法用索引、没有相关性分数；全文检索的答案是**分词 + 倒排索引 + BM25**。
- 倒排索引是"词项 → posting list"的反向映射，把检索代价从"全表行数"降到"命中条目数"；posting 至少含 docID 和词频，位置信息支撑短语查询。
- BM25 修正 TF-IDF 的两个缺陷：k1 让词频**饱和**（渐近线 k1+1），b 做**长度归一**；IDF 为 `log((N−df+0.5)/(df+0.5))`，`df > N/2` 时为负，rank_bm25/MiniOB 用 `0.25 × average_idf` 替换。k1=1.5、b=0.75 是经验值。
- MiniOB 的实现链路：`JiebaUtil` 单例分词（词典 + 停用词过滤）→ `FullTextIndex` 内存倒排（`InvertedIndex`/`PostingEntry`/`DocumentStats`）→ `ALTER TABLE ... ADD FULLTEXT INDEX ... WITH PARSER jieba` 全表扫描建索引 → `MatchAgainstExpr` 对每行取 RID 调 `calculate_bm25`。
- 当前实现功能正确但做了大量简化：全表逐行评分（索引不驱动候选扫描）、每行重复分词、纯内存不落盘不走 WAL、DELETE/并发/幽灵词存在已知欠账。这些是面试加分项，也是可改进方向。

## 面试追问

**问：`LIKE '%xx%'` 为什么慢？全文索引快在哪？**
答：`%xx%` 前后通配，破坏了 B+ 树 key 的前缀有序性，只能全表扫描加逐行子串匹配，复杂度随行数与文本长度线性增长；且结果是布尔的，无法排序。倒排索引把"词 → 文档列表"预先物化，查询时分词后直接取出 posting list，工作量只与命中条目数相关，并且 posting 里带词频、索引维护全局统计，能算出可排序的相关性分数。

**问：jieba 分词的基本思路？**
答：基于前缀词典构造句子的切分 DAG，用动态规划找联合概率最大的切分路径；词典未覆盖的未登录词用 HMM 按字序列标注识别。工程上是"词典为主、统计模型兜底"。MiniOB 用 cppjieba 的 `Cut` 精确模式，随后再过滤停用词表。

**问：BM25 相比 TF-IDF 好在哪？k1 和 b 各控制什么？**
答：TF-IDF 的词频线性增长（灌水刷分）且不做长度归一（长文档占便宜）。BM25 用 `TF×(k1+1)/(TF+K)` 让词频贡献饱和，k1 控制饱和速度（k1→0 只看"出没出现"，越大越接近线性，渐近线 k1+1）；`K = k1×(1−b+b×|D|/avgdl)` 做长度惩罚，b 控制归一强度（0 不归一，1 完全归一）。k1=1.5、b=0.75 是经验值，Lucene 取 1.2/0.75。

**问：IDF 为什么会是负数？MiniOB 怎么处理的？**
答：MiniOB 采用 BM25 的 IDF 形式 `log((N−df+0.5)/(df+0.5))`，当一个词出现在超过一半文档中（`df > N/2`）时分子小于分母，IDF 为负。为对齐评测参考实现 rank_bm25 的 `BM25Okapi`，`FullTextIndex::calculate_bm25` 把负 IDF 替换为 `0.25 × average_idf`（全词典所有词项 IDF 的均值，带缓存）。这让超高频词贡献微小正分而非负分，避免排序倒挂。对应的验证用例见 `test/bm25_epsilon_test.sql` 与 `test/BM25_EPSILON_EXPLANATION.md`。

**问：MiniOB 的全文索引真的"加速"查询了吗？**
答：严格说只兑现了一半。现行 `MATCH ... AGAINST` 路径是全表扫描：每行由 `MatchAgainstExpr::get_value` 取 RID 后调 `calculate_bm25` 算分，倒排索引只承担了"按 RID 查 tf/统计"的评分机角色，**没有用 posting list 先圈定候选集**，所以查询复杂度仍是 O(N) 行级别。改进方向是把 `search_with_scores`（已实现并集 + 打分 + 排序）接到执行器里做候选生成，再配 top-k 堆排序算子。另外当前每行都重复对查询串分词，也是可优化点。

**问：全文索引的删除/更新怎么做？MiniOB 有什么问题？**
答：理论上删除文档要同步清掉它在所有 posting list 里的条目并更新 N、avgdl 等统计，否则残余文档的 BM25 会失真。MiniOB 的 `remove_document` 遍历全部词项做清理（O(总条目数)），且不回收删空的词项，这些 df=0 的幽灵词会继续摊薄 `average_idf`；更关键的是物理删除路径 `Table::delete_record` 只处理 B+ 树索引，**根本不会触碰全文索引**。工业界（Lucene/InnoDB）的通行做法是墓碑标记 + 后台合并，避免原地高代价删除。

## 回到赛题

本章对应赛题 **23 full-text-index**（见 [赛题 17-24 解析](../04_problems_17_24.md) 第 23 节）：实现 `ALTER TABLE ... ADD FULLTEXT INDEX ... WITH PARSER jieba` 建索引，并让 `MATCH(col) AGAINST(query)` 返回与 rank_bm25 对齐的 BM25 分数。赛题里每一处都能在本章找到出处：分词与停用词对应 `JiebaUtil`；倒排结构对应 `FullTextIndex` 的三个核心成员；epsilon 处理对应 `calculate_bm25` 里的 `0.25 × average_idf`；"索引未驱动候选扫描"正是解析文档指出的核心取舍。

动手验证时建议按顺序跑仓库自带的脚本：`test/quick_bm25_test.sql`（快速确认分数是动态计算的而非固定值）、`test/bm25_score_validation.sql`（六个带手工演算的对拍场景，和本章"跟着算一遍"同法）、`test/bm25_epsilon_test.sql`（负 IDF 边界），测试组织方式见 `test/BM25_TEST_README.md`；分词是否符合预期可随时用 `SELECT TOKENIZE('...', 'jieba')` 检查。仓库根的 `BM25/rank_bm25/` 目录预留给 Python 参考实现 rank_bm25（当前为空，评测平台用它做对拍），对齐口径以 `test/BM25_EPSILON_EXPLANATION.md` 和 `fulltext_index.cpp` 注释中的 GitHub 链接为准。调试时若分数异常，`test/BM25_TEST_README.md` 的排查清单（固定分数 ≈ 静态 IDF、分数不变 ≈ 没走索引等）能直接定位问题。
