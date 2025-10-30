# MiniOB 架构分析与理解

> 本文档旨在理解MiniOB项目架构，为实现big order by（外部排序）做准备

## 1. 项目整体架构

MiniOB 是一个教学型数据库管理系统，其架构设计简洁清晰，主要包含以下模块：

```
┌─────────────────────────────────────────────────────────────┐
│                      Client (obclient)                       │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                   Network Service (net)                      │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                    Session Management                         │
└──────────────────────────┬──────────────────────────────────┘
                           │
        ┌──────────────────┴──────────────────┬───────────────┐
        │                                     │               │
┌───────▼────────┐  ┌───────────────────┐  ┌▼──────────┐  ┌──▼─────┐
│   SQL Parser   │  │  Semantic Resolver │  │ Optimizer │  │Executor│
│   (Parser)     │─▶│    (Resolver)      │─▶│           │─▶│        │
└────────────────┘  └───────────────────┘  └───────────┘  └────┬───┘
                                                                 │
                    ┌────────────────────────────────────────────┘
                    │
        ┌───────────▼───────────────┬───────────────────────┐
        │                           │                       │
┌───────▼────────┐  ┌───────────────▼─┐  ┌─────────────────▼──────┐
│ Storage Engine │  │  MVCC/Trx Mgr   │  │   Record Manager       │
│ (Buffer Pool)  │  │  (Transaction)  │  │   (record_manager)     │
└───────┬────────┘  └─────────────────┘  └────────────────────────┘
        │
┌───────▼────────────────────────────────────────────────────────┐
│                    Disk I/O & File System                       │
└─────────────────────────────────────────────────────────────────┘
```

## 2. 核心模块详解

### 2.1 SQL执行流程 (src/observer/sql/)

#### SQL Parser (sql/parser/)
- **功能**: 将SQL文本解析成抽象语法树(AST)
- **关键文件**: 
  - `parse.h/cpp`: 解析入口
  - `lex_sql.l`: 词法分析
  - `yacc_sql.y`: 语法分析

#### Executor (sql/executor/)
- **功能**: 执行具体的SQL命令
- **关键类**:
  - `CommandExecutor`: 命令执行器基类
  - `ExecuteStage`: 执行阶段管理

#### Operator (sql/operator/)
物理算子和逻辑算子，是查询执行的核心：

**逻辑算子 (Logical Operators)**:
- 描述"做什么"
- 例如: `OrderByLogicalOperator`, `TableGetLogicalOperator`

**物理算子 (Physical Operators)**:
- 描述"怎么做"
- 每个算子实现三个核心接口:
  - `open(Trx *trx)`: 打开算子，初始化资源
  - `next()`: 获取下一条记录
  - `close()`: 关闭算子，释放资源

**关键物理算子**:
- `TableScanPhysicalOperator`: 表扫描
- `OrderByPhysicalOperator`: 排序（**当前为内存排序**）
- `ProjectPhysicalOperator`: 投影
- `PredicatePhysicalOperator`: 谓词过滤
- `JoinPhysicalOperator`: 连接

### 2.2 存储引擎 (src/observer/storage/)

#### Buffer Pool (storage/buffer/)
**核心类**:
- `DiskBufferPool`: 磁盘缓冲池
- `BPFrameManager`: 页帧管理器
- `Frame`: 内存页帧

**关键概念**:
- **Page**: 磁盘上的数据单位，固定大小(默认8KB)
- **Frame**: 内存中的页帧，用于缓存磁盘页
- **PageNum**: 页号，用于标识页面
- **LRU Cache**: 用于页面淘汰

**核心接口**:
```cpp
class DiskBufferPool {
  RC get_this_page(PageNum page_num, Frame **frame);  // 获取指定页面
  RC allocate_page(Frame **frame);                     // 分配新页面
  RC dispose_page(PageNum page_num);                   // 释放页面
  RC flush_page(Frame &frame);                         // 刷新页面到磁盘
  RC unpin_page(Frame *frame);                         // 解除页面锁定
};
```

#### Record Manager (storage/record/)
**功能**: 管理表记录的增删改查

**核心类**:
- `RecordFileHandler`: 管理整个文件中的记录
- `RecordPageHandler`: 管理单个页面中的记录
- `RecordFileScanner`: 遍历文件中的所有记录
- `RecordPageIterator`: 遍历页面中的所有记录

