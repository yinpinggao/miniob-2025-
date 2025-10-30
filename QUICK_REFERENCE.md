# MiniOB Quick Reference - 外部排序实现指南

## 📋 项目架构速查

### 关键目录结构
```
miniob/
├── src/observer/
│   ├── sql/
│   │   ├── operator/          # 物理/逻辑算子
│   │   │   ├── order_by_physical_operator.{h,cpp}  ⭐ 需要修改
│   │   │   ├── table_scan_physical_operator.{h,cpp}
│   │   │   └── physical_operator.h
│   │   ├── expr/              # 表达式和Tuple
│   │   │   └── tuple.h        ⭐ 需要序列化
│   │   └── executor/          # SQL执行器
│   └── storage/
│       ├── buffer/            # 缓冲池管理
│       │   ├── disk_buffer_pool.{h,cpp}  ⭐ 磁盘I/O
│       │   └── frame.{h,cpp}
│       ├── record/            # 记录管理
│       │   └── record_manager.{h,cpp}
│       └── table/             # 表管理
│           └── table.h
├── deps/memtracer/            # 内存监控工具
│   └── mt_info.h              ⭐ 内存监控API
└── scripts/                   # 实用脚本
    ├── run_with_memtracer.sh  # 启动MemTracer
    └── test_external_sort.sh  # 测试外部排序
```

## 🔑 核心接口速查

### Physical Operator 接口
```cpp
class PhysicalOperator {
  virtual RC open(Trx *trx) = 0;     // 初始化
  virtual RC next() = 0;              // 获取下一条记录
  virtual RC close() = 0;             // 清理资源
  virtual Tuple *current_tuple();     // 当前Tuple
};
```

### Tuple 接口
```cpp
class Tuple {
  virtual int cell_num() const = 0;
  virtual RC cell_at(int index, Value &cell) const = 0;
  virtual Tuple *copy() const = 0;
  virtual RC compare(const Tuple &other, int &result) const;
};
```

### DiskBufferPool 接口
```cpp
class DiskBufferPool {
  RC get_this_page(PageNum page_num, Frame **frame);
  RC allocate_page(Frame **frame);
  RC dispose_page(PageNum page_num);
  RC flush_page(Frame &frame);
  RC unpin_page(Frame *frame);
};
```

### MemTracer API
```cpp
#include "memtracer/mt_info.h"

namespace memtracer {
  size_t allocated_memory();  // 当前内存使用量（字节）
  size_t meta_memory();       // 元数据内存
  size_t memory_limit();      // 内存限制
}
```

## 🛠️ 编译和运行

### 编译项目
```bash
# 标准编译（不带MemTracer）
bash build.sh release

# 编译并启用MemTracer
bash build.sh release -DWITH_MEMTRACER=ON

# Debug模式编译
bash build.sh debug -DWITH_MEMTRACER=ON
```

### 运行Observer

#### 不使用MemTracer
```bash
./build/bin/observer
```

#### 使用MemTracer（手动）
```bash
# 基本使用
LD_PRELOAD=./build/lib/libmemtracer.so ./build/bin/observer

# 设置100MB内存限制
MT_MEMORY_LIMIT=104857600 LD_PRELOAD=./build/lib/libmemtracer.so \
  ./build/bin/observer

# 每1秒打印一次，32MB限制
MT_PRINT_INTERVAL_MS=1000 MT_MEMORY_LIMIT=33554432 \
  LD_PRELOAD=./build/lib/libmemtracer.so ./build/bin/observer
```

#### 使用脚本（推荐）
```bash
# 设置100MB内存限制，每1秒打印
./scripts/run_with_memtracer.sh -m 104857600 -i 1000

# 设置32MB限制
./scripts/run_with_memtracer.sh -m 33554432

# 查看帮助
./scripts/run_with_memtracer.sh -h
```

### 常用内存大小
| 大小 | 字节数 |
|------|--------|
| 32MB | 33554432 |
| 64MB | 67108864 |
| 100MB | 104857600 |
| 128MB | 134217728 |
| 256MB | 268435456 |
| 512MB | 536870912 |
| 1GB | 1073741824 |

## 📊 外部排序实现要点

### 1. 算法概述

**Multi-way Merge Sort (多路归并排序)**

