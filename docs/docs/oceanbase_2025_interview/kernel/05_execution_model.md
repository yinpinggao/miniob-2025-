# 查询执行：火山模型、向量化与物化

## 本章导读

前面的章节解决的是"SQL 怎么被看懂"：词法语法分析把它变成语法树，Binder 把名字绑到表和列，优化器把它变成一棵**物理算子树**。但算子树本身只是一份"施工图纸"，真正把数据一行一行搬出来、过滤、计算、排序、发回客户端的，是**执行器（Executor）**。

执行器是数据库内核里最容易被低估、却最容易被面试追问的部分。几乎所有关系型数据库——MySQL、PostgreSQL、OceanBase、MiniOB——的执行器都建立在同一个模型上：**火山模型（Volcano Model），也叫迭代器模型（Iterator Model）**。它的全部奥义只有一句话：每个算子只负责在被问到时回答"下一行是什么"。围绕这句话又派生出三个必须搞清楚的问题：

- 为什么这种"一次一行"的设计能统治数据库五十年？它的代价是什么？
- 排序、聚合这类"不看全数据就没法开工"的算子怎么塞进这个模型？（答案：阻塞算子与物化）
- 现代数据库（包括 MiniOB）为什么又引入"一次一批"的向量化执行？

本章从生活直觉出发，手动推演一次 `Project(Predicate(Scan))` 的完整调用栈，然后逐层对照 MiniOB 源码：`src/observer/sql/operator/physical_operator.h`（算子接口契约）、`src/observer/sql/expr/tuple.h`（行数据的四种视图）、`src/observer/storage/common/chunk.h`（批量数据）、`src/observer/sql/executor/sql_result.cpp`（结果如何回到客户端）。读完你应该能徒手画出一条 SELECT 从 `open` 到 `close` 的完整时序。

## 1. 直觉：查询是一条流水线，不是一锅端

先建立一个生活类比。你在食堂打饭，窗口后排着三个师傅：洗菜师傅、炒菜师傅、打菜师傅。你每喊一次"来一份"，打菜师傅就向炒菜师傅要一盘菜，炒菜师傅发现没菜了就向洗菜师傅要一份洗好的菜。**每个师傅都不需要知道全局有多少菜，只需要在"被要"的时候交出"下一份"**。

数据库执行查询就是这条传送带。一条 SQL：

```sql
SELECT name, age + 1 FROM student WHERE age > 18;
```

会被翻译成一棵算子树（括号表示父子嵌套）：

```
Project(name, age+1)        ← 打菜师傅：负责算出你要的两列
  └── Predicate(age > 18)   ← 炒菜师傅：只放行合格的行
        └── TableScan(student)  ← 洗菜师傅：一行一行从表里掏数据
```

执行时没有任何一个算子会"把所有数据读进来算完再交给下一个"。恰恰相反，是最上层的 Project 一次一次地**向下要**（pull），数据一行一行地**向上流**。这个"上层向下拉数据"的模型，就是火山模型。

为什么叫"火山"？它得名于 Goetz Graefe 在 1990 年前后主持的 Volcano 查询优化器/执行器研究项目，迭代器模型在这个项目里被系统化并推广开来。一个常用的助记是：数据像岩浆一样从叶子（存储层）流向树根（输出）。注意这只是助记，命名来源是项目名本身——面试时别把因果关系说反。

## 2. 火山模型精讲：open / next / close 接口契约

### 2.1 严格定义

火山模型规定：执行计划是一棵**物理算子（Physical Operator）**树，每个算子实现同一组接口：

| 接口 | 契约 |
| --- | --- |
| `open()` | 初始化：打开扫描器、分配状态，并递归 open 自己的孩子。自顶向下只调用一次 |
| `next()` | 生产下一行。成功返回该行；数据耗尽返回一个特殊状态（不是错误）；此后不应再有新数据 |
| `close()` | 释放资源：关闭扫描器、释放临时内存/文件，并递归 close 孩子。必须与 open 成对 |

三条契约组合出一个重要性质：**每个算子只关心"给我下一行"，不关心数据从哪来、要到哪去**。表扫描不知道自己在给谁供货，排序也不知道客户在等 LIMIT——大家都只对接口负责。

### 2.2 MiniOB 中的接口

MiniOB 的契约写在 `src/observer/sql/operator/physical_operator.h` 的抽象基类 `PhysicalOperator` 里，文件注释直接写明了生命周期语义：

```cpp
virtual RC open(Trx *trx) = 0;
virtual RC next() { return RC::UNIMPLEMENTED; }
virtual RC next(Chunk &chunk) { return RC::UNIMPLEMENTED; }  // 批量向量化版本
virtual RC close() = 0;

virtual Tuple *current_tuple() { return nullptr; }
```