**页面组织**:
```
┌──────────────────────────────────────────────────────┐
│ PageHeader | record allocate bitmap                  │
├──────────────────────────────────────────────────────┤
│ record1 | record2 | record3 | ... | recordN         │
└──────────────────────────────────────────────────────┘
```

**PageHeader结构**:
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

#### Table (storage/table/)
**功能**: 表的抽象，管理表的元数据和操作

**核心接口**:
```cpp
class Table {
  RC insert_record(Record &record);
  RC delete_record(const RID &rid);
  RC update_record(const Record &old_record, const Record &new_record);
  RC get_record(const RID &rid, Record &record);
  RC get_record_scanner(RecordFileScanner &scanner, Trx *trx, ReadWriteMode mode);
};
```

### 2.3 数据结构 (common/ & sql/expr/)

#### Tuple (sql/expr/tuple.h)
**功能**: 表示一行数据的抽象

**核心接口**:
```cpp
class Tuple {
  virtual int cell_num() const = 0;
  virtual RC cell_at(int index, Value &cell) const = 0;
  virtual Tuple *copy() const = 0;
  virtual RC compare(const Tuple &other, int &result) const;
};
```

**主要实现**:
- `RowTuple`: 表示一行记录
- `ValueListTuple`: 值列表tuple
- `JoinedTuple`: 连接后的tuple
- `ProjectTuple`: 投影后的tuple

#### Value (common/value.h)
表示一个字段的值

#### Record (storage/record/record.h)
表示一条物理记录，包含RID和数据

#### RID (Record Identifier)
```cpp
struct RID {
  PageNum page_num;  // 页号
  SlotNum slot_num;  // 槽位号
};
```

### 2.4 返回码 (common/rc.h)
使用枚举类 `RC` 定义所有返回码：
- `RC::SUCCESS`: 成功
- `RC::RECORD_EOF`: 记录结束
- `RC::NOMEM`: 内存不足
- 等等...

## 3. 当前 ORDER BY 实现分析

### 3.1 现有实现 (order_by_physical_operator.cpp)

**算法**: **内存排序**
- 使用 `std::priority_queue` (最大堆/最小堆)
- 在 `open()` 阶段通过 `fetch_and_sort_tables()` 一次性将所有数据加载到内存

**核心数据结构**:
```cpp
using order_line = pair<vector<Value>, Tuple *>;
using order_func = std::function<bool(const order_line &, const order_line &)>;
using order_list = std::priority_queue<order_line, vector<order_line>, order_func>;

order_list order_and_field_line;  // 存储所有排序数据
```

**执行流程**:
```cpp
RC OrderByPhysicalOperator::open(Trx *trx) {
  // 1. 打开子算子
  children_[0]->open(trx);
  
  // 2. 提取所有数据并排序
  fetch_and_sort_tables();  // 关键：全部加载到内存！
  
  return RC::SUCCESS;
}

RC OrderByPhysicalOperator::next() {
  if (order_and_field_line.empty()) {
    return RC::RECORD_EOF;
  }
  
  // 从优先队列中取出下一条
  tuple_ = order_and_field_line.top().second;
  order_and_field_line.pop();
  return RC::SUCCESS;
}
```

**问题**:
1. ❌ **内存限制**: 所有数据必须加载到内存中
2. ❌ **无法处理大数据集**: 当数据量超过可用内存时会失败
3. ❌ **启动延迟**: 必须等待所有数据加载完成才能返回第一条记录

## 4. Big Order By 需求分析

### 4.1 外部排序 (External Sorting)

**核心思想**: 当数据量超过内存容量时，使用磁盘辅助排序

**经典算法**: 多路归并排序 (Multi-way Merge Sort)

**步骤**:
1. **生成阶段 (Run Generation)**:
   - 读取能放入内存的数据块
   - 在内存中排序
   - 写入临时文件（称为一个"run"）
   - 重复直到处理完所有数据

2. **归并阶段 (Merge)**:
   - 同时打开多个已排序的run文件
   - 使用最小堆从各个run中选择最小值
   - 将结果写入新的run或最终输出
   - 如果run数量过多，可能需要多轮归并

### 4.2 设计要点