#### Phase 1: Run Generation (生成阶段)
```
内存 ──┐
       ├─→ [读取] → [排序] → [写入Run1.tmp]
       ├─→ [读取] → [排序] → [写入Run2.tmp]
       └─→ [读取] → [排序] → [写入Run3.tmp]
磁盘 ←─┘
```

#### Phase 2: Merge (归并阶段)
```
Run1.tmp ─┐
Run2.tmp ─┼─→ [最小堆] → [输出]
Run3.tmp ─┘
```

### 2. 核心数据结构

```cpp
// Run生成器
class RunGenerator {
public:
  RC generate(PhysicalOperator *input, 
              vector<string> &run_files,
              size_t memory_limit);
  
private:
  vector<Tuple *> buffer_;
  size_t current_memory_;
};

// Run归并器
class RunMerger {
public:
  RC init(const vector<string> &run_files);
  RC next(Tuple *&tuple);
  RC close();
  
private:
  struct RunReader {
    ifstream file;
    Tuple *current;
    bool has_next;
  };
  
  vector<RunReader> readers_;
  priority_queue<...> merge_heap_;
};

// Tuple序列化
RC serialize_tuple(ostream &os, const Tuple *tuple);
RC deserialize_tuple(istream &is, Tuple *&tuple);
```

### 3. 改造OrderByPhysicalOperator

```cpp
class OrderByPhysicalOperator : public PhysicalOperator {
public:
  RC open(Trx *trx) override {
    // 估算数据量
    size_t estimated = estimate_data_size();
    size_t available = get_available_memory();
    
    if (estimated > available * 0.8) {
      // 使用外部排序
      use_external_sort_ = true;
      return external_sort_open(trx);
    } else {
      // 使用内存排序（现有逻辑）
      use_external_sort_ = false;
      return memory_sort_open(trx);
    }
  }
  
  RC next() override {
    if (use_external_sort_) {
      return merger_->next(current_tuple_);
    } else {
      // 现有逻辑
      return memory_sort_next();
    }
  }
  
  RC close() override {
    if (use_external_sort_) {
      cleanup_temp_files();
    }
    return children_[0]->close();
  }
  
private:
  bool use_external_sort_ = false;
  unique_ptr<RunMerger> merger_;
  vector<string> temp_files_;
};
```

### 4. 临时文件管理

```cpp
class TempFileManager {
public:
  // 创建临时文件
  string create_temp_file(const string &prefix = "miniob_sort");
  
  // 注册临时文件（用于清理）
  void register_file(const string &path);
  
  // 清理所有临时文件
  void cleanup_all();
  
  // 析构时自动清理
  ~TempFileManager();
  
private:
  vector<string> temp_files_;
};

// 使用示例
TempFileManager temp_mgr;
string run_file = temp_mgr.create_temp_file("run");
// ... 使用文件 ...
temp_mgr.cleanup_all();  // 清理
```

### 5. 内存监控集成

```cpp
RC OrderByPhysicalOperator::fetch_and_sort_tables() {
  LOG_INFO("Memory before sort: %lu bytes", 
           memtracer::allocated_memory());
  
  size_t limit = memtracer::memory_limit();
  if (limit > 0) {
    size_t current = memtracer::allocated_memory();
    size_t available = limit - current;
    
    if (available < MIN_REQUIRED_MEMORY) {
      // 强制使用外部排序
      return external_sort();
    }
  }
  
  // 正常排序逻辑
  // ...
  
  LOG_INFO("Memory after sort: %lu bytes", 
           memtracer::allocated_memory());
}
```

## 🧪 测试策略

### 1. 单元测试
```cpp
// 测试Tuple序列化
TEST(TupleSerializeTest, BasicTypes) {
  RowTuple tuple;
  // ... 设置数据 ...
  
  stringstream ss;
  ASSERT_EQ(RC::SUCCESS, serialize_tuple(ss, &tuple));
  
  Tuple *deserialized = nullptr;
  ASSERT_EQ(RC::SUCCESS, deserialize_tuple(ss, deserialized));
  
  // 验证内容一致
  // ...
}

// 测试Run生成
TEST(RunGeneratorTest, GenerateSingleRun) {
  // ...
}

// 测试多路归并
TEST(RunMergerTest, MergeTwoRuns) {
  // ...
}
```

