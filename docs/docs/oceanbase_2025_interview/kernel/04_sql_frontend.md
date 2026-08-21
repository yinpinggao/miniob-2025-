# SQL 编译：从字符串到可执行的语句对象

本章导读：客户端发给数据库的 SQL，本质上只是一串字符，比如 `"SELECT id+1 FROM t"`。数据库内核看不懂字符串，它必须先像编译器一样把这串字符"编译"成内部结构化的对象，才能谈得上后续的优化与执行。这一步就是 SQL 前端（SQL frontend），对应 MiniOB 执行链（见 [项目架构总览](../01_project_architecture.md)）中的 ParseStage 与 ResolveStage。

这一章要回答三个层层递进的问题：

1. 编译原理中的词法分析、语法分析在干什么？（用 `SELECT id+1 FROM t` 完整走一遍）
2. MiniOB 的解析器长什么样？`lex_sql.l`、`yacc_sql.y`、`parse_defs.h` 各管什么？想新增一种 SQL 语法要动哪些地方？
3. 解析出来的语法树还是"只知道名字"，怎么变成绑定到真实表、真实字段、带类型检查的语句对象（Stmt）？

预推免面试中，"一条 SQL 是怎么被执行的"几乎是必问题，而前两句回答永远属于本章内容。更重要的是，OceanBase 2025 赛题 24 道题里有 5 道（date、expression、function、alias、join-tables）的第一步都是改语法——本章就是它们的共同起点。

## 词法分析：把字符串切成 token

### 直觉与类比

读英文句子时，你不是逐字母理解，而是先把它切成单词。词法分析（lexical analysis）做的就是这件事：把字符流切成一个个有意义的"单词"，称为 **token**（词法单元）。每个 token 带一个类别和一个值，例如：

- `SELECT` 是关键字 token；
- `id` 是标识符 token（类别 ID，值为字符串 `"id"`）；
- `1` 是数字 token（类别 NUMBER，值为整数 1）；
- `+` 是运算符 token。

生活类比：你去餐厅点菜说"一份宫保鸡丁加米饭"，服务员脑子里会先把这句话切成"一份 / 宫保鸡丁 / 加 / 米饭"几个单元，再去理解它们的组合关系。切词就是词法分析，理解组合关系就是后面要讲的语法分析。

### 严格概念

词法分析器的理论基础是**正则表达式**与**有穷自动机（DFA）**。每种 token 用一个正则表达式描述，例如：

```
DIGIT  [0-9]+          // 一个或多个数字
ID     [A-Za-z_]+[A-Za-z0-9_]*   // 字母/下划线开头，后接字母数字下划线
```

工具 **flex** 读入这些规则，自动生成一个 DFA 扫描器（C 代码）。手写当然也可以（MySQL 的词法分析器就是手写的），但自动生成的 DFA 匹配速度是 O(n)，n 为字符数，且每个字符只看一遍。

当多个规则都能匹配时，flex 遵循两条铁律（这也写在 MiniOB `src/observer/sql/parser/lex_sql.l:70` 的注释里）：

1. **最长匹配优先**：看到 `<=` 时，即使 `<` 也能匹配，也按 `<=` 算；
2. 同样长时，**写在文件前面的规则优先**：`SELECT` 既能匹配关键字规则 `SELECT`，也能匹配标识符规则 `ID`，因为关键字规则写在前面（`lex_sql.l:102`），而 `ID` 规则在 `lex_sql.l:176`，所以 `SELECT` 永远不会被当成普通标识符。

### MiniOB 中的词法分析

MiniOB 的词法文件是 `src/observer/sql/parser/lex_sql.l`，由 `src/observer/sql/parser/gen_parser.sh` 中的 `flex --outfile lex_sql.cpp --header-file=lex_sql.h lex_sql.l` 生成 C++ 扫描器。几个关键点：

- `%option case-insensitive`（`lex_sql.l:59`）：SQL 关键字不区分大小写，`select` 和 `SELECT` 等价；
- `%option bison-bridge reentrant`：扫描器可重入，通过 `yyscan_t` 与 bison 生成的语法分析器传递 token 值和位置；
- 每条规则把值写进 `yylval`（可理解为语法分析端 `%union` 的一个成员），再返回 token 类别。看 `lex_sql.l:80-81` 两行：

```
{DIGIT}+                  yylval->number=atoi(yytext); RETURN_TOKEN(NUMBER);
{DIGIT}+{DOT}{DIGIT}+     yylval->floats=(float)(atof(yytext)); RETURN_TOKEN(FLOAT);
```

逐行解释：`yytext` 是当前匹配到的字符串，`yyleng` 是它的长度。整数走 `atoi` 存进 `yylval->number`（对应 `%union` 的 `int number`），带小数点的走 `atof` 存进 `yylval->floats`，分别返回 `NUMBER`、`FLOAT` 两个 token 类别。注意整数规则写在小数规则前面但互不冲突——`1.5` 按最长匹配只会命中第二行。

另外，`lex_sql.l` 顶部的 `YY_USER_ACTION` 宏（`lex_sql.l:16-23`）在每个 token 匹配后记录行列号到 `yylloc`，这样语法分析阶段就能知道每个语法元素对应原始 SQL 的哪几个字符——后面会看到表达式列名 `id+1` 就是这样截取出来的。