#### 4.2.1 内存管理
- 需要设置**内存限制**（可通过环境变量或配置）
- 动态分配内存给:
  - 输入缓冲区
  - 排序缓冲区
  - 输出缓冲区

#### 4.2.2 临时文件管理
- 创建临时文件存储中间结果
- 需要考虑:
  - 临时文件命名规则
  - 清理机制（失败/成功后）
  - 磁盘空间检查

#### 4.2.3 性能优化
- **Run大小**: 尽可能大的run以减少归并轮数
- **归并路数**: 权衡内存使用和I/O次数
- **缓冲区大小**: 减少系统调用次数
- **预读**: 利用操作系统的预读功能

### 4.3 与现有架构的集成

**需要修改/扩展的组件**:

1. **OrderByPhysicalOperator** (主要修改):
   - 检测数据量是否超过内存限制
   - 实现外部排序逻辑
   - 管理临时文件生命周期

2. **临时文件管理**:
   - 可能需要创建 `TempFileManager` 类
   - 利用现有的 `DiskBufferPool` 或直接使用文件I/O

3. **内存监控**:
   - 使用 MemTracer 监控内存使用

## 5. MemTracer 使用指南

### 5.1 MemTracer 简介

MemTracer 是MiniOB提供的内存监控工具，用于:
- 监控进程内存使用情况
- 设置内存上限
- 在内存受限环境下测试和调试

### 5.2 编译配置

**默认情况**: MemTracer 不会被编译（与sanitizers冲突）

```bash
# 编译时启用MemTracer
bash build.sh release -DWITH_MEMTRACER=ON

# 编译时禁用MemTracer（默认）
bash build.sh release -DWITH_MEMTRACER=OFF
```

**输出位置**: `${CMAKE_BINARY_DIR}/lib/libmemtracer.so`

### 5.3 运行时使用

#### 基本使用
```bash
# 通过LD_PRELOAD加载MemTracer
LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```

#### 环境变量配置

**MT_PRINT_INTERVAL_MS**: 内存使用打印间隔（毫秒）
```bash
MT_PRINT_INTERVAL_MS=1000 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```

**MT_MEMORY_LIMIT**: 内存使用上限（字节）
```bash
# 设置100MB内存限制
MT_MEMORY_LIMIT=104857600 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```

**组合使用**:
```bash
# 1秒打印一次，100MB限制
MT_PRINT_INTERVAL_MS=1000 MT_MEMORY_LIMIT=104857600 \
  LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```

### 5.4 程序内部使用

**头文件**: `memtracer/mt_info.h`

```cpp
#include "memtracer/mt_info.h"

// 获取当前内存使用量（字节）
size_t current_mem = memtracer::allocated_memory();

// 获取元数据内存使用量
size_t meta_mem = memtracer::meta_memory();

// 获取内存限制
size_t limit = memtracer::memory_limit();

// 使用示例
LOG_INFO("Current memory usage: %lu bytes", memtracer::allocated_memory());
```

**链接MemTracer**:
```cmake
# 在CMakeLists.txt中添加
target_link_libraries(your_target memtracer)
```

### 5.5 注意事项

⚠️ **重要限制**:
1. **不支持与sanitizers一起使用** (ASAN/TSAN/UBSAN)
2. **不建议使用mmap管理内存** (会记录整个虚拟内存占用)
3. **不允许绕过常规内存分配** (不能用brk/sbrk/syscall)
4. **会影响性能**: 内存分配/释放函数性能下降约4倍

**内存统计包含**:
- 动态内存分配 (malloc/free, new/delete)
- 进程代码段等内存占用
- mmap映射的虚拟内存

### 5.6 使用场景

#### 场景1: 模拟内存受限环境
```bash
# 设置32MB内存限制测试外部排序
MT_MEMORY_LIMIT=33554432 LD_PRELOAD=./lib/libmemtracer.so \
  ./bin/observer
```

当超出限制时，进程会退出并打印:
```
[MEMTRACER] alloc memory:24, allocated_memory: 31653580, 
memory_limit: 31653600, Memory limit exceeded!
```

#### 场景2: 调试内存使用
```cpp
// 在关键位置检查内存使用
RC OrderByPhysicalOperator::fetch_and_sort_tables() {
  LOG_INFO("Memory before sort: %lu", memtracer::allocated_memory());
  
  // 排序逻辑...
  
  LOG_INFO("Memory after sort: %lu", memtracer::allocated_memory());
  
  return RC::SUCCESS;
}
```

