# MiniOB 项目架构全面分析

> **创建时间**: 2025-11-01  
> **目的**: 理解 MiniOB 项目整体架构，为实现 RAG 相关功能做准备

## 📋 目录

1. [项目概述](#1-项目概述)
2. [核心架构](#2-核心架构)
3. [模块详解](#3-模块详解)
4. [向量索引与 RAG 支持](#4-向量索引与-rag-支持)
5. [外部排序实现](#5-外部排序实现)
6. [RAG 功能实现思路](#6-rag-功能实现思路)

---

## 1. 项目概述

### 1.1 MiniOB 简介

**MiniOB** 是 OceanBase 团队开发的教学型数据库管理系统，专为零基础学习者设计。

**特点**:
- 🎓 教学友好：代码简洁，易于理解
- 🔧 功能完整：包含数据库核心模块
- 🚀 可扩展：支持自定义功能开发
- 📚 文档丰富：提供详细的架构文档

**当前状态**:
- ✅ 已实现外部排序（Big Order By）
- ✅ 已支持向量索引（IVF-Flat）
- ✅ 已集成 RAG 工作流（Langflow）

### 1.2 技术栈

```
语言: C++ (C++17)
构建系统: CMake
测试框架: GoogleTest
依赖管理: git submodules
```

### 1.3 目录结构

```
miniob/
├── src/
│   ├── observer/          # 数据库服务端主要代码
│   │   ├── common/        # 公共组件（类型、值、工具等）
│   │   ├── event/         # 事件系统
│   │   ├── net/           # 网络通信
│   │   ├── session/       # 会话管理
│   │   ├── sql/           # SQL处理（核心！）
│   │   │   ├── parser/    # SQL解析
│   │   │   ├── executor/  # SQL执行
│   │   │   ├── operator/  # 物理/逻辑算子
│   │   │   ├── optimizer/ # 查询优化
│   │   │   ├── expr/      # 表达式和Tuple
│   │   │   └── stmt/      # SQL语句抽象
│   │   └── storage/       # 存储引擎
│   │       ├── buffer/    # 缓冲池
│   │       ├── record/    # 记录管理
│   │       ├── table/     # 表管理
│   │       ├── index/     # 索引（含向量索引）
│   │       └── trx/       # 事务管理
│   └── obclient/          # 数据库客户端
├── rag/                   # RAG 工作流配置
│   └── model.json         # Langflow 配置
├── docs/                  # 文档
├── test/                  # 测试用例
├── deps/                  # 依赖库
└── build.sh               # 编译脚本
```

---

## 2. 核心架构

### 2.1 整体架构图

```
┌──────────────────────────────────────────────────────┐
│                  Client (obclient)                    │
└────────────────────┬─────────────────────────────────┘
                     │ SQL 请求/响应
┌────────────────────▼─────────────────────────────────┐
│              Network Service (net)                    │
│  - 网络通信协议（Plain/MySQL/CLI）                    │
│  - 连接管理                                           │
└────────────────────┬─────────────────────────────────┘
                     │
┌────────────────────▼─────────────────────────────────┐
│             Session Management                        │
│  - 用户会话管理                                        │
│  - 参数配置                                           │
└────────────────────┬─────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
┌───────▼────────┐  ┌─────────────▼──────────┐
│   SQL Parser   │  │  Command Executor      │
│  - 词法分析     │  │  - DDL 执行             │
│  - 语法分析     │  │  - DCL 执行             │
│  - AST 生成     │  │                        │
└───────┬────────┘  └────────────────────────┘
        │
┌───────▼────────┐
│   Resolver     │
│  - 语义解析     │
│  - 名称绑定     │
└───────┬────────┘
        │
┌───────▼────────┐
│   Optimizer    │
│  - 逻辑优化     │
│  - 物理优化     │
│  - 代价估算     │
└───────┬────────┘
        │
┌───────▼────────┐
│   Executor     │
│  - 算子树执行   │
│  - DML 处理     │
└───────┬────────┘
        │
        ├─────────────┬──────────────┬─────────────┐
        │             │              │             │
┌───────▼──────┐ ┌────▼─────┐ ┌────▼─────┐ ┌────▼─────┐
│ Table/Record │ │  Index   │ │   MVCC   │ │  Redo    │
│   Manager    │ │ (B+Tree) │ │   Trx    │ │   Log    │
└───────┬──────┘ │ (IVFFlat)│ │  Manager │ └──────────┘
        │        └──────────┘ └──────────┘
┌───────▼──────────────────────────────────────────────┐
│           Buffer Pool (DiskBufferPool)               │
│  - 页面缓存                                           │
│  - LRU 替换                                           │
│  - 脏页管理                                           │
└───────┬──────────────────────────────────────────────┘
        │
┌───────▼──────────────────────────────────────────────┐
│              Disk I/O & File System                  │
└──────────────────────────────────────────────────────┘
```

### 2.2 SQL 执行流程

```
SQL 文本
    │
    ▼
┌─────────────┐
│   Parser    │  词法分析 (lex_sql.l) + 语法分析 (yacc_sql.y)
└──────┬──────┘
       │ AST (ParsedSqlNode)
       ▼
┌─────────────┐
│  Resolver   │  语义解析，名称绑定，生成 Stmt
└──────┬──────┘
       │ Stmt (SelectStmt, InsertStmt, etc.)
       ▼
┌─────────────┐
│  Optimizer  │  生成逻辑算子树 → 物理算子树
└──────┬──────┘
       │ PhysicalOperator 树
       ▼
┌─────────────┐
│  Executor   │  volcano 模型执行
└──────┬──────┘
       │ Tuple 流
       ▼
    结果集
```

### 2.3 物理算子模型（Volcano Model）

MiniOB 使用经典的 **火山模型（Volcano Model）** 执行查询。

**核心接口**:
```cpp
class PhysicalOperator {
public:
  virtual RC open(Trx *trx) = 0;    // 初始化算子
  virtual RC next() = 0;             // 获取下一行
  virtual RC close() = 0;            // 关闭算子
  virtual Tuple *current_tuple() = 0; // 当前行数据
};
```

**主要算子**:

| 算子类型 | 功能 | 文件位置 |
|---------|------|---------|
| `TableScanPhysicalOperator` | 全表扫描 | `sql/operator/table_scan_physical_operator.{h,cpp}` |
| `IndexScanPhysicalOperator` | 索引扫描 | `sql/operator/index_scan_physical_operator.{h,cpp}` |
| `VectorScanPhysicalOperator` | 向量索引扫描 | `sql/operator/vector_scan_physical_operator.{h,cpp}` |
| `ProjectPhysicalOperator` | 投影 | `sql/operator/project_physical_operator.{h,cpp}` |
| `PredicatePhysicalOperator` | 谓词过滤 | `sql/operator/predicate_physical_operator.{h,cpp}` |
| `JoinPhysicalOperator` | 嵌套循环连接 | `sql/operator/join_physical_operator.{h,cpp}` |
| `GraceHashJoinPhysicalOperator` | 哈希连接 | `sql/operator/grace_hash_join_physical_operator.{h,cpp}` |
| `GroupByPhysicalOperator` | 分组聚合 | `sql/operator/group_by_physical_operator.{h,cpp}` |
| `OrderByPhysicalOperator` | 排序 | `sql/operator/order_by_physical_operator.{h,cpp}` |
| `LimitPhysicalOperator` | 限制结果数 | `sql/operator/limit_physical_operator.{h,cpp}` |

---

## 3. 模块详解

### 3.1 存储引擎

#### 3.1.1 Buffer Pool (缓冲池)

**位置**: `src/observer/storage/buffer/`

**核心类**:
- `DiskBufferPool`: 磁盘缓冲池管理
- `BPFrameManager`: 页帧管理器
- `Frame`: 内存页帧

**功能**:
```cpp
class DiskBufferPool {
  RC get_this_page(PageNum page_num, Frame **frame);  // 获取页面
  RC allocate_page(Frame **frame);                     // 分配新页面
  RC dispose_page(PageNum page_num);                   // 释放页面
  RC flush_page(Frame &frame);                         // 刷新到磁盘
  RC unpin_page(Frame *frame);                         // 解锁页面
};
```

**关键概念**:
- **Page**: 磁盘上的数据单位，固定大小（默认 8KB）
- **Frame**: 内存中的页帧，用于缓存磁盘页
- **PageNum**: 页号，用于标识页面
- **LRU Cache**: 用于页面淘汰策略

#### 3.1.2 Record Manager (记录管理)

**位置**: `src/observer/storage/record/`

**核心类**:
- `RecordFileHandler`: 管理整个文件中的记录
- `RecordPageHandler`: 管理单个页面中的记录
- `RecordFileScanner`: 遍历文件中的所有记录

**页面组织**:
```
┌─────────────────────────────────────────────────┐
│ PageHeader | record allocate bitmap             │
├─────────────────────────────────────────────────┤
│ record1 | record2 | record3 | ... | recordN    │
└─────────────────────────────────────────────────┘
```

**PageHeader 结构**:
```cpp
struct PageHeader {
  int32_t record_num;        // 当前页面记录个数
  int32_t column_num;        // 列数
  int32_t record_real_size;  // 每条记录实际大小
  int32_t record_size;       // 每条记录占用空间(对齐后)
  int32_t record_capacity;   // 最大记录个数
  int32_t data_offset;       // 第一条记录的偏移量
};
```

#### 3.1.3 Index (索引)

**位置**: `src/observer/storage/index/`

**索引类型**:

1. **B+ Tree 索引** (`bplus_tree_index.{h,cpp}`)
   - 标准 B+ 树实现
   - 支持范围查询
   - 支持唯一键约束

2. **IVF-Flat 向量索引** (`ivfflat_index.{h,cpp}`)
   - 倒排文件索引（Inverted File Index）
   - 基于聚类的 ANN 搜索
   - 支持 L2 距离、余弦相似度等

**向量索引接口**:
```cpp
class IvfflatIndex : public Index {
public:
  // 创建索引
  RC create(Table *table, const char *file_name, 
            const IndexMeta &index_meta, const FieldMeta &field_meta);
  
  // ANN 搜索（近似最近邻）
  std::vector<RID> ann_search(const std::vector<float> &base_vector, 
                               size_t limit);
  
  // 插入向量
  RC insert_entry(const char *record, const RID *rid);
  
  // 构建索引
  RC build_index(std::vector<std::pair<Vector, RID>> &initial_data,
                 NormalFunctionType distance_fn,
                 const std::vector<int> &options);
};
```

### 3.2 SQL 层

#### 3.2.1 Parser (解析器)

**位置**: `src/observer/sql/parser/`

**组件**:
- `lex_sql.l`: Flex 词法分析器定义
- `yacc_sql.y`: Bison 语法分析器定义
- `parse.{h,cpp}`: 解析入口

**支持的 SQL 类型**:
- DDL: CREATE TABLE, DROP TABLE, CREATE INDEX, etc.
- DML: SELECT, INSERT, UPDATE, DELETE
- DCL: 事务控制（BEGIN, COMMIT, ROLLBACK）
- 特殊: SHOW TABLES, DESC, EXPLAIN, etc.

#### 3.2.2 Tuple (数据行抽象)

**位置**: `src/observer/sql/expr/tuple.h`

**核心接口**:
```cpp
class Tuple {
public:
  virtual int cell_num() const = 0;
  virtual RC cell_at(int index, Value &cell) const = 0;
  virtual RC spec_at(int index, TupleCellSpec &spec) const = 0;
  virtual RC compare(const Tuple &other, int &result) const;
  virtual Tuple *copy() const = 0;
};
```

**主要实现**:

| Tuple 类型 | 说明 |
|-----------|------|
| `RowTuple` | 表中的一行记录 |
| `ValueListTuple` | 值列表（用于中间结果） |
| `JoinedTuple` | 连接后的 Tuple |
| `ProjectTuple` | 投影后的 Tuple |
| `ExpressionTuple` | 表达式计算结果 |

#### 3.2.3 Value (值类型)

**位置**: `src/observer/common/value.h`

**支持的类型**:
```cpp
enum class AttrType {
  INTS,      // 整型
  FLOATS,    // 浮点型
  CHARS,     // 字符串
  BOOLEANS,  // 布尔型
  DATES,     // 日期型
  TEXTS,     // 文本型
  VECTORS,   // 向量型（重要！）
  NULLS      // NULL
};
```

**向量类型支持**:
```cpp
class Value {
public:
  explicit Value(const vector<float> &values);  // 构造向量
  std::vector<float> get_vector() const;        // 获取向量
  int get_vector_length() const;                // 获取向量长度
  float get_vector_element(int i) const;        // 获取向量元素
  
  void set_vector(const vector<float> &val);    // 设置向量
};
```

### 3.3 事务管理（MVCC）

**位置**: `src/observer/storage/trx/`

**核心概念**:
- 使用 MVCC (Multi-Version Concurrency Control)
- 每个事务有唯一 TrxID
- 记录携带版本信息（创建版本、删除版本）

---

## 4. 向量索引与 RAG 支持

### 4.1 向量数据类型

MiniOB 原生支持 **VECTOR** 数据类型。

**创建表示例**:
```sql
CREATE TABLE documents (
  id INT,
  title CHARS(100),
  content TEXT,
  embedding VECTOR(768)  -- 768 维向量
);
```

**插入数据**:
```sql
INSERT INTO documents VALUES (
  1,
  'Document Title',
  'Document content...',
  '[0.1, 0.2, 0.3, ..., 0.768]'
);
```

### 4.2 IVF-Flat 向量索引

**创建索引**:
```sql
CREATE VECTOR INDEX idx_embedding ON documents(embedding) 
  WITH (LISTS=100, DISTANCE='L2');
```

**索引参数**:
- `LISTS`: 聚类中心数量（影响索引构建速度和查询精度）
- `PROBES`: 查询时探测的聚类数量（影响查询精度和速度）
- `DISTANCE`: 距离度量函数
  - `L2`: 欧几里得距离
  - `COSINE`: 余弦相似度
  - `IP`: 内积

**ANN 查询（近似最近邻）**:
```sql
SELECT id, title, 
       VECTOR_DISTANCE(embedding, '[query_vector]', 'L2') as distance
FROM documents
ORDER BY VECTOR_DISTANCE(embedding, '[query_vector]', 'L2')
LIMIT 5;
```

### 4.3 VectorScanPhysicalOperator

**算子特点**:
- 使用 IVF-Flat 索引加速查询
- 支持 Top-K 查询
- 返回按距离排序的结果

**执行流程**:
```cpp
RC VectorScanPhysicalOperator::open(Trx *trx) {
  // 1. 调用 IvfflatIndex::ann_search 获取 Top-K RID
  rids_ = index_->ann_search(base_vector_, limit_);
  
  // 2. 初始化 RecordFileHandler
  record_handler_ = &table_->record_handler();
  
  return RC::SUCCESS;
}

RC VectorScanPhysicalOperator::next() {
  // 遍历 RID 列表，读取记录
  while (cnt_ < rids_.size()) {
    RC rc = record_handler_->get_record(rids_[cnt_], current_record_);
    if (rc == RC::SUCCESS) {
      tuple_.set_record(&current_record_);
      cnt_++;
      return RC::SUCCESS;
    }
    cnt_++;
  }
  return RC::RECORD_EOF;
}
```

### 4.4 RAG 工作流（Langflow 集成）

**配置文件**: `rag/model.json`

**组件架构**:
```
┌──────────────┐
│ Chat Input   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Embedding    │  (Ollama Embeddings)
│ Model        │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  MiniOB      │  (自定义 VectorStore 组件)
│ Vector Store │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Parser     │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Prompt     │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Chat Output  │
└──────────────┘
```

**MiniOB VectorStore 组件功能**:

```python
class MiniOBVectorStore(VectorStore):
    """MiniOB 向量存储组件"""
    
    def __init__(self, embedding_model, table_name, 
                 host="127.0.0.1", port=6789):
        self.embedding_model = embedding_model
        self.table_name = table_name
        self.client = MiniOBClient(host, port)
    
    def add_documents(self, documents: List[Document]):
        """添加文档到向量库"""
        for doc in documents:
            # 1. 生成嵌入
            embedding = self.embedding_model.embed_query(doc.page_content)
            
            # 2. 插入 MiniOB
            sql = f"""
                INSERT INTO {self.table_name} 
                VALUES ({doc.id}, '{doc.page_content}', '{embedding}')
            """
            self.client.execute(sql)
    
    def similarity_search(self, query: str, k: int = 4):
        """相似度搜索"""
        # 1. 生成查询向量
        query_vector = self.embedding_model.embed_query(query)
        
        # 2. 执行 ANN 查询
        sql = f"""
            SELECT id, content,
                   VECTOR_DISTANCE(embedding, '{query_vector}', 'L2') as distance
            FROM {self.table_name}
            ORDER BY distance
            LIMIT {k}
        """
        results = self.client.execute(sql)
        
        return [Document(page_content=r['content']) for r in results]
```

**工作流程**:

1. **用户提问** → Chat Input
2. **生成查询向量** → Ollama Embeddings
3. **检索相关文档** → MiniOB Vector Store (ANN 搜索)
4. **解析结果** → Parser
5. **构建提示词** → Prompt (包含检索到的上下文)
6. **生成回答** → LLM + Chat Output

---

## 5. 外部排序实现

### 5.1 需求背景

**原有 ORDER BY 问题**:
- ❌ 全部数据必须加载到内存
- ❌ 无法处理大数据集
- ❌ 启动延迟高

**外部排序解决方案**:
- ✅ 使用磁盘辅助排序
- ✅ 支持任意大小的数据集
- ✅ 内存使用可控

### 5.2 外部排序架构

**位置**: `src/observer/sql/operator/external_sort/`

**核心组件**:

```
external_sort/
├── external_sorter.{h,cpp}      # 外部排序器（主要逻辑）
├── temp_file_manager.{h,cpp}    # 临时文件管理
└── tuple_serializer.{h,cpp}     # Tuple 序列化/反序列化
```

### 5.3 ExternalSorter 实现

**核心接口**:
```cpp
class ExternalSorter {
public:
  ExternalSorter(const std::vector<OrderBySqlNode> &order_by, 
                 size_t memory_limit);
  
  // 执行外部排序
  RC sort(PhysicalOperator *input);
  
  // 获取下一个排序好的 tuple
  RC next(Tuple *&tuple);
  
  // 关闭排序器
  RC close();

private:
  // Phase 1: 生成排序好的 run 文件
  RC generate_runs(PhysicalOperator *input);
  
  // Phase 2: 多路归并 run 文件
  RC merge_runs();
  
  // 刷新缓冲区到 run 文件
  RC flush_buffer_to_run();
  
  // Tuple 比较
  bool compare_tuples(const Tuple *t1, const Tuple *t2) const;
};
```

**算法流程**:

```
┌─────────────────────────────────────────┐
│         Phase 1: Run Generation          │
├─────────────────────────────────────────┤
│  1. 读取数据到内存缓冲区                  │
│  2. 当缓冲区满时：                       │
│     - 在内存中排序                       │
│     - 写入临时文件（一个 run）            │
│  3. 重复直到处理完所有数据                │
└─────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│         Phase 2: Multi-way Merge         │
├─────────────────────────────────────────┤
│  1. 打开所有 run 文件                    │
│  2. 从每个 run 读取第一个 tuple          │
│  3. 构建最小堆                           │
│  4. 循环：                               │
│     - 从堆顶取出最小 tuple               │
│     - 从对应 run 读取下一个 tuple        │
│     - 插入堆中                           │
│  5. 堆为空时，排序完成                   │
└─────────────────────────────────────────┘
```

### 5.4 TupleSerializer 实现

**序列化格式**:
```
┌─────────────────────────────────────────┐
│ cell_num (int32_t)                      │
├─────────────────────────────────────────┤
│ TupleCellSpec[0]                        │
│   - table_name_len + table_name         │
│   - field_name_len + field_name         │
│   - alias_len + alias                   │
├─────────────────────────────────────────┤
│ TupleCellSpec[1]                        │
│   ...                                   │
├─────────────────────────────────────────┤
│ Value[0]                                │
│   - AttrType (enum)                     │
│   - is_null (bool)                      │
│   - length (int32_t)                    │
│   - data                                │
├─────────────────────────────────────────┤
│ Value[1]                                │
│   ...                                   │
├─────────────────────────────────────────┤
│ has_order_keys (uint8_t)                │
│ order_key_count (size_t)                │
│ OrderKey[0..N]                          │
└─────────────────────────────────────────┘
```

**关键方法**:
```cpp
class TupleSerializer {
public:
  // 序列化 Tuple 到输出流
  static RC serialize(std::ostream &os, const Tuple *tuple);
  
  // 从输入流反序列化 Tuple
  static RC deserialize(std::istream &is, Tuple *&tuple);
  
  // 估算 Tuple 序列化后的大小
  static size_t estimate_size(const Tuple *tuple);

private:
  // 序列化单个 Value
  static RC serialize_value(std::ostream &os, const Value &value);
  
  // 反序列化单个 Value
  static RC deserialize_value(std::istream &is, Value &value);
};
```

**向量类型序列化**:
```cpp
// 序列化 VECTOR 类型
case AttrType::VECTORS: {
  int vec_length = value.get_vector_length();
  os.write(reinterpret_cast<const char *>(&vec_length), sizeof(vec_length));
  
  std::vector<float> vec = value.get_vector();
  for (int i = 0; i < vec_length; i++) {
    float elem = vec[i];
    os.write(reinterpret_cast<const char *>(&elem), sizeof(elem));
  }
} break;

// 反序列化 VECTOR 类型
case AttrType::VECTORS: {
  int vec_length;
  is.read(reinterpret_cast<char *>(&vec_length), sizeof(vec_length));
  
  std::vector<float> vec;
  vec.reserve(vec_length);
  for (int i = 0; i < vec_length; i++) {
    float elem;
    is.read(reinterpret_cast<char *>(&elem), sizeof(elem));
    vec.push_back(elem);
  }
  value = Value(vec);
} break;
```

### 5.5 OrderByPhysicalOperator 集成

**决策逻辑**:
```cpp
RC OrderByPhysicalOperator::open(Trx *trx) {
  // 1. 估算输入数据量
  size_t estimated_rows = estimate_input_rows();
  size_t estimated_tuple_size = estimate_tuple_size();
  size_t estimated_memory = estimated_rows * estimated_tuple_size;
  
  // 2. 获取内存阈值
  size_t memory_threshold = get_sort_memory_threshold();
  
  // 3. 选择排序算法
  if (estimated_memory > memory_threshold) {
    // 使用外部排序
    LOG_INFO("using external sort: estimated_memory=%lu bytes", 
             estimated_memory);
    use_external_sort_ = true;
    return external_sort_open(trx);
  } else {
    // 使用内存排序
    LOG_INFO("using in-memory sort: estimated_memory=%lu bytes",
             estimated_memory);
    use_external_sort_ = false;
    return memory_sort_open(trx);
  }
}
```

**外部排序启动**:
```cpp
RC OrderByPhysicalOperator::external_sort_open(Trx *trx) {
  // 创建 ExternalSorter
  size_t memory_limit = get_available_memory();
  external_sorter_ = std::make_unique<ExternalSorter>(order_by_, memory_limit);
  
  // 执行排序
  RC rc = external_sorter_->sort(children_[0].get());
  if (rc != RC::SUCCESS) {
    LOG_WARN("external sort failed");
    return rc;
  }
  
  return RC::SUCCESS;
}
```

**next 方法**:
```cpp
RC OrderByPhysicalOperator::next() {
  if (use_external_sort_) {
    // 从外部排序器获取下一个 tuple
    Tuple *tuple = nullptr;
    RC rc = external_sorter_->next(tuple);
    if (rc == RC::SUCCESS) {
      tuple_holder_.reset(tuple);
      tuple_ = tuple;
    }
    return rc;
  } else {
    // 从内存排序结果获取下一个 tuple
    if (sorted_pos_ >= sorted_entries_.size()) {
      return RC::RECORD_EOF;
    }
    tuple_ = sorted_entries_[sorted_pos_].tuple.get();
    sorted_pos_++;
    return RC::SUCCESS;
  }
}
```

---

## 6. RAG 功能实现思路

### 6.1 当前 RAG 集成状态

**已实现**:
- ✅ 向量数据类型（VECTOR）
- ✅ 向量索引（IVF-Flat）
- ✅ ANN 查询算子（VectorScanPhysicalOperator）
- ✅ Langflow 工作流配置（rag/model.json）

**待完善**:
- 🔧 MiniOB 与 Langflow 的通信接口
- 🔧 向量嵌入生成集成
- 🔧 文档分块与预处理
- 🔧 元数据管理

### 6.2 RAG 系统架构设计

```
┌────────────────────────────────────────────────────────┐
│                   Application Layer                     │
│  - Langflow UI                                         │
│  - Chat Interface                                      │
└────────────────┬───────────────────────────────────────┘
                 │
┌────────────────▼───────────────────────────────────────┐
│                  RAG Orchestration                      │
│  - Query Processing                                    │
│  - Context Assembly                                    │
│  - Prompt Engineering                                  │
└────────────────┬──────────────┬─────────────────────────┘
                 │              │
    ┌────────────▼──────┐  ┌───▼────────────────────┐
    │  Embedding Model  │  │   LLM (Language Model) │
    │  (Ollama)         │  │   (Ollama)             │
    └────────────┬──────┘  └────────────────────────┘
                 │
┌────────────────▼───────────────────────────────────────┐
│              MiniOB Vector Database                     │
│  ┌─────────────────────────────────────────────────┐  │
│  │  Documents Table                                │  │
│  │  - id: INT                                      │  │
│  │  - content: TEXT                                │  │
│  │  - metadata: TEXT (JSON)                        │  │
│  │  - embedding: VECTOR(768)                       │  │
│  └─────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────┐  │
│  │  IVF-Flat Index on embedding                    │  │
│  └─────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
```

### 6.3 RAG 工作流实现

#### 6.3.1 文档摄入（Document Ingestion）

```python
def ingest_documents(miniob_client, embedding_model, documents):
    """将文档摄入 MiniOB 向量库"""
    
    for doc in documents:
        # 1. 文档分块
        chunks = split_document(doc, chunk_size=512, overlap=50)
        
        for chunk in chunks:
            # 2. 生成嵌入
            embedding = embedding_model.embed_query(chunk.content)
            
            # 3. 准备元数据
            metadata = json.dumps({
                'source': doc.source,
                'chunk_id': chunk.id,
                'title': doc.title,
                'created_at': datetime.now().isoformat()
            })
            
            # 4. 插入 MiniOB
            sql = f"""
                INSERT INTO documents (content, metadata, embedding)
                VALUES ('{chunk.content}', '{metadata}', '{embedding}')
            """
            miniob_client.execute(sql)
    
    # 5. 确保向量索引已创建
    miniob_client.execute("""
        CREATE VECTOR INDEX IF NOT EXISTS idx_embedding 
        ON documents(embedding) 
        WITH (LISTS=100, DISTANCE='L2')
    """)
```

#### 6.3.2 RAG 查询流程

```python
def rag_query(miniob_client, embedding_model, llm, query, k=5):
    """RAG 查询流程"""
    
    # 1. 生成查询向量
    query_embedding = embedding_model.embed_query(query)
    
    # 2. 检索相关文档（使用 ANN 搜索）
    sql = f"""
        SELECT content, metadata,
               VECTOR_DISTANCE(embedding, '{query_embedding}', 'L2') as distance
        FROM documents
        ORDER BY distance
        LIMIT {k}
    """
    results = miniob_client.execute(sql)
    
    # 3. 构建上下文
    context = "\n\n".join([
        f"[Document {i+1}]\n{r['content']}" 
        for i, r in enumerate(results)
    ])
    
    # 4. 构建提示词
    prompt = f"""
Based on the following context, please answer the question.

Context:
{context}

Question: {query}

Answer:"""
    
    # 5. 调用 LLM 生成回答
    answer = llm.generate(prompt)
    
    return {
        'answer': answer,
        'sources': [json.loads(r['metadata']) for r in results]
    }
```

### 6.4 MiniOB 向量存储接口封装

**Python 客户端封装**:

```python
import socket
import json

class MiniOBVectorStore:
    """MiniOB 向量存储客户端"""
    
    def __init__(self, host='127.0.0.1', port=6789, 
                 table_name='documents'):
        self.host = host
        self.port = port
        self.table_name = table_name
        self.sock = None
    
    def connect(self):
        """连接到 MiniOB"""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
    
    def execute(self, sql):
        """执行 SQL 查询"""
        # 发送 SQL
        self.sock.sendall(sql.encode('utf-8') + b'\n')
        
        # 接收结果
        response = b''
        while True:
            chunk = self.sock.recv(4096)
            if not chunk:
                break
            response += chunk
            if b'\n' in chunk:
                break
        
        return self._parse_response(response.decode('utf-8'))
    
    def add_documents(self, documents, embeddings, metadatas=None):
        """批量添加文档"""
        for i, (doc, emb) in enumerate(zip(documents, embeddings)):
            metadata = metadatas[i] if metadatas else '{}'
            sql = f"""
                INSERT INTO {self.table_name} (content, metadata, embedding)
                VALUES ('{doc}', '{metadata}', '{emb}')
            """
            self.execute(sql)
    
    def similarity_search(self, query_embedding, k=5):
        """相似度搜索"""
        sql = f"""
            SELECT id, content, metadata,
                   VECTOR_DISTANCE(embedding, '{query_embedding}', 'L2') as distance
            FROM {self.table_name}
            ORDER BY distance
            LIMIT {k}
        """
        return self.execute(sql)
    
    def create_index(self, lists=100, distance='L2'):
        """创建向量索引"""
        sql = f"""
            CREATE VECTOR INDEX idx_{self.table_name}_embedding
            ON {self.table_name}(embedding)
            WITH (LISTS={lists}, DISTANCE='{distance}')
        """
        self.execute(sql)
    
    def close(self):
        """关闭连接"""
        if self.sock:
            self.sock.close()
```

### 6.5 Langflow 自定义组件

**MiniOB VectorStore 组件**:

```python
# 文件: miniob_vectorstore_component.py

from langflow.base.vectorstores.model import LCVectorStoreComponent
from langflow.io import HandleInput, IntInput, StrInput, FloatInput
from langchain_core.documents import Document
from langchain_core.embeddings import Embeddings
from typing import List
import socket
import json

class MiniOBVectorStoreComponent(LCVectorStoreComponent):
    display_name = "MiniOB Vector Store"
    description = "MiniOB vector store component."
    icon = "database"
    name = "MiniOBVectorStore"
    
    inputs = [
        StrInput(
            name="host",
            display_name="Host",
            value="127.0.0.1",
            info="MiniOB server host"
        ),
        IntInput(
            name="port",
            display_name="Port",
            value=6789,
            info="MiniOB server port"
        ),
        StrInput(
            name="table_name",
            display_name="Table Name",
            value="documents",
            info="Table name for storing documents"
        ),
        HandleInput(
            name="embedding_model",
            display_name="Embedding Model",
            input_types=["Embeddings"],
            info="Embedding model to use"
        ),
        IntInput(
            name="lists",
            display_name="IVF Lists",
            value=100,
            advanced=True,
            info="Number of clusters for IVF index"
        ),
        StrInput(
            name="distance",
            display_name="Distance Metric",
            value="L2",
            advanced=True,
            info="Distance metric (L2, COSINE, IP)"
        )
    ]
    
    def build_vector_store(self):
        """构建 MiniOB 向量存储"""
        return MiniOBVectorStore(
            host=self.host,
            port=self.port,
            table_name=self.table_name,
            embedding_function=self.embedding_model,
            lists=self.lists,
            distance=self.distance
        )
    
    def add_documents(self, documents: List[Document]):
        """添加文档到向量库"""
        vector_store = self.build_vector_store()
        vector_store.add_documents(documents)
    
    def similarity_search(self, query: str, k: int = 5) -> List[Document]:
        """相似度搜索"""
        vector_store = self.build_vector_store()
        return vector_store.similarity_search(query, k=k)
```

### 6.6 RAG 功能扩展建议

#### 6.6.1 混合搜索（Hybrid Search）

结合关键词搜索和向量搜索：

```sql
-- 1. 全文搜索（可使用 LIKE 或将来的全文索引）
SELECT id, content, 0 as vector_distance
FROM documents
WHERE content LIKE '%keyword%'

UNION

-- 2. 向量搜索
SELECT id, content,
       VECTOR_DISTANCE(embedding, '[query_vector]', 'L2') as vector_distance
FROM documents
ORDER BY VECTOR_DISTANCE(embedding, '[query_vector]', 'L2')
LIMIT 10

-- 3. 合并结果并重新排序（可在应用层实现）
```

#### 6.6.2 元数据过滤

支持基于元数据的过滤查询：

```sql
-- 创建带元数据列的表
CREATE TABLE documents_with_metadata (
  id INT,
  content TEXT,
  source VARCHAR(255),
  author VARCHAR(100),
  created_at DATE,
  embedding VECTOR(768)
);

-- 带过滤的向量搜索
SELECT id, content, source, author,
       VECTOR_DISTANCE(embedding, '[query_vector]', 'L2') as distance
FROM documents_with_metadata
WHERE source = 'technical_docs'
  AND created_at >= '2024-01-01'
ORDER BY distance
LIMIT 5;
```

#### 6.6.3 多模态支持

扩展向量存储以支持多模态数据：

```sql
-- 多模态文档表
CREATE TABLE multimodal_documents (
  id INT,
  doc_type VARCHAR(50),  -- 'text', 'image', 'audio'
  content TEXT,
  content_url VARCHAR(500),
  text_embedding VECTOR(768),
  image_embedding VECTOR(512),
  metadata TEXT
);

-- 多模态查询（文本-图像跨模态检索）
SELECT id, content, content_url,
       VECTOR_DISTANCE(image_embedding, '[text_query_vector]', 'COSINE') as distance
FROM multimodal_documents
WHERE doc_type = 'image'
ORDER BY distance
LIMIT 10;
```

#### 6.6.4 向量索引优化

**动态参数调整**:
```sql
-- 调整 PROBES 参数以平衡精度和速度
SET ivfflat_probes = 10;  -- 探测 10 个聚类

-- 重建索引以优化性能
DROP INDEX idx_embedding;
CREATE VECTOR INDEX idx_embedding ON documents(embedding)
  WITH (LISTS=200, DISTANCE='COSINE');
```

**索引监控**:
```cpp
// 在 IvfflatIndex 中添加统计信息
struct IndexStats {
  size_t total_vectors;
  size_t num_centroids;
  double avg_cluster_size;
  double index_build_time;
  double avg_search_time;
};

IndexStats IvfflatIndex::get_stats() const {
  // 返回索引统计信息
}
```

---

## 7. 总结与展望

### 7.1 当前架构优势

1. **模块化设计**: 各模块职责清晰，易于扩展
2. **向量支持完善**: 原生支持向量类型和向量索引
3. **外部排序实现**: 可处理超大数据集的排序
4. **RAG 集成友好**: 已有 Langflow 工作流配置

### 7.2 可扩展方向

#### 数据库内核
- [ ] 全文索引支持
- [ ] 更多向量索引算法（HNSW, PQ 等）
- [ ] 查询优化器增强
- [ ] 并发控制优化

#### RAG 功能
- [ ] 混合搜索实现
- [ ] 元数据丰富查询
- [ ] 多模态支持
- [ ] 向量索引自动调优

#### 性能优化
- [ ] 向量索引压缩（PQ, OPQ）
- [ ] GPU 加速向量计算
- [ ] 分布式向量索引
- [ ] 缓存优化

### 7.3 实现 RAG 功能的关键任务

**任务清单**:

1. **基础设施** (已完成 ✅)
   - [x] 向量数据类型
   - [x] 向量索引（IVF-Flat）
   - [x] ANN 查询算子
   - [x] Tuple 序列化（含向量）

2. **接口封装** (待完成 🔧)
   - [ ] Python 客户端封装
   - [ ] Langflow 自定义组件
   - [ ] REST API 接口

3. **文档处理** (待完成 🔧)
   - [ ] 文档分块逻辑
   - [ ] 嵌入生成集成
   - [ ] 元数据提取与存储

4. **查询优化** (待完成 🔧)
   - [ ] 混合搜索实现
   - [ ] 元数据过滤
   - [ ] 查询结果重排序

5. **工作流集成** (部分完成 ✅)
   - [x] Langflow 配置文件
   - [ ] 端到端测试
   - [ ] 性能调优

---

## 附录

### A. 编译和运行

**编译**:
```bash
# 标准编译
bash build.sh release

# 启用 MemTracer（用于外部排序测试）
bash build.sh release -DWITH_MEMTRACER=ON

# Debug 模式
bash build.sh debug
```

**运行**:
```bash
# 启动 Observer（服务端）
./build/bin/observer

# 启动客户端
./build/bin/obclient

# 使用 MemTracer 运行
MT_MEMORY_LIMIT=33554432 LD_PRELOAD=./build/lib/libmemtracer.so \
  ./build/bin/observer
```

### B. SQL 示例

**创建向量表**:
```sql
CREATE TABLE documents (
  id INT,
  title CHARS(100),
  content TEXT,
  embedding VECTOR(768)
);
```

**创建向量索引**:
```sql
CREATE VECTOR INDEX idx_embedding ON documents(embedding)
  WITH (LISTS=100, DISTANCE='L2');
```

**插入数据**:
```sql
INSERT INTO documents VALUES (
  1,
  'Sample Document',
  'This is a sample document content.',
  '[0.1, 0.2, ..., 0.768]'
);
```

**向量搜索**:
```sql
SELECT id, title,
       VECTOR_DISTANCE(embedding, '[query_vector]', 'L2') as distance
FROM documents
ORDER BY distance
LIMIT 5;
```

### C. 参考资源

- **MiniOB 官方文档**: https://oceanbase.github.io/miniob/
- **GitHub 仓库**: https://github.com/oceanbase/miniob
- **MemTracer 文档**: https://oceanbase.github.io/miniob/game/miniob-memtracer/
- **Langflow 官方文档**: https://docs.langflow.org/

---

**文档创建**: 2025-11-01  
**作者**: AI Assistant  
**版本**: 1.0  
**目的**: 理解 MiniOB 架构，为实现 RAG 功能提供指导