逐条解释：

- `open(Trx *trx)`：传入当前事务指针。扫描类算子在这一步才真正打开存储层的扫描器。
- `next()`：每调一次，算子就让自己的"当前行"前进一步。返回 `RC::SUCCESS` 表示产出了一行；**返回 `RC::RECORD_EOF` 表示数据结束——这是一个状态码而不是执行错误**，`src/observer/sql/executor/sql_result.cpp` 的注释专门强调了这一点。
- `next(Chunk &chunk)`：向量化版本的 next，一次填一批（第 6 节展开）。基类默认返回 `RC::UNIMPLEMENTED`，即"本算子不支持批模式"。
- `current_tuple()`：next 成功后，通过它取出"当前这一行"的视图（`Tuple`，第 7 节展开）。
- 算子之间用 `children_`（`vector<unique_ptr<PhysicalOperator>>`）连接，`unique_ptr` 表达独占所有权：父算子析构时递归释放整棵树。

算子类型枚举 `PhysicalOperatorType` 也在同一文件里，常见的有 `TABLE_SCAN`、`PREDICATE`、`PROJECT`、`NESTED_LOOP_JOIN`、`ORDER_BY`、`HASH_GROUP_BY`、`LIMIT`，以及一批带 `_VEC` 后缀的向量化版本（`TABLE_SCAN_VEC`、`PREDICATE_VEC`、`PROJECT_VEC`、`GROUP_BY_VEC`、`AGGREGATE_VEC`、`EXPR_VEC`）。

## 3. 手动推演：Project(Predicate(TableScan)) 的一次完整调用

概念不落地等于没学。我们用一张 4 行的小表，把整条调用链像单步调试一样走一遍。

```
student 表：
 id | name | age
----+------+-----
  1 | Ada  | 20
  2 | Bob  | 17
  3 | Cat  | 22
  4 | Dan  | 16
```

SQL：`SELECT name, age + 1 FROM student WHERE age > 18;`，物理计划是 `Project(name, age+1) → Predicate(age>18) → TableScan(student)`。期望结果是两行：`Ada | 21` 和 `Cat | 23`。

### 3.1 open 阶段：自顶向下初始化

```
SqlResult::open
 └─ Project.open      → 直接递归（自身无状态）
     └─ Predicate.open → 直接递归
         └─ TableScan.open → table_->get_record_scanner(...) 打开存储扫描器
```

对照 `src/observer/sql/operator/table_scan_physical_operator.cpp`：`TableScanPhysicalOperator::open` 调用 `table_->get_record_scanner(record_scanner_, trx, mode_)` 拿到一个 `RecordFileScanner`，并用 `tuple_.set_schema(...)` 给自己的 `RowTuple` 成员绑定表结构。Project 和 Predicate 的 `open` 都只是转发给孩子——它们是"无状态"的流水线算子。

### 3.2 next 阶段：一次调用栈只产出一行

客户端每要一行，`SqlResult::next_tuple` 就调用一次根算子的 `next()`。第 1 次调用的完整调用栈：

```
SqlResult::next_tuple
 └─ Project.next                     (project_physical_operator.cpp)
     └─ Predicate.next               (predicate_physical_operator.cpp)
         └─ loop: TableScan.next
              ├─ 扫出 (1,Ada,20)，回 Predicate
              │    └─ 表达式 age>18 求值 = true  → Predicate.next 返回 SUCCESS
     └─ Project 拿到当前行，current_tuple() 对表达式求值：name='Ada', age+1=21
 输出第 1 行：Ada | 21
```

第 2 次 `next()`：TableScan 扫出 `(2,Bob,17)`，Predicate 求值为 false，**Predicate 不返回，而是继续循环再调 TableScan.next**；扫出 `(3,Cat,22)`，通过，Project 输出 `Cat | 23`。注意这里的关键细节：一次外层 `next()` 内部可能驱动下层吐多行——被拒绝的行在 Predicate 内部就消化掉了，上层毫无感知。

第 3 次 `next()`：TableScan 扫出 `(4,Dan,16)` 被拒绝，扫描器随后返回 EOF，`RC::RECORD_EOF` 沿着 TableScan → Predicate → Project 逐级原样上传，`SqlResult` 知道：结果发完了。

### 3.3 源码对照：三个算子的 next

TableScan 的 `next()` 是整条链的发动机（`src/observer/sql/operator/table_scan_physical_operator.cpp`）：

```cpp
while (OB_SUCC(rc = record_scanner_.next(current_record_))) {
  tuple_.reset();
  tuple_.append_base_rids(table_, current_record_.rid());
  tuple_.set_record(&current_record_);
  rc = filter(tuple_, filter_result);   // 下推谓词在这里先过滤一轮
  if (rc != RC::SUCCESS) { return rc; }
  if (filter_result) { break; }         // 找到一行合格的就收工
}
return rc;                               // 扫描器耗尽时 rc 就是 RECORD_EOF
```