## 6. 实现Big Order By的技术路径

### 6.1 设计方案

**方案选择**: 实现自适应排序
- 数据量小 → 使用当前的内存排序
- 数据量大 → 切换到外部排序

### 6.2 实现步骤

#### Step 1: 内存估算
```cpp
// 估算所需内存
size_t estimated_memory = record_count * avg_record_size;
size_t available_memory = get_available_memory();

if (estimated_memory > available_memory) {
  // 使用外部排序
} else {
  // 使用内存排序
}
```

#### Step 2: Run生成
```cpp
class RunGenerator {
  RC generate_runs(PhysicalOperator *child, 
                   vector<string> &run_files);
  
private:
  size_t memory_limit_;
  vector<Tuple *> buffer_;
};
```

#### Step 3: 多路归并
```cpp
class RunMerger {
  RC merge(const vector<string> &run_files, 
           PhysicalOperator *output);
  
private:
  struct RunReader {
    ifstream file;
    Tuple *current_tuple;
  };
  
  priority_queue<RunReader> merge_heap_;
};
```

#### Step 4: 临时文件管理
```cpp
class TempFileManager {
  string create_temp_file();
  RC write_tuple(const string &file, Tuple *tuple);
  RC read_tuple(const string &file, Tuple *&tuple);
  RC cleanup_temp_files();
};
```

### 6.3 数据序列化

**需要序列化Tuple到磁盘**:
```cpp
// Tuple序列化格式
struct TupleFormat {
  uint32_t cell_count;
  struct Cell {
    AttrType type;
    uint32_t length;
    char data[length];
  } cells[];
};

// 实现
RC serialize_tuple(ostream &os, const Tuple *tuple);
RC deserialize_tuple(istream &is, Tuple *&tuple);
```

### 6.4 集成到OrderByPhysicalOperator

```cpp
class OrderByPhysicalOperator : public PhysicalOperator {
public:
  RC open(Trx *trx) override {
    // 1. 估算内存需求
    // 2. 选择排序策略
    if (use_external_sort_) {
      return external_sort_open(trx);
    } else {
      return memory_sort_open(trx);
    }
  }
  
  RC next() override {
    if (use_external_sort_) {
      return external_sort_next();
    } else {
      return memory_sort_next();
    }
  }
  
private:
  bool use_external_sort_ = false;
  unique_ptr<RunMerger> merger_;
  vector<string> temp_files_;
};
```

## 7. 关键技术点总结

### 7.1 磁盘I/O
- 使用缓冲I/O减少系统调用
- 考虑使用 `DiskBufferPool` 或标准文件I/O
- 注意文件同步和错误处理

### 7.2 内存管理
- 使用MemTracer监控内存
- 实现内存配额管理
- 及时释放不用的资源

### 7.3 错误处理
- 磁盘空间不足
- 内存分配失败
- 临时文件创建/读写失败
- 清理临时文件

### 7.4 测试策略
- 小数据集（内存排序）
- 大数据集（外部排序）
- 内存限制场景（MemTracer）
- 边界条件（空结果、单条记录）

## 8. 参考资料

### 8.1 项目文档
- [MiniOB架构文档](https://oceanbase.github.io/miniob/design/miniob-architecture/)
- [MemTracer使用文档](https://oceanbase.github.io/miniob/game/miniob-memtracer/)

### 8.2 相关算法
- 外部排序算法 (External Sorting)
- 多路归并排序 (Multi-way Merge Sort)
- 替换选择排序 (Replacement Selection)

### 8.3 数据库系统实现
- 《Database System Implementation》
- 《Database Internals》

## 9. 下一步行动

✅ **已完成**:
- [x] 理解MiniOB整体架构
- [x] 分析当前ORDER BY实现
- [x] 学习MemTracer使用方法

🚀 **待实现**:
- [ ] 设计外部排序详细方案
- [ ] 实现Tuple序列化/反序列化
- [ ] 实现Run生成器
- [ ] 实现多路归并器
- [ ] 集成到OrderByPhysicalOperator
- [ ] 编写测试用例
- [ ] 性能优化和调优

---

**文档创建时间**: 2025-10-30
**目标**: 为实现big order by（外部排序）提供架构理解和技术指导