### 小例子：`SELECT id+1 FROM t` 的 token 流

把这条 SQL 送进词法分析器，输出（忽略空白）为：

| 序号 | token 类别 | 值 |
|---|---|---|
| 1 | SELECT | - |
| 2 | ID | "id" |
| 3 | '+' | 字符 '+'（`lex_sql.l:178-181` 直接返回字符本身） |
| 4 | NUMBER | 1 |
| 5 | FROM | - |
| 6 | ID | "t" |

## 语法分析：上下文无关文法与移入-归约

### 直觉与类比

词法分析给出了"单词表"，但单词的排列组合是否合法、组合成什么含义，需要**语法**来裁决。就像"鸡丁吃我"每个词都合法但句子不合法，SQL 里 `FROM t SELECT id` 每个 token 都合法，整体却不合语法。

### 严格概念

语法用**上下文无关文法（CFG）**描述：一组**产生式**，左边是**非终结符**，右边是终结符（token）与非终结符的序列。例如 MiniOB 表达式文法的核心（`yacc_sql.y:1128-1140`，有删减）：

```
expression: expression '+' expression
          | expression '*' expression
          | nonnegative_value
          | rel_attr
          ;
```

读法：一个 expression 可以是"表达式+表达式"、"表达式*表达式"、一个字面量值、或一个字段引用。`| ` 分隔多个候选式，递归定义使任意深的表达式（如 `a+b*c+1`）都属于这个文法。

语法分析器常用的实现方式是**自底向上的移入-归约（shift-reduce）**，bison 生成的是其中的 **LALR(1)** 分析器：维护一个栈，反复做两件事——

- **移入（shift）**：从输入取一个 token 压栈；
- **归约（reduce）**：栈顶若干符号若匹配某产生式右部，就弹出它们、压入左边的非终结符，同时执行该产生式附带的**语义动作**（一段 C++ 代码，构造语法树节点）。

### 小例子：`1+2*3` 是怎么被算对优先级的

考虑 token 流 `NUMBER(1) '+' NUMBER(2) '*' NUMBER(3)`。当栈里是 `expr(1) + expr(2)`、下一个输入是 `*` 时，分析器面临**移入/归约冲突**：是按加法归约（得到 `(1+2)*3`，错），还是继续移入 `*`（得到 `1+(2*3)`，对）？

bison 用**优先级声明**解决。MiniOB `yacc_sql.y:298-303`：

```
%left OR
%left AND
%left EQ LT GT LE GE NE
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS
```

越靠后优先级越高；`%left` 表示左结合。冲突时比较"栈顶产生式的优先级"与"待移入 token 的优先级"：`*` 高于 `+`，所以选择移入。完整分析轨迹：

| 分析栈 | 剩余输入 | 动作 |
|---|---|---|
| 空 | `1 + 2 * 3` | 移入 1 |
| `1` | `+ 2 * 3` | 归约 NUMBER→expr |
| `expr(1)` | `+ 2 * 3` | 移入 +，移入 2，归约出 `expr(2)` |
| `expr(1) + expr(2)` | `* 3` | 冲突：`*` 优先级高 → 移入 |
| `expr(1) + expr(2) *` | `3` | 移入 3，归约出 `expr(3)` |
| `expr(1) + expr(2) * expr(3)` | 空 | 归约乘法 → `expr(6)` |
| `expr(1) + expr(6)` | 空 | 归约加法 → 完成 |

最终得到一棵树：根是 `+`，左子树是常量 1，右子树是 `*(2,3)`。这棵树就是**抽象语法树（AST）**——剥离了括号、优先级等语法细节，只保留计算结构。

## MiniOB 的语法树：lex_sql.l、yacc_sql.y、parse_defs.h 的分工

### 三个文件各管什么

| 文件 | 角色 | 产物 |
|---|---|---|
| `src/observer/sql/parser/lex_sql.l` | 词法规则 | flex 生成 `lex_sql.cpp/.h` |
| `src/observer/sql/parser/yacc_sql.y` | 语法规则 + 语义动作 | bison 生成 `yacc_sql.cpp/.hpp` |
| `src/observer/sql/parser/parse_defs.h` | 语法树节点的数据结构定义 | 直接被两者 include |

注意：这两个 `.l/.y` 文件**没有纳入 CMake 自动构建**，修改后必须手动运行 `src/observer/sql/parser/gen_parser.sh` 重新生成（官方文档 `docs/docs/design/miniob-sql-parser.md` 指出 flex 2.5.35、bison 3.7 测试通过，bison 不要用旧版本）。

调用入口在 `src/observer/sql/parser/parse_stage.cpp:43`：`ParseStage::handle_request` 调用 `parse()`（`src/observer/sql/parser/parse.cpp:34`），它转发给 `yacc_sql.y:1466` 的 `sql_parse()`，后者创建 scanner 并调用 bison 生成的 `yyparse()`。解析失败时 `yyerror`（`yacc_sql.y:23-31`）会造一个 `SCF_ERROR` 节点，`ParseStage` 据此返回 `RC::SQL_SYNTAX`（`parse_stage.cpp:56-68`）。

### bison 文件的三件套：%union、%token、规则动作

`yacc_sql.y` 的核心机制是：