逐行看：每次循环从扫描器取一条物理记录，把它包进成员 `tuple_`（一个 `RowTuple`），然后应用下推到扫描层的谓词；只有合格的行才让 `next()` 返回。扫描器一旦耗尽，循环退出条件把 `RECORD_EOF` 自然地带出去——**结束标志不需要任何特殊代码路径，就是循环的正常退出**。

Predicate 的 `next()` 形状几乎一样（`src/observer/sql/operator/predicate_physical_operator.cpp`）：`while (child->next() == SUCCESS)` 里对每行求值布尔表达式，`value.get_boolean()` 为真就返回，否则继续要下一行。

Project 则展示了"纯转发 + 延迟计算"（`src/observer/sql/operator/project_physical_operator.cpp`）：

```cpp
RC ProjectPhysicalOperator::next() {
  if (children_.empty()) { return RC::RECORD_EOF; }
  return children_[0]->next();          // 自己什么都不做，直接转发
}

Tuple *ProjectPhysicalOperator::current_tuple() {
  auto tuple = children_[0]->current_tuple();
  tuple_.set_tuple(tuple);              // 把孩子的行绑进自己的表达式元组
  tuple_.set_base_rids(tuple->base_rids());
  return &tuple_;
}
```

Project 的 `next()` 甚至不碰数据——它只是把"前进一步"的命令传下去。真正的投影计算（`age + 1`）发生在别人调它的 `current_tuple()` 再调 `cell_at()` 的时候，那时才对表达式逐个求值（`tuple_` 的类型是 `ExpressionTuple`，见第 7 节）。**计算被推迟到值真正被消费的那一刻**，这是火山模型里一个很典型的技巧。

### 3.4 close 阶段：与 open 镜像收尾

`close` 自顶向下递归：Project.close → Predicate.close → TableScan.close，后者调用 `record_scanner_.close_scan()` 关闭扫描器。之后 `SqlResult::close` 还会完成事务收尾（第 8 节）。契约要求 close 必须与 open 成对：中途出错也要 close，否则扫描器、临时文件、事务都会泄漏。

## 4. 火山模型的优点与缺点

### 4.1 优点：为什么五十年不过时

- **组合性（可插拔）**。所有算子实现同一接口，优化器可以像搭积木一样任意组合计划：给 Scan 上面套 IndexScan、给 Join 换成 HashJoin，上层完全无感。这也是逻辑计划/物理计划分离能成立的前提。
- **流式、内存友好**。任意时刻系统只需要记住"当前这一行"加少量算子状态，处理 1 亿行和 10 行占用的内存一样多——工作集是 O(1)（不算阻塞算子，见第 5 节）。
- **天然支持提前终止**。`LIMIT 10` 只需要根算子被调 10 次 next，之后就 close，下层数据再多也不会被读出来。
- **统一的错误与事务边界**。open/next/close 的生命周期给了事务一个天然的挂载点：open 时启动，close 时收尾。

### 4.2 缺点：一行一次的代价

做个数字演算。一张 1000 万行的表，算子树高 4 层（Scan → Predicate → Project → 根），每个算子每层每行一次 `next()` 加一次 `current_tuple()`：

- 虚函数调用次数 ≈ 1000 万 × 4 × 2 = **8000 万次间接跳转**；
- 每次调用都伴随一次 `Value` 的构造/拷贝/析构（行模型里数据按"一格一格"的 `Value` 对象流转）；
- 每行访问的代码路径不同（不同算子的虚函数），CPU 指令缓存和分支预测器频繁失效；
- 循环体太小（每圈只处理一行），编译器和 CPU 都没法做循环展开、流水并行，更谈不上 SIMD。

结论：**火山模型的算法复杂度没问题，慢在"每行固定的调度税"**。数据量小无所谓，跑分析型大查询时这个税会吃掉可观的比例——这正是向量化的动机（第 6 节）。

## 5. 阻塞算子 vs 流水线算子：排序和聚合为什么必须物化

### 5.1 一个思想实验

假设你是排序算子，正在流水线上班。上游递给你第 1 行：`age = 30`。你能立刻把它交给下游吗？不能——下一行可能是 `age = 3`，它才该排最前面。事实上，**只要输入没看完，任何一行都可能是全局最小值**，你一行都不敢发出去。聚合同理：`SELECT SUM(age) FROM student` 的最后一行到达之前，SUM 都不是最终结果。

据此算子分两类：

