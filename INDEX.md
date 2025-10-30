# MiniOB Big Order By 项目资源索引

> 本文档列出了为实现big order by（外部排序）准备的所有资源

## 📑 文档列表

### 1. 主要文档

| 文档名称 | 文件路径 | 说明 |
|---------|---------|------|
| **Big Order By README** | `BIG_ORDER_BY_README.md` | 📘 项目入口文档，包含实现路线图和快速开始指南 |
| **架构分析文档** | `ARCHITECTURE_ANALYSIS.md` | 📚 完整的MiniOB架构分析，包含所有关键模块详解 |
| **快速参考手册** | `QUICK_REFERENCE.md` | 📝 常用接口、命令、代码片段速查 |
| **本索引** | `INDEX.md` | 📑 资源导航文档 |

### 2. 文档结构

```
📘 BIG_ORDER_BY_README.md (从这里开始！)
    ├─→ 快速开始
    ├─→ 实现路线图
    └─→ 参考其他文档

📚 ARCHITECTURE_ANALYSIS.md (深度理解)
    ├─→ 项目架构详解
    ├─→ 存储引擎分析
    ├─→ 当前ORDER BY实现
    ├─→ 外部排序需求
    └─→ MemTracer详解

📝 QUICK_REFERENCE.md (快速查阅)
    ├─→ 核心接口速查
    ├─→ 编译运行命令
    ├─→ 示例代码片段
    └─→ 常见问题解答
```

## 🛠️ 工具脚本

### 1. Shell脚本

| 脚本名称 | 路径 | 功能 | 使用示例 |
|---------|------|------|---------|
| **MemTracer启动脚本** | `scripts/run_with_memtracer.sh` | 便捷地使用MemTracer运行Observer | `./scripts/run_with_memtracer.sh -m 104857600 -i 1000` |
| **外部排序测试脚本** | `scripts/test_external_sort.sh` | 自动化测试外部排序 | `./scripts/test_external_sort.sh -n 100000 -m 32` |

### 2. 示例代码

| 文件名称 | 路径 | 说明 |
|---------|------|------|
| **MemTracer使用示例** | `examples/memtracer_example.cpp` | 演示如何使用MemTracer API监控内存 |

## 📂 源码位置

### 1. 需要修改的文件

| 组件 | 路径 | 说明 |
|------|------|------|
| OrderByPhysicalOperator | `src/observer/sql/operator/order_by_physical_operator.{h,cpp}` | ⭐ 主要修改点：添加外部排序逻辑 |

### 2. 需要创建的文件（建议）

```
src/observer/sql/operator/external_sort/
├── tuple_serialize.h              # Tuple序列化/反序列化
├── tuple_serialize.cpp
├── run_generator.h                # Run生成器
├── run_generator.cpp
├── run_merger.h                   # Run归并器
├── run_merger.cpp
└── temp_file_manager.h            # 临时文件管理
    temp_file_manager.cpp
```

### 3. 关键依赖文件（只读参考）

| 组件 | 路径 | 用途 |
|------|------|------|
| Physical Operator基类 | `src/observer/sql/operator/physical_operator.h` | 理解算子接口 |
| Tuple抽象 | `src/observer/sql/expr/tuple.h` | 理解数据行结构 |
| DiskBufferPool | `src/observer/storage/buffer/disk_buffer_pool.{h,cpp}` | 磁盘I/O操作 |
| RecordManager | `src/observer/storage/record/record_manager.{h,cpp}` | 记录管理 |
| MemTracer API | `deps/memtracer/mt_info.h` | 内存监控API |

## 🎯 推荐阅读顺序

### 对于初学者

1. **第一步**: 阅读 `BIG_ORDER_BY_README.md`
   - 了解项目目标
   - 快速开始部分
   - 大致了解实现步骤

2. **第二步**: 浏览 `QUICK_REFERENCE.md`
   - 熟悉常用接口
   - 记住编译运行命令
   - 了解MemTracer基本用法

3. **第三步**: 深入 `ARCHITECTURE_ANALYSIS.md`
   - 理解MiniOB整体架构
   - 分析当前ORDER BY实现
   - 学习外部排序算法

4. **第四步**: 实践
   - 编译项目并启用MemTracer
   - 运行示例代码
   - 开始实现

### 对于有经验的开发者

1. 快速浏览 `BIG_ORDER_BY_README.md` 的实现路线图
2. 查阅 `QUICK_REFERENCE.md` 所需的接口
3. 参考 `ARCHITECTURE_ANALYSIS.md` 的关键技术点
4. 直接开始实现

## 📚 快速链接

### 编译相关

```bash
# 标准编译
bash build.sh release

# 启用MemTracer
bash build.sh release -DWITH_MEMTRACER=ON

# Debug模式
bash build.sh debug -DWITH_MEMTRACER=ON
```

详见: `QUICK_REFERENCE.md` → "编译和运行"

### MemTracer使用