- **`%union`（190-221 行）**：声明语义值的类型集合，如 `ParsedSqlNode *sql_node`、`Expression *expression`、`std::vector<std::unique_ptr<Expression>> *expression_list`、`int number` 等。bison 把它变成类型 `YYSTYPE`。
- **`%token <number> NUMBER`**：声明 token 及其携带值的 union 成员——词法端写的 `yylval->number` 对上的就是这里。
- **`%type <sql_node> select_stmt`**：声明非终结符的语义值类型。
- **规则动作里的 `$$` 与 `$1..$n`**：`$$` 是本产生式归约后的语义值，`$n` 是右部第 n 个符号的值。例如 `rel_attr`（`yacc_sql.y:1239-1252`）：

```
rel_attr:
    ID {
      $$ = new RelAttrSqlNode;
      $$->attribute_name = $1;   // $1 是 ID token 携带的字符串
      free($1);
    }
    | ID DOT ID {
      $$ = new RelAttrSqlNode;
      $$->relation_name  = $1;   // 表名
      $$->attribute_name = $3;   // 字段名
      free($1); free($3);
    }
    ;
```

这就是 `t.id` 和裸 `id` 两种写法的语法。动作代码里 `new` 出来的节点逐级向上传递，最终在根规则 `commands`（`yacc_sql.y:306-311`）里挂进 `ParsedSqlResult`。

另一个值得知道的细节是 `token_name()`（`yacc_sql.y:18-21`）：利用词法端传来的列号 `@$`，从原始 SQL 字符串里把该语法元素的原文截出来作为节点名字。这就是为什么 `SELECT id+1 FROM t` 的输出列头恰好是 `id+1`。

### ParsedSqlNode：所有语句的"总线"

`parse_defs.h` 定义了解析结果的数据结构。设计非常直白：一个枚举 `SqlCommandFlag`（`parse_defs.h:401-429`，`SCF_SELECT`、`SCF_INSERT`……）标记语句种类，类 `ParsedSqlNode`（`parse_defs.h:434-460`）则把每种语句的结构体（`SelectSqlNode selection`、`InsertSqlNode insertion`……）都作为成员平铺在一起，用 `flag` 指明哪一个有效。`SelectSqlNode`（`parse_defs.h:129-139`）的核心成员是：

- `expressions`：SELECT 后面的投影表达式列表；
- `relations`：FROM 的表（`RelationNode` 含 `relation` 和 `alias` 两个字符串，`parse_defs.h:97-103`）；
- `conditions` / `having_conditions`：WHERE / HAVING 的表达式树（AND/OR 已组织成 `ConjunctionExpr`）；
- `group_by`、`order_by`、`limit`、`set_operations`（UNION）。

可以看到一个关键事实：**解析阶段的 SELECT 里连"字段"都只是字符串**（`UnboundFieldExpr` 包着表名/字段名两个 string），它不知道 `t` 是什么、`id` 存不存在。这正是"语法"与"语义"的分界线。

### 完整走一遍：`SELECT id+1 FROM t`

结合前两节，整条归约链如下（每个箭头是一次归约 + 语义动作）：

```
ID("id")            → rel_attr → UnboundFieldExpr(table="", field="id")
NUMBER(1)           → nonnegative_value → Value(1) → ValueExpr
expr(id) '+' expr(1) → ArithmeticExpr(ADD)，name 截取原文 "id+1"
                      （表达式规则在 yacc_sql.y:1128-1176）
expression alias     → expression_list（alias 规则在 1178-1187，此处为空）
ID("t")  + 空 alias  → rel_list = [RelationNode("t")]
where/group_by/opt_having/opt_order_by/opt_limit 全部归约为 nullptr
select_core 归约（yacc_sql.y:1014-1052）：
    new ParsedSqlNode(SCF_SELECT)，装入 expressions 和 relations
select_stmt → command_wrapper → commands：add_sql_node 挂入 ParsedSqlResult
```

最终内存里的对象：

```
ParsedSqlNode (flag = SCF_SELECT)
└── selection : SelectSqlNode
    ├── expressions[0] : ArithmeticExpr(ADD)          name = "id+1"
    │    ├── left  : UnboundFieldExpr("", "id")       ← 只是个名字
    │    └── right : ValueExpr(Value(1, AttrType::INTS))
    ├── relations[0]   : RelationNode(relation="t", alias="")
    └── conditions / group_by / order_by / having / limit : 空
```

顺便一个容易面试加分的冷知识：`yacc_sql.y:1092-1097` 里 `SELECT expression_list`（不带 FROM）被归约为 `SCF_CALC`，所以 MiniOB 里 `SELECT 1+2` 和 `CALC 1+2` 等价。

## 新增一种 SQL 语法：完整流程

官方文档 `docs/docs/design/miniob-how-to-add-new-sql.md` 以 CALC 为例总结了改动清单，把它对应到文件就是七步：