### 2. 集成测试
```sql
-- 小数据集（内存排序）
CREATE TABLE small_test (id INT, value INT);
-- 插入100条记录
SELECT * FROM small_test ORDER BY value;

-- 大数据集（外部排序）
CREATE TABLE large_test (id INT, value INT);
-- 插入100000条记录
SELECT * FROM large_test ORDER BY value;
```

### 3. 压力测试
```bash
# 32MB内存限制，10万条记录
./scripts/test_external_sort.sh -n 100000 -m 32

# 64MB内存限制，50万条记录
./scripts/test_external_sort.sh -n 500000 -m 64
```

## 🐛 调试技巧

### 1. 启用调试日志
```cpp
LOG_DEBUG("Run file created: %s, size: %lu", 
          run_file.c_str(), file_size);
LOG_TRACE("Merging tuple: %s", tuple->to_string().c_str());
```

### 2. 内存泄漏检查
```bash
# 使用valgrind
valgrind --leak-check=full ./build/bin/observer

# 注意：不能同时使用ASAN和MemTracer
```

### 3. 性能分析
```bash
# 使用gprof
g++ -pg ...
./build/bin/observer
gprof ./build/bin/observer gmon.out > analysis.txt
```

### 4. MemTracer调试
```cpp
// 在关键位置检查内存
#define CHECK_MEMORY(msg) \
  LOG_INFO("[MEMORY] %s: allocated=%lu, limit=%lu", \
           msg, \
           memtracer::allocated_memory(), \
           memtracer::memory_limit())

CHECK_MEMORY("Before loading data");
// ... 加载数据 ...
CHECK_MEMORY("After loading data");
```

## 📝 常见问题

### Q1: MemTracer编译失败？
```bash
# 确保关闭sanitizers
bash build.sh release -DWITH_MEMTRACER=ON -DENABLE_ASAN=OFF
```

### Q2: 临时文件存放在哪里？
```cpp
// 推荐使用/tmp目录
string temp_file = "/tmp/miniob_sort_" + to_string(getpid()) + "_" + 
                   to_string(run_id) + ".tmp";
```

### Q3: 如何选择归并路数？
```cpp
// 假设每个run reader需要buffer_size内存
size_t available_memory = ...;
size_t buffer_size = 4096;  // 4KB per reader
size_t max_merge_way = available_memory / buffer_size;

// 通常选择8-32路
size_t merge_way = min(max_merge_way, run_count);
merge_way = max(2, merge_way);  // 至少2路
```

### Q4: 内存估算不准确怎么办？
```cpp
// 保守估计，使用安全系数
const float SAFETY_FACTOR = 0.7;
size_t safe_memory = available_memory * SAFETY_FACTOR;
```

## 🎯 实现检查清单

### Phase 1: 基础设施
- [ ] Tuple序列化/反序列化
- [ ] 临时文件管理器
- [ ] 内存使用估算函数

### Phase 2: Run生成
- [ ] RunGenerator类实现
- [ ] 内存缓冲区管理
- [ ] Run写入磁盘

### Phase 3: Run归并
- [ ] RunReader类实现
- [ ] 多路归并堆
- [ ] 结果输出

### Phase 4: 集成
- [ ] 修改OrderByPhysicalOperator
- [ ] 自适应选择排序策略
- [ ] 错误处理和资源清理

### Phase 5: 测试
- [ ] 单元测试
- [ ] 集成测试
- [ ] 性能测试
- [ ] 内存限制测试

### Phase 6: 优化
- [ ] 缓冲区大小调优
- [ ] 归并路数优化
- [ ] I/O优化（预读、批量写）

## 📚 参考资料

- **MiniOB文档**: https://oceanbase.github.io/miniob/
- **MemTracer文档**: https://oceanbase.github.io/miniob/game/miniob-memtracer/
- **外部排序算法**: Database System Implementation, Chapter 13
- **项目架构分析**: `/root/miniob/ARCHITECTURE_ANALYSIS.md`

## 💡 最佳实践

1. **先实现简单版本**: 2路归并 → 多路归并
2. **充分测试**: 每个组件独立测试
3. **监控内存**: 使用MemTracer验证
4. **错误处理**: 磁盘满、内存不足等
5. **清理资源**: 使用RAII，析构函数清理临时文件
6. **性能优化**: 先正确，后优化

---

**最后更新**: 2025-10-30
**用途**: 实现big order by（外部排序）的快速参考