| 类别 | 定义 | 例子 | 内存特征 |
| --- | --- | --- | --- |
| 流水线算子（pipelined） | 拿到一行就能产出一行 | TableScan、Predicate、Project、Limit、NestedLoopJoin（左行驱动时） | O(1) |
| 阻塞算子（blocking / pipeline breaker） | 必须消费完（几乎全部）输入才能产出第一行 | OrderBy、GroupBy/Aggregate（无索引序时）、去重 | O(N)，与输入成正比 |

### 5.2 物化（Materialization）的含义与代价

阻塞算子没法"流过"数据，只能先把输入**物化**：把上游来的每一行复制一份，存进自己拥有的内存结构（或磁盘文件），存够了再处理。物化的代价有三层：

1. **复制开销**。上游的 `Tuple` 通常只是"指向当前记录的视图"（下一条 next 就会覆盖它），不能直接收藏，必须逐字段拷贝成自己拥有的数据。
2. **内存开销**。N 行全部驻留内存，峰值 O(N × 行宽）。
3. **溢出风险**。内存装不下时必须落盘（外部排序、哈希分区），引入额外 I/O。

### 5.3 MiniOB 的证据：ORDER BY 在 open 里就把孩子榨干

MiniOB 的 `OrderByPhysicalOperator`（`src/observer/sql/operator/order_by_physical_operator.cpp`）是教科书式的阻塞算子。它的 `open()` 打开孩子后直接调用 `fetch_and_sort_tables()`，后者的核心循环：

```cpp
while (RC::SUCCESS == (rc = children_[0]->next())) {
  // 1. 对每个排序键表达式求值，存入 order_by_line
  // 2. copy_current_tuple_as_value_list(...) 把整行复制成 ValueListTuple
  OrderEntry entry;
  entry.keys     = std::move(order_by_line);
  entry.tuple    = std::unique_ptr<Tuple>(copied_tuple);
  entry.sequence = sequence_counter_++;
  sorted_entries_.emplace_back(std::move(entry));
}
std::stable_sort(sorted_entries_.begin(), sorted_entries_.end(), /* 多键比较 */);
```

也就是说：**`open()` 返回时，孩子的全部输出已经被复制进 `sorted_entries_` 并排序完毕**。之后的每次 `next()` 只是 `sorted_entries_[sorted_pos_++]` 按序发牌。复制用的是 `ValueListTuple::make`——把视图型 Tuple 的每个 cell 取值拷出，变成一个真正拥有数据的 `ValueListTuple`（第 7 节）。

聚合也一样：`HashGroupByPhysicalOperator::open`（`src/observer/sql/operator/hash_group_by_physical_operator.cpp`）里同样是一个 `while (OB_SUCC(rc = child.next()))` 循环，先把孩子消费完、逐组累积聚合值，之后 `next()` 才逐组吐出结果。

## 6. 向量化执行：一次喂一批

### 6.1 直觉

逐行模型像一粒一粒数米：每一粒都要走一遍"拿起—检查—放下"的完整动作。向量化执行换成一铲一铲：一次取一批（比如几千行），对这一批做同样的动作，**固定动作的成本被摊薄到每一粒米上**。

### 6.2 严格概念与数据结构

向量化执行把 `next()` 的语义从"给我下一行"改成"给我下一批"。一批数据按**列式**组织：同一列的值在内存中连续存放。MiniOB 的两个核心数据结构：

- `Column`（`src/observer/storage/common/column.h`）：一列的定长值连续存放在一块内存 `data_` 里，记录 `count_`（当前行数）和 `capacity_`（容量，默认 `DEFAULT_CAPACITY = 8192`）。目前只支持定长类型。
- `Chunk`（`src/observer/storage/common/chunk.h`）：一组 `Column` 的集合（`vector<unique_ptr<Column>>`），外加 `column_ids_` 记录这些列对应孩子算子的哪些输出。提供 `rows()`（当前批行数）、`get_value(col_idx, row_idx)`、`reset_data()`（清空数据复用内存）等。

算子接口对应地多了一个重载：`virtual RC next(Chunk &chunk)`——一次调用要求孩子填满（或部分填满）一个 Chunk。

### 6.3 为什么快

还是 1000 万行、4 层算子，批大小 8192：

- **虚函数调用被分摊**。调用次数从 8000 万降到 1000 万 / 8192 × 4 × 2 ≈ 1 万次，调度税几乎归零。
- **循环密集，利于 SIMD**。处理逻辑变成"对 8192 个连续的 int 做同一个比较/加法"，编译器可以向量化成 SIMD 指令，一条指令同时算多个值。
- **缓存命中率高**。同一列的值连续存放，顺序扫描一列时每次缓存行加载进来的都是马上要用到的数据；而行模型下一行的一个字段后面跟着一堆这次用不到的字段，缓存行里大半是浪费。

注意一个常见误解：**向量化没有改变算法复杂度**，1000 万行还是要比较 1000 万次；它省的是每次比较之外的"行刑队开销"（per-tuple overhead）。

### 6.4 MiniOB 的双模式：选择与回退

MiniOB 同时保留了逐行和按批两条执行路径，由会话级开关控制：

- 枚举定义在 `src/observer/common/types.h`：`ExecutionMode { UNKNOWN_MODE, TUPLE_ITERATOR, CHUNK_ITERATOR }`；会话默认是 `TUPLE_ITERATOR`（`src/observer/session/session.h`）。
- 用户可以用 SQL 切换：`SET execution_mode = 'chunk_iterator';`（语法规则是 `SET 变量名 = 值`，见 `src/observer/sql/parser/yacc_sql.y` 的 `set_variable_stmt`；值不区分大小写），解析在 `src/observer/sql/executor/set_variable_executor.cpp`。
- 真正的选择在优化阶段的 `OptimizeStage::generate_physical_plan`（`src/observer/sql/optimizer/optimize_stage.cpp`）：

```cpp
if (session->get_execution_mode() == ExecutionMode::CHUNK_ITERATOR &&
    LogicalOperator::can_generate_vectorized_operator(logical_operator->type())) {
  session->set_used_chunk_mode(true);
  rc = physical_plan_generator_.create_vec(*logical_operator, physical_operator);
} else {
  session->set_used_chunk_mode(false);
  rc = physical_plan_generator_.create(*logical_operator, physical_operator);
}
```

- `can_generate_vectorized_operator`（`src/observer/sql/operator/logical_operator.cpp`）目前只对 `DELETE` / `INSERT` / `UNION` 三类返回 false——**这是"回退"的确切含义：按逻辑算子类型决定是否走行模式，而不是计划生成失败后自动降级**。
- `create_vec`（`src/observer/sql/optimizer/physical_plan_generator.cpp`）目前只实现了四类节点的向量化生成：`TableGet` → `TableScanVecPhysicalOperator`、`Project` → `ProjectVecPhysicalOperator`（外加一个 `ExprVecPhysicalOperator` 包在孩子外面）、`GroupBy` → `GroupByVecPhysicalOperator` 或 `AggregateVecPhysicalOperator`、`Explain`。其他形状的计划在 vec 入口会直接返回 `INVALID_ARGUMENT`。所以准确地说：**MiniOB 的向量化覆盖了"单表扫描 + 投影 + 聚合"这条分析型常用路径，会话默认仍是逐行模式**。
- 会话上的 `used_chunk_mode_` 标记记录这条 SQL 实际走了哪条路，输出层据此分流（第 8 节）。

## 7. Tuple 体系：同一行数据的四种视图

执行器里流转的"行"不是裸字节，而是 `Tuple` 抽象（`src/observer/sql/expr/tuple.h`）。核心接口就四个：`cell_num()`（几个字段）、`cell_at(i, value)`（取第 i 个值）、`spec_at(i, spec)`（第 i 列的描述：表名/列名/别名）、`find_cell(spec, value)`（按名字反查）。为什么需要好几种实现？因为**"一行"在不同算子眼里来源完全不同**：有的是表里读出的物理记录，有的是表达式算出来的，有的是左右两行拼起来的。用多态视图屏蔽差异，算子之间才能互相对接。

| Tuple 实现 | 扮演角色 | 数据在哪 |
| --- | --- | --- |
| `RowTuple` | 表扫描的一行：物理记录的执行层视图 | 不拥有数据，`set_record()` 绑到一条 `Record`，`cell_at` 按 `FieldMeta` 的 offset/len 从记录字节里现切 |
| `ProjectTuple` / `ExpressionTuple` | 投影/表达式的一行：如 `age + 1` | 不拥有数据，持有一组表达式和指向孩子 Tuple 的指针，`cell_at` 即对孩子行求表达式值 |
| `JoinedTuple` | 连接的一行：左右两行拼成一行 | 不拥有数据，持有 left/right 两个 Tuple 指针，`cell_at` 按下标区间转发给左或右 |
| `ValueListTuple` | 物化的一行：值全部拷出来自己持有 | 拥有数据，`cells_` 是一份完整的 `Value` 数组 |

几个值得记住的细节：

- `RowTuple::cell_at` 是真正的"解释执行"：按字段元数据的偏移量从记录字节串里切出这一段，必要时处理 NULL 标记位（可空字段尾部存一个 `'1'`/`'0'` 字节）。所以行模型里取一个字段要做一次"定位 + 解释"。
- `ProjectTuple` 存在于 `tuple.h`；而当前 `ProjectPhysicalOperator` 实际持有的成员是 `ExpressionTuple<std::unique_ptr<Expression>>`（见 `src/observer/sql/operator/project_physical_operator.h` 和 `src/observer/sql/expr/expression_tuple.h`）。两者思路相同：cell_at 时对底层行求表达式。这正是第 3 节说的"延迟计算"。
- `JoinedTuple` 不只服务 Join 算子：相关子查询里内层算子通过 `set_parent_tuple` 拿到外层行后，TableScan/Predicate 的过滤也会临时拼一个 `JoinedTuple`（左 = 内层行，右 = 外层行），让谓词能同时引用内外两层的列。
- `ValueListTuple` 是物化的落点。`ValueListTuple::make(tuple, out)` 把任意视图型 Tuple 逐格取值拷成自有副本——第 5 节 ORDER BY 物化用的就是它；排序键缓存 `order_keys_` 和 `SplicedTuple` 也是为排序场景准备的同类结构。
- 每个 Tuple 还带一份 `base_rids_`（来源基表 + 行号 RID）。`TableScanPhysicalOperator::next` 里那句 `tuple_.append_base_rids(table_, current_record_.rid())` 就在维护它，供需要回表或按 RID 取数据的表达式（如全文检索的 MATCH...AGAINST）使用。

四种 Tuple 可以嵌套。`tuple.h` 的注释里就有官方示例：`SELECT t1.a + t2.b FROM t1, t2` 的行结构是

```
Project(t1.a + t2.b)
      |
   Joined
   /    \
Row(t1) Row(t2)
```

视图层层委托，只有 Row 真正碰存储——**取一个值，实际是一条委托链走到底层记录**。

## 8. 结果如何回到客户端：SqlResult、Communicator 与事务收尾

执行器不会主动"推"结果，整条链是被动等拉的。MiniOB 里拉动算子树的是 `SqlResult`（`src/observer/sql/executor/sql_result.h` / `.cpp`），再往外是网络层的 `Communicator`。完整时序：

```
客户端 ──SQL──> SqlTaskHandler → Parse → Resolve → Optimize(生成算子树)
                                                     │
ExecuteStage::handle_request_with_physical_operator  │ (execute_stage.cpp)
  sql_result->set_operator(std::move(physical_operator))  ← 只移交所有权，不执行
                                                     │
Communicator::write_result_internal  (plain_communicator.cpp)
  ├─ sql_result->open()     → trx->start_if_need(); operator_->open(trx)
  ├─ 按 tuple_schema 打印表头
  ├─ while next_tuple/next_chunk == SUCCESS: 逐行/逐批写出 "a | b | c"
  │    （RECORD_EOF 时把状态码翻译回 SUCCESS）
  └─ sql_result->close()    → operator_->close(); 销毁算子树; 事务 commit/rollback
```

三个关键点：

1. **ExecuteStage 不执行查询**（`src/observer/sql/executor/execute_stage.cpp`）。它只把算子树的所有权转给 `SqlResult`——"此处仅转移算子树所有权，并不立刻把查询全部执行完"。真正的执行发生在输出阶段，客户端要一行才生产一行。
2. **事务在 open 启动、在 close 收尾**。`SqlResult::open` 先 `trx->start_if_need()` 再打开算子树；`SqlResult::close` 先 `operator_->close()`、`operator_.reset()` 销毁整棵树，然后：如果会话不在显式事务模式（没有 BEGIN），这条 SQL 就是一个事务边界——成功 `commit()`，失败 `rollback()`。所以 **close 不是可选项**：中途不 close，留下的是没提交的事务和没释放的扫描器。
3. **输出按模式分流**。以 `PlainCommunicator` 为例（`src/observer/net/plain_communicator.cpp`，MySQL 协议的 `mysql_communicator.cpp` 同样有此分支）：

```cpp
if (event->session()->get_execution_mode() == ExecutionMode::CHUNK_ITERATOR
    && event->session()->used_chunk_mode()) {
  rc = write_chunk_result(sql_result);   // next_chunk：一批一批取
} else {
  rc = write_tuple_result(sql_result);   // next_tuple：一行一行取
}
```

`write_tuple_result` 的循环就是火山模型的最终消费端：`while (RC::SUCCESS == (rc = sql_result->next_tuple(tuple)))`，对每行逐格 `cell_at` 转字符串、以 ` | ` 分隔写出；循环因 `RECORD_EOF` 退出后把它翻译回 `RC::SUCCESS`。`write_chunk_result` 形状相同，只是内层多了一层"遍历这批的行"的循环，每批取完 `chunk.reset()` 复用内存。

## 9. 复杂度与开销对比

把本章的开销模型汇总成一张表（N = 行数，H = 算子树高，B = 批大小，K = LIMIT/top-K 大小）：

| 维度 | 逐行火山模型 | 向量化（Chunk）模型 |
| --- | --- | --- |
| 每行/批接口调用 | N × H 次虚函数 | (N/B) × H 次虚函数 |
| 数据布局 | 行式，字段随记录走 | 列式，同列值连续 |
| SIMD/循环优化 | 几乎不可能 | 天然友好 |
| 内存占用（流水线算子） | O(1) | O(B × 列宽），批内存 |
| 适合场景 | 点查、短事务（TP） | 大扫描、聚合分析（AP） |

物化的代价单独记：

| 场景 | 时间 | 额外空间 |
| --- | --- | --- |
| 内存排序（MiniOB `sorted_entries_`） | O(N log N) | O(N × 行宽），全量驻留 |
| 外部排序（`ExternalSorter`，run + 归并） | 比较仍约 O(N log N)，另有 O(N) 级磁盘读写 | 内存受 buffer 限制（MiniOB 固定 5 MB），代价是临时文件与归并管理 |
| 哈希聚合 | O(N) | O（分组数 × 组宽） |

## 10. 工业数据库怎么做

- **MySQL / InnoDB**：长期采用逐行迭代（存储引擎层 `handler::rnd_next` 一行一行喂给 server 层）。MySQL 8.0 把 SELECT 执行器重构成标准的迭代器框架（`Iterator` 体系），本质上仍是火山模型；MySQL 本体没有全面的向量化执行，分析型负载由列存引擎 HeatWave 承担。
- **PostgreSQL**：经典火山模型，`ExecInitNode` / `ExecProcNode` / `ExecEndNode` 三件套与 open/next/close 一一对应。它的优化方向不是换模型，而是给模型"减负"：PostgreSQL 11 起引入表达式 LLVM JIT，把逐行求值中解释执行的部分编译成机器码。
- **OceanBase**：在保留迭代器框架的同时引入**向量化执行**——算子间一次传递一批按列组织的数据（Vector），3.x 起逐步铺开，4.x 演进到更彻底的向量化 2.0；同时对排序、聚合、连接等阻塞算子做物化与落盘（dump）管理，内存超预算时写临时文件。面试里常被用来对比：OceanBase 用"向量化 + 物化落盘"同时压住逐行开销和内存峰值。
- **ClickHouse / DuckDB** 这类原生分析型数据库则一步到位：列存 + 全向量化执行，批是默认单位。

可以看到工业界的共识：**火山模型的接口契约没人推翻，大家改的是"批的大小"和"阻塞算子的内存纪律"**。

## 小结

- 火山模型 = 所有物理算子实现 `open` / `next` / `close`，父算子每调一次 `next`，子树只生产下一行；`RC::RECORD_EOF` 是结束状态而非错误。MiniOB 的契约在 `src/observer/sql/operator/physical_operator.h`。
- 推演 `Project(Predicate(TableScan))` 要会画调用栈：open 自顶向下开扫描器；next 逐行上拉，被 Predicate 拒绝的行在下层内部消化；Project 延迟到 `current_tuple`/`cell_at` 才求表达式。
- 优点：组合性、流式 O(1) 内存、LIMIT 提前终止；缺点：每行每层一次虚函数调用、行式布局缓存不友好——这是"每行固定调度税"，不是复杂度问题。
- 排序/聚合是阻塞算子：不看全输入不敢产出第一行，只能物化（MiniOB 的 `OrderByPhysicalOperator::open` 用 `ValueListTuple` 复制全部输入再 `stable_sort`）。物化撑爆内存就外排落盘。
- 向量化把 next 的粒度从一行改成一批（`Chunk`/`Column`，默认容量 8192），靠虚函数分摊、SIMD、缓存命中提速。MiniOB 双模式由 `execution_mode` 会话变量控制，回退按逻辑算子类型决定，vec 路径目前覆盖 TableGet/Project/GroupBy/Explain。
- Tuple 是行的多态视图：`RowTuple`（物理记录）、`ExpressionTuple`/`ProjectTuple`（表达式）、`JoinedTuple`（拼接）、`ValueListTuple`（物化副本）。
- 结果回流：`ExecuteStage` 只移交算子树所有权，`SqlResult` 驱动 open/next/close，`Communicator` 逐行（或逐 Chunk）写回客户端，事务在 `SqlResult::close` 里提交或回滚。

## 面试追问

**问：为什么叫"火山模型"？是因为数据像火山喷发吗？**

答：名字来自 Goetz Graefe 在 1990 年前后主持的 Volcano 查询优化器/执行器研究项目，迭代器执行模型在这个项目里被系统化。"数据像岩浆从叶子流向树根"只是后人的助记。它的另一个常用名字是迭代器模型（Iterator Model），SQL Server 里就叫 iterators。

**问：ORDER BY 为什么必须物化？有没有不需要物化全部输入的排序？**

答：全局有序要求看到全部输入——最后一行没读之前，任何已读行都可能是新的最小值，所以排序算子在输入耗尽前不能产出任何一行，只能先物化。两种例外：其一，若有索引天然按排序键有序（B+ 树序扫描），行到达的顺序就是最终顺序，可以完全流水线化；其二，只需要前 K 名时（ORDER BY ... LIMIT K）可以用大小为 K 的堆做 top-K，内存从 O(N) 降到 O(K)——注意 MiniOB 当前的 `OrderByPhysicalOperator` 没有做 top-K 优化，依然全量物化。

**问：向量化为什么快？复杂度不是没变吗？**

答：复杂度确实没变，快在常数项。一是接口开销分摊：批大小 B 时虚函数调用次数降为 1/B；二是批内是"对连续同列数据做同一操作"的密集循环，编译器能生成 SIMD，CPU 分支预测也稳定；三是列式连续布局让每次缓存行加载的都是有效数据。MiniOB 里一批的默认容量是 8192（`Column::DEFAULT_CAPACITY`），即调度税大约摊薄三个数量级。

**问：MiniOB 在向量化模式下，遇到不支持的计划会自动回退到逐行吗？**

答：以源码为准，回退发生在"逻辑算子类型"层面而不是运行期自动降级：`OptimizeStage::generate_physical_plan` 先看会话 `execution_mode` 是否为 `CHUNK_ITERATOR`，再看 `LogicalOperator::can_generate_vectorized_operator`——目前只有 DELETE/INSERT/UNION 三类被排除、强制走逐行；通过检查后调 `PhysicalPlanGenerator::create_vec`，但它只实现了 TableGet/Project/GroupBy/Explain 四类节点，其他形状会直接返回 `INVALID_ARGUMENT` 报错，而不是悄悄回退。所以 MiniOB 的向量化是"覆盖单表扫描 + 投影 + 聚合的常用路径"，默认会话仍是 TUPLE_ITERATOR。

**问：`next()` 返回 `RC::RECORD_EOF` 之后，调用方还要做什么？**

答：停止循环并调用 `close()`。EOF 只表示"没有更多行了"，不代表资源已释放。MiniOB 里 `SqlResult::close` 会递归关闭算子树、销毁算子树，并在非显式事务模式下为这条 SQL 提交（成功时）或回滚（失败时）事务——漏掉 close 就是泄漏扫描器、临时文件和未决事务。

**问：`LIMIT 10` 在火山模型里为什么省时间？如果下面有 ORDER BY 呢？**

答：火山模型是拉式的，根算子被调 10 次 next 后调用方就 close，上游不会再被驱动——扫描多少行取决于下游要多少行，这就是提前终止。但阻塞算子会破坏这个性质：ORDER BY 在 open 阶段已经把全部输入物化并排序完毕，LIMIT 只能省"发牌"的环节，省不掉排序本身。要进一步省，就得靠 top-K 堆或有序索引扫描让计划里根本不出现全量物化。

## 回到赛题

- **题 1 basic**（[赛题 1–8 解析](../02_problems_01_08.md)）：这是本章推演模板的完整实例。`SELECT id FROM t WHERE id = 1` 生成 `Project(Predicate(TableGet))` 逻辑树，物理阶段可能把 TableGet 换成 IndexScan；执行时正是 `SqlResult` 逐行 `next_tuple` 拉取、`close` 时自动提交事务。basic 题为什么"牵一发动全身"？因为所有后续赛题都建立在这条 open/next/close 契约不被破坏的前提上。
- **题 15 order-by**（[赛题 9–16 解析](../03_problems_09_16.md)）：`OrderByPhysicalOperator` 就是本章的阻塞算子范例——`open` 里 `fetch_and_sort_tables` 把每行复制成 `ValueListTuple`（物化 + 排序键 + 输入序号），再 `stable_sort`；`next` 只做发牌。多列排序的字典序比较、`LIMIT` 必须位于排序之后，都是"阻塞算子不看全输入不能产出"这一性质的直接推论。
- **题 24 big-order-by**（[赛题 17–24 解析](../04_problems_17_24.md)）：本章"物化与内存的关系"的压轴案例。四表连接放大了 N，全量物化的 O(N × 行宽） 内存直接撞墙，`OrderByPhysicalOperator::open` 因此要先估算内存、超过 50 MB 阈值就切换 `ExternalSorter`：5 MB buffer 分批排序落盘成 run，再多路归并。它完整演示了阻塞算子在内存约束下的标准出路——物化从内存搬到磁盘，用 I/O 换峰值内存。