1. **词法**：`lex_sql.l` 增加关键字或符号的 token 规则（注意写在 `ID` 规则之前）；
2. **语法树结构**：`parse_defs.h` 增加 `XxxSqlNode` 结构体和 `SqlCommandFlag` 枚举值，并在 `ParsedSqlNode` 中加成员；
3. **语法规则**：`yacc_sql.y` 在 `%union`/`%token`/`%type` 中登记类型，编写 `xxx_stmt` 产生式与语义动作，并挂进 `command_wrapper` 的候选列表（`yacc_sql.y:313-337`）；
4. **重新生成**：运行 `gen_parser.sh`（flex + bison），再整体编译；
5. **Stmt**：`src/observer/sql/stmt/stmt.h` 的 `DEFINE_ENUM()` 宏列表加一项（如 `DEFINE_ENUM_ITEM(CALC)`），新建 `xxx_stmt.h/.cpp` 实现 `XxxStmt::create()`，并在 `src/observer/sql/stmt/stmt.cpp` 的 `Stmt::create_stmt` 大 switch 里加分支；
6. **算子**：查询类语句需要新增 LogicalOperator/PhysicalOperator（见 `src/observer/sql/optimizer/`），命令类语句则新增 CommandExecutor（`src/observer/sql/executor/command_executor.cpp`）；
7. **结果输出**：在 `src/observer/sql/executor/execute_stage.cpp` 为 `SqlResult` 设置 TupleSchema（列头）。

赛题里的 join-tables、alias、function 等，第一步都是这条流水线的子集：多数时候只改前 4 步，因为 SELECT 的 Stmt 和算子已经存在。

## 语义分析与绑定：从名字到对象

### 直觉与类比

语法分析只保证"句子通顺"，不保证"内容真实"。`SELECT id FROM t` 语法完美，但如果数据库里根本没有表 `t`，执行就是空谈。这就像编译器里"变量未声明"的错误：不是语法错误，而是**语义错误**，需要查**符号表**才能发现。数据库的符号表就是**系统 catalog**（元数据）：有哪些表、每张表有哪些字段、字段什么类型。

MiniOB 里这一步发生在 ResolveStage：`src/observer/sql/parser/resolve_stage.cpp:54` 调用 `Stmt::create_stmt(db, *sql_node, stmt)`，把 `ParsedSqlNode` 翻译成 `Stmt`。这个翻译过程在数据库教材里常叫 **binding（绑定）** 或 name resolution（名字解析）。

### SelectStmt::create：绑定的主战场

以 SELECT 为例，`src/observer/sql/stmt/select_stmt.cpp:34` 的 `SelectStmt::create` 做四件事：

1. **解析 FROM**：逐张表调 `db->find_table(table_name)`（`select_stmt.cpp:60`），不存在则返回 `RC::SCHEMA_TABLE_NOT_EXIST`。同时处理别名——看 66-74 行：

```cpp
auto &table_alias = select_sql.relations[i].alias;
if (!table_alias.empty()) {
  const auto &success = temp_map.emplace(table_alias, table);
  if (!success.second)
    return RC::INVALID_ALIAS;      // 同层别名重复，直接报错
} else {
  temp_map.emplace(table_name, table);
}
```

逐行解释：有别名就以别名为 key 登记进 `temp_map`；`unordered_map::emplace` 的 second 为 false 表示 key 已存在，即同层两张表用了同一个别名（比如 `FROM t a, t2 a`），返回 `INVALID_ALIAS`。没别名就用真名登记。注意这实现了"取了别名就必须用别名引用"的常见 SQL 规则。

2. **建立作用域**：`table_map` 以 `parent_table_map`（外层查询的表）初始化再插入本层表（`select_stmt.cpp:49,82`），这正是**相关子查询**能引用外层表的机制；`BinderContext`（`expression_binder.h:21-58`）把表列表、别名列表、表映射、默认表打包，交给 `ExpressionBinder`。

3. **绑定各处表达式**：投影、GROUP BY、ORDER BY 依次调 `expression_binder.bind_expression`（`select_stmt.cpp:96-120`），WHERE 和 HAVING 交给 `FilterStmt::create`（`select_stmt.cpp:135,143`）。

4. **组装 SelectStmt**：把绑定后的表达式、表、FilterStmt 等装进 `SelectStmt`。

### ExpressionBinder：把 UnboundFieldExpr 变成 FieldExpr

`ExpressionBinder::bind_expression`（`expression_binder.cpp:62-127`）是一个按 `ExprType` 分派的大 switch，对每种表达式递归处理。最核心的是 `bind_unbound_field_expression`（`expression_binder.cpp:177-237`），逻辑：

- 字段名没带表名：用 `context_.default_table()`（只有单表查询才有默认表，见 `select_stmt.cpp:84-87`）；
- 带了表名/别名：`context_.find_table(table_name)` 查 `table_map`，查不到返回 `RC::SCHEMA_TABLE_NOT_EXIST`；
- 在表的 `TableMeta` 里查字段：`table->table_meta().field(field_name)`，查不到返回 `RC::SCHEMA_FIELD_MISSING`；
- 都查到，创建 `FieldExpr`（内含 `Field{table, field_meta}`——真实表对象与字段元数据的指针），处理别名与多表时的列名展示（`表名.字段名`），替换掉原来的 `UnboundFieldExpr`。

另外几个分派也值得记住：