```bash
# 使用脚本（推荐）
./scripts/run_with_memtracer.sh -m 33554432 -i 1000

# 手动运行
MT_MEMORY_LIMIT=33554432 MT_PRINT_INTERVAL_MS=1000 \
  LD_PRELOAD=./build/lib/libmemtracer.so ./build/bin/observer
```

详见: `ARCHITECTURE_ANALYSIS.md` → "MemTracer使用指南"

### 核心API速查

```cpp
// Physical Operator接口
virtual RC open(Trx *trx) = 0;
virtual RC next() = 0;
virtual RC close() = 0;

// MemTracer API
#include "memtracer/mt_info.h"
size_t memtracer::allocated_memory();
size_t memtracer::memory_limit();

// Tuple接口
virtual int cell_num() const = 0;
virtual RC cell_at(int index, Value &cell) const = 0;
```

详见: `QUICK_REFERENCE.md` → "核心接口速查"

## 🔬 测试资源

### 单元测试模板

位置: `BIG_ORDER_BY_README.md` → Phase 5: 测试与调试

### 集成测试脚本

```bash
./scripts/test_external_sort.sh -n 100000 -m 32
```

### 内存限制测试

```bash
./scripts/run_with_memtracer.sh -m 33554432
```

## 📊 进度追踪

### 实现检查清单

详见: `BIG_ORDER_BY_README.md` → "实现检查清单"

### 时间规划

| 阶段 | 预计时间 |
|------|----------|
| Phase 1: 基础设施 | 1-2天 |
| Phase 2: Run生成 | 2-3天 |
| Phase 3: Run归并 | 2-3天 |
| Phase 4: 集成 | 1-2天 |
| Phase 5: 测试 | 2-3天 |
| Phase 6: 优化 | 1-2天 |
| **总计** | **9-15天** |

详见: `BIG_ORDER_BY_README.md` → "预期时间线"

## 💡 关键概念速查

### 外部排序

**核心思想**: 当数据量超过内存时，使用磁盘辅助排序

**两个阶段**:
1. **Run Generation**: 读取→内存排序→写入临时文件
2. **Merge**: 多路归并临时文件

详见: `ARCHITECTURE_ANALYSIS.md` → "外部排序算法"

### MemTracer

**作用**: 监控MiniOB进程内存使用，支持设置内存上限

**环境变量**:
- `MT_MEMORY_LIMIT`: 内存上限（字节）
- `MT_PRINT_INTERVAL_MS`: 打印间隔（毫秒）

详见: `ARCHITECTURE_ANALYSIS.md` → "MemTracer使用指南"

### 当前ORDER BY问题

- ❌ 全部数据必须加载到内存
- ❌ 无法处理大数据集
- ❌ 启动延迟高

详见: `ARCHITECTURE_ANALYSIS.md` → "当前ORDER BY实现分析"

## 🎓 学习路径

### 1. 理论学习

- [ ] 理解MiniOB整体架构
- [ ] 掌握外部排序算法原理
- [ ] 了解磁盘I/O和缓冲区管理
- [ ] 学习Tuple和物理算子概念

### 2. 实践操作

- [ ] 编译MiniOB并运行
- [ ] 使用MemTracer监控内存
- [ ] 阅读现有ORDER BY代码
- [ ] 运行测试脚本

### 3. 代码实现

- [ ] 实现Tuple序列化
- [ ] 实现Run生成器
- [ ] 实现Run归并器
- [ ] 集成到OrderByOperator
- [ ] 编写测试用例

### 4. 优化和调试

- [ ] 性能分析
- [ ] 内存优化
- [ ] I/O优化
- [ ] 错误处理完善

## 📞 获取帮助

### 文档内部链接

遇到问题时，可以查阅：

- **架构问题**: `ARCHITECTURE_ANALYSIS.md`
- **接口用法**: `QUICK_REFERENCE.md`
- **实现步骤**: `BIG_ORDER_BY_README.md`
- **MemTracer**: 所有文档的MemTracer章节

### 外部资源

- [MiniOB官方文档](https://oceanbase.github.io/miniob/)
- [MiniOB GitHub仓库](https://github.com/oceanbase/miniob)
- [MemTracer文档](https://oceanbase.github.io/miniob/game/miniob-memtracer/)

## 📝 文档版本信息

| 项目 | 信息 |
|------|------|
| **创建时间** | 2025-10-30 |
| **文档版本** | 1.0 |
| **MiniOB版本** | 基于最新主分支 |
| **目标** | 实现big order by（外部排序） |
| **状态** | 📋 准备阶段 - 文档就绪 |

## 🎯 下一步行动

1. ✅ **已完成**: 
   - 理解MiniOB架构
   - 学习MemTracer使用
   - 准备开发文档和工具

2. 🚀 **立即开始**:
   - 阅读 `BIG_ORDER_BY_README.md`
   - 编译项目并启用MemTracer
   - 运行示例代码验证环境
   - 开始Phase 1实现

3. 📅 **计划**:
   - 按照路线图逐步实现
   - 每个Phase完成后进行测试
   - 记录遇到的问题和解决方案

---

**祝你实现顺利！** 🚀

如有任何问题，请参考本索引中列出的文档资源。