- `bind_star_expression`（`expression_binder.cpp:129`）：把 `*` 展开成所有表的所有用户字段，`wildcard_fields`（`expression_binder.cpp:35-60`）从 `sys_field_num()` 开始遍历以跳过系统隐藏字段（比如 MVCC 的 `__trx_xid_begin/end`）；
- `bind_function_expression`（`expression_binder.cpp:480-605`）：按函数名分别识别聚合函数（`AggregateFunctionExpr::type_from_string`）、`distance`、普通函数（`NormalFunctionExpr::type_from_string`），都不认识返回 `RC::UNKNOWN_FUNCTION`；`count(*)` 会被特判改写成 `count(1)`；
- `check_aggregate_expression`（`expression_binder.cpp:434-478`）：一个真实的**类型检查**例子——`SUM/AVG` 的参数必须是 `INTS/FLOATS`，且聚合函数不允许嵌套（`sum(sum(x))` 报错）；
- `bind_subquery_expression`（`expression_binder.cpp:607-625`）：子查询递归地走"生成 SelectStmt → 逻辑计划 → 物理计划"，因为子查询在执行期要被当成一个能吐出值的黑盒。

### 绑定前后的对比

```
绑定前 (ParseStage 输出)                绑定后 (ResolveStage 输出)
ArithmeticExpr(ADD)                     ArithmeticExpr(ADD)
 ├── UnboundFieldExpr("", "id")   ==>    ├── FieldExpr(Field{Table t, FieldMeta id:INT})
 └── ValueExpr(1)                        └── ValueExpr(1)
```

至此，AST 上每个名字都落实到了真实的数据库对象，`ArithmeticExpr::value_type()` 这类静态类型推断也才有了依据（字段类型只有绑定后才知道）。

## 表达式体系与 Stmt 族谱

### 表达式树：一切皆表达式

MiniOB 把"任何能产生值的 SQL 元素"统一抽象为表达式，基类 `Expression` 在 `src/observer/sql/expr/expression.h:74`。`ExprType` 枚举（`expression.h:41-59`）列出全部种类，常用的有：

| ExprType | 类 | 含义 |
|---|---|---|
| STAR | `StarExpr` | `*` 或 `t.*` |
| UNBOUND_FIELD | `UnboundFieldExpr` | 绑定前的字段名（两个字符串） |
| FIELD | `FieldExpr` | 绑定后的字段（Table + FieldMeta） |
| VALUE | `ValueExpr` | 常量 |
| CAST | `CastExpr` | 类型转换 |
| COMPARISON | `ComparisonExpr` | `= < > LIKE IN EXISTS` 等比较 |
| CONJUNCTION | `ConjunctionExpr` | AND / OR |
| ARITHMETIC | `ArithmeticExpr` | `+ - * /` 与负号 |
| UNBOUND_FUNCTION → AGGREGATION / NORMAL_FUNCTION | `UnboundFunctionExpr` → `AggregateFunctionExpr` / `NormalFunctionExpr` | 函数调用，绑定后分流入聚合或普通函数 |
| SUBQUERY / EXPRLIST / MATCH_AGAINST | `SubQueryExpr` / `ListExpr` / `MatchAgainstExpr` | 子查询、`(1,2,3)` 列表、全文检索 |

求值接口是经典的树递归：`get_value(const Tuple &tuple, Value &value)`——内部节点先递归求子表达式的值，再做自己的运算。`ArithmeticExpr::calc_value`（`src/observer/sql/expr/expression.cpp:494`）里能看到 NULL 传播语义：任一操作数为 NULL，结果直接置 NULL（499-501 行）。

### 常量折叠思想

另一个虚函数 `try_get_value(Value &value) const`（`expression.h:93`）回答"不依赖任何行，这个表达式现在能不能算出值"。`ValueExpr` 直接返回常量；`ArithmeticExpr::try_get_value`（`expression.cpp:674-696`）递归尝试左右子树，都成功就调用 `calc_value` 当场算出结果。这正是编译器**常量折叠（constant folding）**的思想：`1+2` 不必等执行期逐行计算，编译期就能折成 `3`。MiniOB 把它用于距离函数的度量参数提取、子查询物化判断等场景。

静态类型推断同样在编译期完成：`ArithmeticExpr::value_type()`（`expression.cpp:476-492`）的规则是——两个 INT 做非除法运算结果是 INT；除法一律 FLOAT；有 FLOAT 参与则 FLOAT；向量加减乘除仍是 VECTORS。

### Stmt 族谱

绑定完成的语句统一是 `Stmt` 的子类（`src/observer/sql/stmt/stmt.h:85-94`）。`StmtType` 枚举用一个 X-macro 技巧生成（`stmt.h:32-64`）：`DEFINE_ENUM()` 宏里每行一个 `DEFINE_ENUM_ITEM(SELECT)`，展开两次分别生成枚举值和 `stmt_type_name()` 的字符串化 switch，加新语句类型只需加一行。`Stmt::create_stmt`（`stmt.cpp:57-153`）则是 `SqlCommandFlag → XxxStmt::create` 的分派表。现有族谱包括 `SelectStmt`、`InsertStmt`、`UpdateStmt`、`DeleteStmt`、`CalcStmt`、`CreateTableStmt`、`TrxBeginStmt` 等二十余种，一一对应 `parse_defs.h` 里的 `XxxSqlNode`。

## 类型系统：AttrType、Value 与类型转换

### AttrType 与 Value

MiniOB 的类型枚举在 `src/observer/common/type/attr_type.h:17-29`：`CHARS`（定长字符串）、`INTS`（4 字节整数）、`FLOATS`（4 字节浮点）、`DATES`（4 字节整数编码的日期）、`TEXTS`（超长文本）、`VECTORS`（向量）、`BOOLEANS`（内部使用）、`NULLS`、`UNDEFINED`。

`Value`（`src/observer/common/value.h:36-171`）是"类型 + 数据"的运行时载体，核心是一个 union（`value.h:157-164`）：

```cpp
union Val {
  int32_t int_value_;
  float   float_value_;
  bool    bool_value_;
  char   *pointer_value_;   // CHARS/TEXTS
  float  *vector_value_;    // VECTORS
} value_;
```

配三个标志：`attr_type_` 记录类型，`own_data_` 标记字符串内存是否归自己管（决定析构时是否 free），`is_null_` 独立于类型记录 NULL——所以一个 NULL 值仍然"记得"自己本来是 INT 还是 DATE。

### DataType：每种类型的行为策略

类型的行为（怎么比较、怎么加减乘除、怎么转换）不在 `Value` 里写死，而是由策略类 `DataType` 的子类实现（`src/observer/common/type/data_type.h`）：`IntegerType`、`FloatType`、`CharType`、`DateType`、`VectorType` 等，全部注册在静态数组 `type_instances_` 里（`data_type.h:104`），用 `DataType::type_instance(attr_type)` 按类型取单例。`Value::add/subtract/divide/cast_to` 都只是转发到对应 `DataType` 的虚函数（`value.h:70-98`）。这就是"新增一种类型该往哪挂"的答案。

隐式转换用**代价模型**决策：`Value::implicit_cast_cost(from, to)`（`value.h:135-141`）转发给 `DataType::cast_cost`，不能转换返回 `INT32_MAX`。真正的插入点在 `src/observer/sql/expr/expression_iterator.cpp:113-170`：比较表达式两侧类型不同时，分别算"左转右"和"右转左"的代价，选代价小的方向插一个 `CastExpr`；如果被转的一侧是常量，还会顺手用 `try_get_value` 把转换折成新常量（又一次常量折叠）。

### 日期与向量是怎么挂进来的

- **DATE**：建表语法 `type` 规则把 `DATE_T` 映射为 `AttrType::DATES`（`yacc_sql.y:738`），字段长度定为 `sizeof(int)`（`yacc_sql.y:696-697`）。SQL 里的 `'2024-05-20'` 一开始只是 CHARS 字面量；插入时 `Value::cast_to` 把它交给 `CharType::cast_to`（`src/observer/common/type/char_type.cpp`），其中调用 `parse_date` 把字符串编成 `YYYYMMDD` 形式的 int 并做闰年/合法性检查；`DateType::compare/to_string`（`src/observer/common/type/date_type.cpp`）再让日期可以直接按整数比较、按 `YYYY-MM-DD` 显示。用整数编码而不是 `time_t`，天然免疫 2038 年溢出和时区问题。
- **VECTOR**：`VECTOR_T` 映射为 `AttrType::VECTORS`，`attr_def` 把 `vector(N)` 的长度定为 `sizeof(float) * N`（`yacc_sql.y:676-677`）。字面量有两种写法：直接的 `[1.0, 2.0]`（`yacc_sql.y:871` 的 `LSBRACE digits_list RSBRACE`）和 `string_to_vector('[1,2]')` 函数（`yacc_sql.y:874-890` 在语法动作里直接完成 cast）。距离度量则通过 `distance(v1, v2, 'cosine')` 函数暴露，绑定逻辑在 `bind_function_expression` 的 distance 分支。

## 复杂度分析

| 阶段 | 输入规模 | 时间复杂度 | 说明 |
|---|---|---|---|
| 词法分析 | 字符数 n | O(n) | DFA 每个字符扫一遍 |
| 语法分析 | token 数 m | O(m) | LALR(1) 每个 token 移入一次，归约总次数也有界 |
| 语义绑定 | AST 节点数 k | O(k) | 每节点常数次 hash 查找（表名/字段名），平均 O(1) |
| 空间 | - | O(k) | AST + Stmt 对象 |

几点说明：解析阶段的复杂度只与 **SQL 语句本身的长度** 有关，与表里的数据量完全无关——这就是为什么"解析慢"几乎从不是数据库的瓶颈（除非超高频的短 SQL，这时工业界会用 prepared statement 缓存解析结果）。绑定阶段查表/查字段走 `unordered_map` 和 `TableMeta` 的字段索引，平均 O(1)。真正的性能分水岭在后面的优化与执行阶段，那是下一章的话题。

## 工业界对比：PostgreSQL 的 parser/analyzer 分工

MiniOB 的"parse → resolve"两段式不是教学发明的简化，而是工业界的标准结构。

**PostgreSQL**（`src/backend/parser/`）分得最干净，也最常被面试官当作参照系：

1. **raw parser**：`scan.l`（flex）+ `gram.y`（bison）产出**原始语法树**（`RawStmt` 包着 `SelectStmt` 等节点）。注意一个容易混淆的同名点：PostgreSQL 的 `SelectStmt` 是*未绑定*的解析节点，而 MiniOB 的 `SelectStmt` 是*已绑定*的语句对象——名字相同，阶段不同，类比时别张冠李戴。PG 的原始树里表名是 `RangeVar`、字段名是 `ColumnRef`，同样只是字符串。
2. **analyzer**：`parse_analyze()`（`analyze.c`）把原始树变换成 `Query`：查 catalog（`pg_class`/`pg_attribute`）解析表与字段，生成 `RangeTblEntry` 和 `Var`；函数名查 `pg_proc` 按参数类型做重载决议；未指定类型的字符串字面量在这个阶段才推断类型。作用域同样按查询层级逐层向外查找——与 MiniOB 的 `parent_table_map` 异曲同工。
3. 之后才轮到 rewriter（视图/规则展开）和 planner/optimizer。

**MySQL**：词法分析器是手写的（`sql/sql_lex.cc`），语法分析用 bison（`sql/sql_yacc.yy`）；表达式统一是 `Item` 类族（`Item_field`、`Item_func`……），名字解析在 `fix_fields()` 中完成——"parser 产出未绑定对象，再由 resolve 阶段 fix"的思路与 MiniOB 的 `UnboundFieldExpr → FieldExpr` 完全一致。

**OceanBase**：作为生产级分布式数据库，其 SQL 层同样是 parser → resolver → transformer → optimizer 的流水线（`src/sql/` 下对应目录），MiniOB 的 ParseStage/ResolveStage/OptimizeStage 命名正是对它的教学化复刻。差别主要在工业界要处理方言兼容、更细的报错定位（行列号）、海量内置函数的重载与类型推导规则表，以及 plan cache——解析结果被缓存复用以省掉每条 SQL 的编译开销。

一句话总结分工哲学：**parser 只对"形式"负责（合不合语法），analyzer/resolver 才对"含义"负责（表存在吗、类型匹配吗、名字歧义吗）**。前者不需要 catalog，可以在客户端或无元数据环境独立完成；后者必须访问 catalog。

## 小结

- SQL 前端 = 编译器前端：词法分析（flex，正则→DFA，最长匹配+规则顺序）把字符串切成 token；语法分析（bison，LALR(1) 移入-归约）按上下文无关文法把 token 归约成 AST，优先级用 `%left/%nonassoc/%prec` 声明。
- MiniOB 解析产物是 `ParsedSqlNode`：`SqlCommandFlag` 标记语句种类，各 `XxxSqlNode` 平铺存放，`SelectSqlNode` 里字段还只是字符串（`UnboundFieldExpr`）。
- 新增 SQL 语法的流水线：`lex_sql.l` 加 token → `parse_defs.h` 加结构 → `yacc_sql.y` 加规则 → `gen_parser.sh` 重新生成 → `stmt.h/.cpp` 加 Stmt → 算子/执行器/输出。
- 语义分析（ResolveStage/`SelectStmt::create`/`ExpressionBinder`）负责名字解析、作用域（`parent_table_map` 支持相关子查询）、类型检查（如聚合参数校验）与隐式转换（`CastExpr` 按 cast cost 插入）。
- 表达式体系以 `Expression` 为基类的树结构，`get_value` 逐行求值、`try_get_value` 实现常量折叠、`value_type` 静态推断结果类型；`Stmt` 族谱用 X-macro 维护。
- 类型系统三件套：`AttrType` 枚举、`Value`（union + is_null）、`DataType` 策略单例；DATE 用 `YYYYMMDD` 整数编码，VECTOR 用 float 数组，都是沿"枚举 + DataType 子类 + 词法/语法挂钩"这条路上挂进来的。
- 解析/绑定复杂度只取决于 SQL 长度（O(n)），与数据量无关；PostgreSQL 的 parser/analyzer、MySQL 的 Item/fix_fields 都是同样的两段式。

## 面试追问

**问：flex 里 `SELECT` 既能匹配关键字规则又能匹配标识符规则 `ID`，怎么保证它被识别成关键字？如果把 `ID` 规则写到关键字前面会怎样？**

答：flex 两条规则：最长匹配优先；等长时写在前面的优先。`SELECT` 与 `ID` 对 "SELECT" 匹配长度相同，而关键字规则全部写在 `ID` 规则（`lex_sql.l:176`）之前，所以关键字胜出。若把 `ID` 提前，所有关键字都会被吞成标识符，语法分析立刻全面报错。工业界（如 PostgreSQL）更进一步：把关键字分成保留/非保留等级，非保留关键字（如 `NAME`）在特定语法位置仍可当标识符用，这是用文法而非词法顺序解决的。

**问：bison 文法里表达运算符优先级和结合性有哪几种手段？`-1+2` 和 `1+2*3` 分别靠什么解析对？**

答：靠 `%left/%right/%nonassoc` 声明优先级（声明越靠后优先级越高）解决二元运算符之间的移入/归约冲突——`1+2*3` 靠 `*` 优先级高于 `+` 选择先移入。一元负号是另一个问题：它和二元减号共用 `'-'` 这个 token，但优先级应高于乘除，于是声明一个虚拟 token `UMINUS` 并在产生式上用 `%prec UMINUS` 强行指定该产生式的优先级（`yacc_sql.y:303,1149-1151`），`-1+2` 才能解析成 `(-1)+2` 而不是 `-(1+2)`。

**问：为什么 MiniOB 不在语法分析阶段直接把字段名解析成 FieldExpr，而要绕一圈 UnboundFieldExpr？**

答：职责分离。parser 只依赖文法，不需要、也不应该访问数据库 catalog——这样 parser 可以独立测试、独立复用；而且绑定需要上下文：同一字段名在不同 FROM 子句、不同子查询层级含义不同（相关子查询要查外层的 `parent_table_map`），这些信息在归约当下并不齐全。PostgreSQL 的 `ColumnRef → Var`、MySQL 的 `Item_field::fix_fields` 都是同一个设计。

**问：`SELECT id FROM t` 如果表 t 不存在，错误在哪一层报出？和 `SELECTT id FROM t` 的报错有何本质区别？**

答：表不存在是**语义错误**，在 ResolveStage 报出：`SelectStmt::create` 调 `db->find_table` 失败返回 `RC::SCHEMA_TABLE_NOT_EXIST`（`select_stmt.cpp:60-64`）。而 `SELECTT` 会让整条语句无法匹配任何产生式，bison 触发 `yyerror`，生成 `SCF_ERROR` 节点，ParseStage 返回 `RC::SQL_SYNTAX`（`parse_stage.cpp:56-68`）——这是**语法错误**。区别本质：前者 token 序列合法、只是引用了不存在的对象；后者连 token 序列本身都不合文法，根本走不到语义阶段。

**问：`try_get_value` 这个接口存在的意义是什么？**

答：它区分了"值依赖于行"的表达式和"编译期就能确定"的表达式，是常量折叠的入口。`ValueExpr` 直接成功，`ArithmeticExpr` 递归尝试左右子树，全常量则当场算出结果（`expression.cpp:674-696`）。用途至少有三处：优化器可以把 `WHERE id = 1+2` 折成 `id = 3` 以利用索引；比较表达式插 `CastExpr` 时把常量一侧直接折成目标类型的新常量（`expression_iterator.cpp:137-144`）；`distance()` 的度量参数必须编译期取到字符串字面量（`expression_binder.cpp:550-552`）。

**问：多表查询时 `SELECT id FROM t1, t2` 且两张表都有 id 字段，MiniOB 怎么处理？**

答：字段没带表名时走 `BinderContext::default_table()`，而默认表只在单表查询时设置（`select_stmt.cpp:84-87`），多表时为 nullptr——严格说这里应该报"字段歧义（ambiguous）"错误，但当前实现对这个边界处理不完善（可能空指针解引用而非返回清晰错误码，这也是赛题 alias 题解析中提到的已知边界）。PostgreSQL/MySQL 在同样场景下会明确报 `column reference "id" is ambiguous`。面试中主动指出这个差距，反而是加分项。

## 回到赛题

本章是以下五道题的共同第一步——它们都要求 MiniOB 认识新的 SQL 形式，而"认识"永远从词法/语法开始：

- **第 4 题 date**（[赛题解析 4. date](../02_problems_01_08.md)）：`lex_sql.l:120` 的 `DATE` 关键字 token、`yacc_sql.y:738` 的 `type` 规则把 `DATE_T` 映射到 `AttrType::DATES`、`attr_def` 把日期列长度定为 `sizeof(int)`——日期类型正是沿本章第 8 节"类型挂载路线"接入的；非法日期在 CHARS→DATES 的 `cast_to` 阶段才被拒绝。
- **第 5 题 join-tables**（[赛题解析 5. join-tables](../02_problems_01_08.md)）：`yacc_sql.y:1053-1083` 的 `INNER JOIN` 分支与 `join_clauses`（1287-1303）把多张表和 ON 条件折进 `SelectSqlNode`（ON 条件间用 `ConjunctionExpr::AND` 串联）。当前文法的缺口——JOIN 分支不接 HAVING/ORDER BY/LIMIT、不支持与逗号连接混写、`relation` 不带别名——全部要先动本章的语法层才能修。
- **第 6 题 expression**（[赛题解析 6. expression](../02_problems_01_08.md)）：就是本章表达式体系的直接扩展——`expression` 规则（`yacc_sql.y:1128-1176`）、`ArithmeticExpr` 的 `value_type` 静态类型推断、`calc_value` 的 NULL 传播与除零行为，都是该题的核心考点。
- **第 7 题 function**（[赛题解析 7. function](../02_problems_01_08.md)）：`func_expr` 规则（`yacc_sql.y:1189-1230`）把 `LENGTH(...)` 等解析成 `UnboundFunctionExpr`，`bind_function_expression`（`expression_binder.cpp:480`）按名字分流成 `NormalFunctionExpr`，再分派到 `src/observer/sql/builtin/builtin.cpp` 的实现——新增函数就是"加枚举 + 加 binder 分支 + 写 builtin"三步。
- **第 12 题 alias**（[赛题解析 12. alias](../03_problems_09_16.md)）：`alias` 规则（`yacc_sql.y:1178-1187`）、`RelationNode` 的 alias 字段、`SelectStmt::create` 用 `temp_map.emplace` 检测同层别名重复并返回 `INVALID_ALIAS`、绑定后的 `FieldExpr` 携带 table alias 穿透到扫描算子——别名问题从头到尾是名字解析问题，正是本章第 6 节的内容。

更广泛的官方背景可交叉阅读设计文档 [miniob-sql-parser.md](../../design/miniob-sql-parser.md)、[miniob-sql-expression.md](../../design/miniob-sql-expression.md) 与 [miniob-how-to-add-new-sql.md](../../design/miniob-how-to-add-new-sql.md)。
