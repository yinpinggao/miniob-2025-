# MiniOB-2025 训练营 — 题目列表 全部题目 (24题) 完整题面


来源页面: https://open.oceanbase.com/train/detail/17 （题目列表 tab）


## 1.basic
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800445&subQuestionName=basic
- 页面标题: basic
- 统计信息: 通过率：62.3%难度：简单题目总分：10

MiniOB本身具有的一些基本功能。比如创建表、创建索引、查询数据、查看表结构等。也就是说本题可以理解为送分题。
在开发其它功能时，需要留意不要破坏这些基础功能。


## 2.update
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800448&subQuestionName=update
- 页面标题: update
- 统计信息: 通过率：65.2%难度：简单题目总分：10

1. 实现更新行数据的功能。
2. 当前实现 update 单个字段即可。现在 MiniOB 具有 insert 和 delete 功能，在此基础上实现更新功能。可以参考 insert_record 和 delete_record 的实现。目前仅能支持单字段update的语法解析，但是不能执行。需要考虑带条件查询的更新，和不带条件的更新，同时需要考虑带索引时的更新。
示例 SQL 语句：
```
UPDATE t SET id = 1 WHERE age = 1;
```


## 3.drop-table
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800447&subQuestionName=drop-table
- 页面标题: drop-table
- 统计信息: 通过率：77.8%难度：简单题目总分：10

1.实现删除表 (drop table)，清除表相关的资源。
2.当前 MiniOB 支持建表与创建索引，但是没有删除表的功能。
3.在实现此功能时，除了要删除所有与表关联的数据，不仅包括磁盘中的文件，还包括内存中的索引等数据。
示例 SQL 语句：
```
DROP TABLE table_name;
```


## 4.date
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800446&subQuestionName=date
- 页面标题: date
- 统计信息: 通过率：44.2%难度：简单题目总分：10

在现有功能上实现日期类型字段。
- 当前已经支持了 int、char、float类型，在此基础上实现date类型的字段。date 测试可能超过 2038 年 2 月，也可能小于 1970 年 1 月 1 号。注意处理非法的 date 输入（考虑 date 类型的值的合法性，如考虑闰年的情况），需要返回 FAILURE。
- 这道题目需要考虑语法解析，类型相关操作，还需要考虑 DATE 类型数据的存储。
示例 SQL 语句：
```
CREATE TABLE t(id INT, birthday DATE);
INSERT INTO t VALUES(1, '2022-10-10');
```

- 有关基本类型转换可以参考：[22年赛题-基本类型转换](https://github.com/oceanbase/miniob/wiki/OceanBase--数据库大赛-2022-初赛赛题#4-基本类型转换)
-
- 参考资料：
- Date 类型解析视频：[https://open.oceanbase.com/course/detail/13252](https://open.oceanbase.com/course/detail/13252)


## 5.join-tables
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800454&subQuestionName=join-tables
- 页面标题: join-tables
- 统计信息: 通过率：32.3%难度：简单题目总分：10

1. 实现 INNER JOIN 功能，需要支持 join 多张表。
2. 当前已经支持多表查询的功能，这里主要工作是语法扩展，并考虑数据量比较大时如何处理。
3. 注意带有多条 on 条件的 join 操作。
4. 注意隐式内连接和 INNER JOIN 混合的情况。
示例 SQL 语句：
```
SELECT * FROM t INNER JOIN t1 ON t.id = t1.id, t2 WHERE t2.id = t.id;
```

有关基本类型转换可以参考：[22年赛题-基本类型转换](https://github.com/oceanbase/miniob/wiki/OceanBase--%E6%95%B0%E6%8D%AE%E5%BA%93%E5%A4%A7%E8%B5%9B-2022-%E5%88%9D%E8%B5%9B%E8%B5%9B%E9%A2%98#4-%E5%9F%BA%E6%9C%AC%E7%B1%BB%E5%9E%8B%E8%BD%AC%E6%8D%A2)


## 6.expression
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800451&subQuestionName=expression
- 页面标题: expression
- 统计信息: 通过率：19.0%难度：简单题目总分：10

实现表达式功能。
各种表达式运算是 SQL 的基础，有了表达式，才能使用 SQL 描述丰富的应用场景。
这里的表达式仅考虑算数表达式，可以参考现有实现的 calc 语句，可以参考 [表达式解析](https://oceanbase.github.io/miniob/design/miniob-sql-expression) ，在 SELECT 语句中实现。
如果有些表达式运算结果有疑问，可以在 MySQL 中执行相应的 SQL，然后参考 MySQL 的执行即可。比如一个数字除以 0，应该按照NULL 类型的数字来处理。
当然为了简化，这里只有数字类型的运算。
示例 SQL 语句：
```
SELECT col3 * 4 FROM exp_table WHERE 5 + col2 < col1 + 6;
```


## 7.function
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800449&subQuestionName=function
- 页面标题: function
- 统计信息: 通过率：16.0%难度：简单题目总分：10

实现一些常见的函数，包括 length、round 和 date_format。
函数是 SQL 中常见功能之一。这些函数除了用在查询字段上，还可能会出现在条件语句中，作为查询数据过滤条件之一。
为了简化，仅考虑上述三种函数即可，其中 length 只考虑 char 类型，round 只考虑 float 类型，date_format 只考虑 date 类型，遇到其他的数据类型返回 FAILURE 即可。
示例 SQL 语句：
```
SELECT id, LENGTH(name), ROUND(score), DATE_FORMAT(u_date, '%D,%M,%Y') FROM function_table;
```

可以参考：[22年赛题说明-函数](https://github.com/oceanbase/miniob/wiki/OceanBase--%E6%95%B0%E6%8D%AE%E5%BA%93%E5%A4%A7%E8%B5%9B-2022-%E5%88%9D%E8%B5%9B%E8%B5%9B%E9%A2%98#10-%E5%87%BD%E6%95%B0)
DATE_FORMAT 函数补充说明（ [MySQL文档](https://dev.mysql.com/doc/refman/9.4/en/date-and-time-functions.html#function_date-format) ）
只需实现如下格式符即可：

|格式符 |含义 |示例 |
|`%Y` |四位年份 |`2024` |
|`%y` |两位年份 |`24` |
|`%m` |两位月份 |`05` |
|`%d` |两位日期 |`20` |
|`%D` |带英文后缀的日期 |`20th` |
|`%M` |完整月份名（英文） |`December` |
对于 %z 和 %n 这类非法格式符不进行替换，原样输出
```
SELECT DATE_FORMAT('2024-05-20', 'abc %z def %n ghi');
abc z def n ghi
```


## 8.multi-index
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800456&subQuestionName=multi-index
- 页面标题: multi-index
- 统计信息: 通过率：30.4%难度：中等题目总分：20

多字段索引功能。即一个索引中同时关联了多个字段。
此功能除了需要修改语法分析，还需要调整 B+ 树相关的实现，帮助同学们增加 B+ 树数据存储知识的理解。
示例 SQL 语句：
```
CREATE INDEX i_1_12 ON multi_index(col1, col2);
```


## 9.unique
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800457&subQuestionName=unique
- 页面标题: unique
- 统计信息: 通过率：9.1%难度：中等题目总分：20

实现唯一索引功能。
唯一索引是指一个索引上的数据都不是重复的。支持使用简单的 SQL 创建索引。
注意：需要支持多列的唯一索引。为了简化场景，不考虑在已有重复数据的列上建立唯一索引的情况。
需要考虑数据插入、数据更新等场景。此功能主要涉及到 B+ 树与语法解析方面的模块。
注意：本题目需要实现 drop index 功能。
示例 SQL 语句：
```
CREATE UNIQUE INDEX t_i ON t(id);
DROP INDEX t_i ON t;
```


## 10.group-by
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800453&subQuestionName=group-by
- 页面标题: group-by
- 统计信息: 通过率：9.4%难度：中等题目总分：20

**本题目要求实现聚合函数功能和数据分组（group by）功能。**
**聚合函数功能要求：**
1. 实现聚合函数 max/min/count/avg/sum。
2. 聚合函数会遍历所有相关的行数据做相关的统计，并输出结果。
3. 对于如下这样的聚合和单个字段混合的测试语句，返回FAILURE。
```
SELECT id, COUNT(age) FROM t;
SELECT COUNT(id) FROM t1 GROUP BY name HAVING COUNT(id) > 2;
```

4. 测试用例中不会包含一些比较复杂的处理，比如表达式。但是有些数据类型会有隐式转换，比如avg计算整数类型时，结果会是浮点数。
5. 注意处理语义处理时的异常场景，比如:

- 查询不存在的字段；
- 查询空字段；

**group by 功能要求：**
分组功能也是数据库的基本功能之一，目的是为了方便用户查询数据结果，按照一定条件进行分组，方便分析数据。
按照一个或多个字段对查询结果分组，group by中的聚合函数不要求支持表达式。
需要支持having子句，因为聚合函数不能出现在where后面，所以增加having子句用于筛选分组后的数据。不过having只和聚合函数一起出现。
注意需要考虑分组字段为null的情况。
示例：
```
SELECT t.id, t.name, AVG(t.score), AVG(t2.age) FROM t, t2 WHERE t.id = t2.id GROUP BY t.id, t.name;
SELECT COUNT(id) FROM t1 GROUP BY name HAVING COUNT(id) > 2;
```


## 11.simple-sub-query
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800455&subQuestionName=simple-sub-query
- 页面标题: simple-sub-query
- 统计信息: 通过率：15.1%难度：中等题目总分：20

简单子查询。此功能是对基础查询功能的一个扩充，使数据库 SQL 的能力更加丰富。
这里需要支持的功能包括但不限于：
   - 支持简单的 IN (NOT IN) 语句，不涉及基本类型转换。注意 NOT IN 语句面对 NULL 时的特殊性。
   - 支持与子查询结果做比较运算。 注意子查询结果为多行的情况。
   - 支持子查询中带聚合函数。
   - 子查询中不会与主查询做关联。这也是简单子查询区分于复杂子查询的地方。
   - 表达式中可能存在不同类型值比较。
示例 SQL 语句：
```
SELECT * FROM ssq_1 WHERE id IN (SELECT ssq_2.id FROM ssq_2);
```


## 12.alias
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800459&subQuestionName=alias
- 页面标题: alias
- 统计信息: 通过率：9.4%难度：中等题目总分：20

实现字段、表别名的功能。
别名功能看起来不是数据库的必备功能，但是它可以极大地方便我们使用。比如美化或简化数据结构的输出、优化查询语句的编写。
表和列可以临时取别名，在打印结果时表和字段都打印别名（如果有）。在查询时能够使用表的别名访问表的字段。两个表的别名在同一层查询中不能重复，子查询里面和外面的表的别名可以重复。列的别名只需要支持查询结果显示，不需要考虑使用列别名进行运算和比较，也不考虑列的别名重复。需要考虑表别名对运算和比较的影响。
注意：本题前置依赖 simple_sub_query中的 in 和子查询。
示例：
```
SELECT column_name AS col FROM table_name;
SELECT t.column_name FROM table_name AS t;
```


## 13.null
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800460&subQuestionName=null
- 页面标题: null
- 统计信息: 通过率：10.1%难度：中等题目总分：20

NULL 是数据库的一个基本功能。
表字段可以有NULL属性，表示此字段是否允许为 NULL 值。NULL 在做数值运算、逻辑比较时，都有特殊的含义，同时在做聚合运算(count/avg 等）都需要做不同的处理。使用 NULL 关键字，不区分大小写。
注意：
   - NULL 与任何数值比较，结果都是 false。
   - NULL 用例非常基础，它出现在许多其它用例中。
示例：
```
CREATE TABLE t(id INT NULL, name CHAR NOT NULL);
```

其中字段 id 可以为 NULL 值，而 name 字段不允许为 NULL 值。


## 14.union
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800452&subQuestionName=union
- 页面标题: union
- 统计信息: 通过率：6.9%难度：中等题目总分：20

UNION 操作符用于连接两个以上的 SELECT 语句的结果组合到一个结果集合，并去除重复的行，而 UNION ALL 不去除重复行。
UNION (UNION ALL) 操作符必须由两个或多个 SELECT 语句组成，每个 SELECT 语句的列数和对应位置的数据类型必须相同。
为了简化，本题不会出现SELECT之间对应位置的数据类型不同的情况。
注意：
UNION 的操作是合并两个查询结果，并自动去除所有重复的行（基于整行完全相同），所以单个表中的重复数据也会被去重。
UNION (UNION ALL) 执行顺序从左到右。为了简化，本题不存在使用括号改变其执行顺序。
示例 SQL 语句：
```
SELECT * FROM t UNION SELECT * FROM t1 UNION ALL SELECT * FROM t2;
```


## 15.order-by
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800462&subQuestionName=order-by
- 页面标题: order-by
- 统计信息: 通过率：16.8%难度：中等题目总分：20

实现排序功能。
排序也是数据库的一个基本功能，就是将查询的结果按照指定的字段和顺序进行排序。
示例：
```
SELECT * from t, t1 WHERE t.id = t1.id ORDER BY t.id ASC, t1.score DESC;
```

示例中就是将结果按照 t.id 升序、t1.score 降序的方式排序。
其中 asc 表示升序排序，desc 表示降序。如果不指定排序顺序，就是升序，即 asc。
在MySQL中，可以使用 order by 1,2 的方式，指定排序字段，我们为了简化，不实现这种功能。


## 16.vector-basic
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800450&subQuestionName=vector-basic
- 页面标题: vector-basic
- 统计信息: 通过率：8.9%难度：中等题目总分：20

### 向量数据库题目一：向量类型基础功能
实现向量类型：
1. 支持创建包含向量类型的表。
2. 支持插入向量类型的记录。

- 实现距离表达式计算：

    DISTANCE(v1, v2, COSINE/EUCLIDEAN/DOT)

- 实现 VECTOR_TO_STRING，STRING_TO_VECTOR，参考MySQL。

    所有输入的向量都由 STRING_TO_VECTOR 包裹
    查询向量列时，对应列由 VECTOR_TO_STRING 包裹
SQL 示例：
```
CREATE TABLE TEST (id INT, C1 VECTOR(3));
INSERT INTO TEST VALUES(1, STRING_TO_VECTOR('[1, 2, 3]'));
SELECT DISTANCE(STRING_TO_VECTOR('[1, 2, 3]'), STRING_TO_VECTOR('[2, 3, 4]'), 'COSINE') AS DIST_COSINE;

SELECT ID, VECTOR_TO_STRING(C1) AS VEC_STR, DISTANCE(C1, STRING_TO_VECTOR('[1, 2, 3]'), 'EUCLIDEAN') AS DIST_EUC FROM TEST;
1, [3.07000e+00,-1.24000e+00], 2.31
```

对于 MySQL 向量的科学计数法显示（如：[-4.94000e+00, 9.23000e+00] ），可以参考如下代码：
```
std::string formatFloatToString(float value) const {
    std::ostringstream oss;    // 设置格式：
    oss << std::scientific    // 科学计数法
        << std::setprecision(5)  // 保留 5 位小数，例如 3.61000
        << value;
    return oss.str();
}
```

详情请参考文档：
[https://oceanbase.github.io/miniob/game/miniob-vectordb-2025](https://oceanbase.github.io/miniob/game/miniob-vectordb-2025/)


## 17.text
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800461&subQuestionName=text
- 页面标题: text
- 统计信息: 通过率：13.8%难度：中等题目总分：20

实现文本字段。
在数据库中，我们会有存储超大数据的需求，比如在数据库中存放网页。这里考虑使用 text 字段，存放超大数据。参考 MySQL 的实现，text 字段的最大长度为 65535 个字节，插入超出这个长度的数据就报错。
这里除了需要实现语法解析，还需要考虑如何在存储引擎中存放超长字段，扩展 record_manager，以支持超过一页的数据。
示例：
```
CREATE TABLE t(id INT, article TEXT);
```


## 18.vector-search
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800463&subQuestionName=vector-search
- 页面标题: vector-search
- 统计信息: 通过率：8.7%难度：中等题目总分：20

### 向量数据库题目二：向量检索

- 需要在没有索引的场景下，支持向量检索功能（即精确检索）。

向量检索示例：
```
SELECT ID FROM TAB_VEC ORDER BY DISTANCE(B, STRING_TO_VECTOR('[10, 0.0, 5.0]'), 'EUCLIDEAN') LIMIT 1;
```

详情请参考文档：
 [https://oceanbase.github.io/miniob/game/miniob-vectordb-2025](https://oceanbase.github.io/miniob/game/miniob-vectordb-2025/)


## 19.alter
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800458&subQuestionName=alter
- 页面标题: alter
- 统计信息: 通过率：5.3%难度：中等题目总分：20

当我们需要修改数据表名或者修改数据表字段时，就需要使用到 alter 命令。
ALTER 命令用于修改数据库、表和索引等对象的结构。
ALTER 命令允许你添加、修改或删除数据库对象，并且可以用于更改表的列定义、添加约束、创建和删除索引等操作。
ALTER 命令非常强大，可以在数据库结构发生变化时进行灵活的修改和调整。
为了简化，本题不涉及对字段数据类型的修改，且所有操作只基于表。
本题单次操作最多只会修改一个列，且表上只会构建单列索引，注意更改表结构后相应索引的变化。
只需实现如下四种 SQL 语句：
```
ALTER TABLE alter_table_1 ADD COLUMN col INT;
ALTER TABLE alter_table_1 DROP COLUMN col;
ALTER TABLE alter_table_1 CHANGE COLUMN col id INT; // 将 col 列改名为 id，不涉及类型修改
ALTER TABLE alter_table_1 RENAME TO alter_table_2;
```


## 20.update-mvcc
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800464&subQuestionName=update-mvcc
- 页面标题: update-mvcc
- 统计信息: 通过率：6.0%难度：困难题目总分：30

实现MVCC中的更新功能。
事务是数据库的基本功能，此功能希望同学们补充 MVCC（多版本并发控制）的 update 功能。这里主要考察不同连接同时操作数据库表时的问题。
事务管理在MiniOB并没有完善的实现，比如原子性提交、持久化、垃圾回收等。如果有兴趣的同学，可以给 MiniOB 提交 PR。
注意：
   - 测试多连接，但不会测试多并发（即程序是串行执行的）；
   - 启动 observer 程序时，需要增加 -t mvcc 参数 ，比如 ./bin/observer -f ../etc/observer.ini -s miniob.sock -t mvcc
   - 测试过程中如果遇到官方代码自有的BUG，请修复它，也欢迎提PR。


## 21.complex-sub-query
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800466&subQuestionName=complex-sub-query
- 页面标题: complex-sub-query
- 统计信息: 通过率：4.8%难度：困难题目总分：30

实现复杂子查询功能。
复杂子查询是简单子查询的升级。与其最大的不同就在于子查询中会跟复查询联动。注意需要考虑查询条件中带有聚合函数的情况。
本题会考察对 EXISTS (NOT EXISTS) 语句的支持。
示例：
```
SELECT * FROM t1 WHERE age IN (SELECT id FROM t2 WHERE t2.name IN (SELECT name FROM t3));
```

备注：查询条件只会使用 and 或 or，没有包含 and 和 or 混合的情况。


## 22.create-view
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800467&subQuestionName=create-view
- 页面标题: create-view
- 统计信息: 通过率：0.8%难度：困难题目总分：30

实现视图功能。
视图是数据库的基本功能之一。视图可以极大地方便数据库的使用。
视图，顾名思义，就是一个能够自动执行查询语句的虚拟表。
不过视图功能也非常复杂，需要考虑视图更新时如果更新实体表。如果视图对应了单张表，并且没有虚拟字段，更新视图，即更新了实体表。如果实体表中某些字段不在视图中，那此字段的结果应该是 NULL 或默认值。如果视图中包含虚拟字段，比如通过聚合查询的结果，或者视图关联了多张表，他的更新规则就变得复杂起来。在这些场景中，同学们可以参考MySQL的实现方案。
示例：
```
CREATE VIEW create_view_v4 AS select t1.id AS id, t1.age AS age, t2.name AS name FROM create_view_t1 t1, create_view_t2 t2 WHERE t1.id = t2.id;
INSERT INTO create_view_v4(id, age) VALUES(1, 1);
UPDATE create_view_v4 SET id=1, age=1;
```

详情请参考文档：
[https://oceanbase.github.io/miniob/game/create-view-2025](https://oceanbase.github.io/miniob/game/create-view-2025/)
MySQL官方文档：
 [https://dev.mysql.com/doc/refman/8.0/en/view-updatability.html](https://dev.mysql.com/doc/refman/8.0/en/view-updatability.html)


## 23.full-text-index
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800468&subQuestionName=full-text-index
- 页面标题: full-text-index
- 统计信息: 通过率：1.0%难度：困难题目总分：30

全文索引（Full-Text Search）是一种用于在大量文本数据中进行高效搜索的技术。它通过基于相似度的查询，而不是精确数值比较，来查找文本中的相关信息。相比于使用 LIKE + % 的模糊匹配，全文索引在处理大量数据时速度更快。
简化流程如下：
#### 1.1 **文本预处理**

- **分词（Tokenization）**：将文本数据拆分为单个的词语或短语，这些词语成为索引的基本单位。例如，“全文索引的原理”可能会被拆分为“全文”、“索引”、“原理”等词条。
- **去除停用词（Stop Words Removal）**：停用词是指在搜索中不太有意义的常用词汇，如“的”、“是”等。去除这些词可以减少索引的规模，并提高搜索效率。

#### 1.2 **倒排索引（Inverted Index）**
倒排索引是全文索引的核心数据结构。它通过记录每个词条在哪些文档中出现来实现快速查询。

- **词典（Dictionary）**：保存所有出现过的词条，以及这些词条的文档频率。
- **倒排列表（Posting List）**：对于每个词条，倒排列表保存了包含该词条的文档ID，甚至可能包含词条在文档中出现的位置和频率等信息。

#### 1.3 **查询处理**

- **排名和排序**：全文索引系统通常会根据词频、文档长度、词条的逆文档频率（IDF）等因素对查询结果进行评分和排序，返回最相关的文档。

#### **功能要求：**
1. 支持使用 jieba 进行中文分词
2. 支持创建全文索引，支持全文索引查询
3. 支持使用 BM25 评分
SQL示例：
```
SELECT TOKENIZE('information_schema.SCHEMATA表的主要功能是什么？', 'jieba') as text_tokens;
["information", "schema", "SCHEMATA", "表", "主要", "功能"] // 结果之间存在空格

ALTER TABLE texts ADD FULLTEXT INDEX idx_texts_jieba (content) WITH PARSER jieba;
SELECT id, content, MATCH(content) AGAINST('你好') AS score FROM texts WHERE MATCH(content) AGAINST('你好') > 0 ORDER BY MATCH(content) AGAINST('你好') DESC, id ASC;
```

注意：
1. 本题只需实现使用 [cppjieba](https://github.com/yanyiwu/cppjieba) 库进行 jieba 分词。
初始代码已经实现 cppjieba 的对接，提测能正常通过编译。cppjieba 所需词库位于 /usr/local/dict/ 下。
2. 本题只需使用 BM25 评分进行排序，MATCH(content) AGAINST('xxx') 返回结果为该文档的 BM25 评分
BM25 分数计算公式如下：

![1760683225](https://obcommunityprod.oss-cn-shanghai.aliyuncs.com/prod/competiondetail/2025-10/9569c708-0bf6-4e42-8d14-06bbab0af2f4.png)

其中 k1 = 1.5，b = 0.75
3. 本题只会对单列建全文索引，不考虑多列索引。
测评端代码标准：

- 测评端 jieba 分词： [https://github.com/HuXin0817/cppjieba/](https://github.com/HuXin0817/cppjieba/)
- BM25：  [https://github.com/dorianbrown/rank_bm25/tree/0.2.2/](https://github.com/dorianbrown/rank_bm25/tree/0.2.2)


## 24.big-order-by
- questionId: 600060
- URL: https://open.oceanbase.com/train/TopicDetails?questionId=600060&subQesitonId=800465&subQuestionName=big-order-by
- 页面标题: big-order-by
- 统计信息: 通过率：6.4%难度：困难题目总分：30

大数据量的排序功能。
在内存有限的情况下，实现大数据量的排序，需要优化内存使用。注意，测试数据具备一定的随机性。
有四张表，每张表有 20 个字段，数据量在 20 左右，所有表在一起做笛卡尔积查询，并且对每个字段都会做 order by 排序。通常这么大的数据量不能在纯内存中排序完成，所以需要考虑使用外部排序。
注意：本题内存限制在 350MB，返回结果中的调用栈出现 memtracer::MemTracer::alloc(unsigned long) (下图 #5) 时说明超出内存限制。
```
#0  __pthread_kill_implementation (no_tid=0, signo=6, threadid=) at ./nptl/pthread_kill.c:44
#1  __pthread_kill_internal (signo=6, threadid=) at ./nptl/pthread_kill.c:78
#2  __GI___pthread_kill (threadid=, signo=signo@entry=6) at ./nptl/pthread_kill.c:89
#3  0x00007fdc46c7527e in __GI_raise (sig=sig@entry=6) at ../sysdeps/posix/raise.c:26
#4  0x00007fdc46c588ff in __GI_abort () at ./stdlib/abort.c:79
#5  0x00007fdc471df84b in memtracer::MemTracer::alloc(unsigned long) () from /usr/lib/libmemtracer.so
#6  0x00007fdc471de6fe in malloc () from /usr/lib/libmemtracer.so
```

本题预估数据量为 (20)^4 = 1.6e5 行，常数较高 (O(80)) 。中间/最终结果预估达到 51.2 MB。
关于内存限制可以参考：
 [https://oceanbase.github.io/miniob/game/miniob-memtracer ](https://oceanbase.github.io/miniob/game/miniob-memtracer/)
或者MiniOB目录下 docs/docs/game/miniob-memtracer.md


---

# 官方补充文档（题面中引用的外部文档全文）


## 附录 A：MiniOB 向量数据库文档（vector-basic / vector-search 参考）

来源: https://oceanbase.github.io/miniob/game/miniob-vectordb-2025/

# MiniOB 向量数据库¶

## 向量搜索¶

向量搜索技术是非结构化数据检索的关键技术之一，通常通过近似最近邻搜索（Approximate Nearest Neighbor Search, ANN）的方式来在高维空间中进行检索，以此来找到满足要求的数据。向量搜索在检索相似的图片、音频和文本等方面发挥着关键作用。例如，在图像检索中，我们可以通过计算图像的特征向量，然后使用向量检索技术来找到与查询图像最相似的图像；在推荐系统中，我们可以通过计算用户和物品的特征向量，然后使用向量检索技术来找到与用户兴趣最相似的其他用户或物品。下面针对向量搜索技术的基本概念进行简要介绍。

### 向量（Vector）是什么¶

向量是一种表示多维特征的数据结构。每个向量由一组数值组成，这些数值通常对应于某种特定的特征或属性。例如，在图像处理中，一个向量可以表示图像的颜色、纹理等特征。为了更有效地管理非结构化数据，常见的做法是将其转换为向量表示，并存储在向量数据库中，这种转换过程通常被称为 Embedding。通过将文本、图像或其他非结构化数据映射到高维向量空间，我们可以捕捉数据的语义特征和潜在关系。单词、短语或整个文档以及图像、音频和其他类型的数据都可以表示成向量，例如，我们可以将下面的文本表示成向量：

```
"West Highland White Terrier": [0.0296700,0.0231020,0.0166550,0.0642470,-0.0110980, ... ,0.0253750]

```

### 向量数据库是什么¶

向量数据库是可以高效存储/检索向量的数据库。向量数据库用专门的数据结构和算法来处理向量之间的相似性计算和查询。通过构建索引结构，向量数据库可以快速找到最相似的向量。

### 向量搜索的应用¶

向量在检索相似的图片、音频和文本等方面发挥着关键作用，这源于其数据属性和特征表示能力。在机器学习和数据科学领域，向量被广泛用于描述数据特征。以图片数据为例，我们可以将其表示为向量。在计算机中，图片本质上是由像素构成的二维矩阵。每个像素的亮度值可视为图片的一个特征，因此，我们可以将这些亮度值串联成一个高维向量，从而实现图片的向量化表示。这种向量化表示使我们能够利用向量空间中的距离和相似度度量方法来比较不同图片之间的相似程度。例如，欧氏距离可用于衡量两个图片向量间的像素差异，而余弦相似度则可测量它们的方向差异。通过计算向量间的距离或相似度，我们可以量化评估不同图片之间的相似程度。

除此之外，检索增强生成（Retrieval-Augmented Generation）已经成为大语言模型应用的范式之一。而向量数据库是 RAG 应用中重要组成部分，向量数据库的存储容量、召回精度和召回速度在很大程度上影响了大语言模型应用的服务质量。

## MiniOB 向量数据库赛题¶

本次赛题，需要选手在 MiniOB 的基础上实现向量数据库的基本功能，向量数据库的功能被拆解为如下几个题目。

注意：在实现向量数据库相关题目时，不限制向量检索算法的实现方式，可以基于开源的第三方库实现，也可以自行实现。

### MiniOB 向量类型¶

VECTOR 是一种结构，最多可以容纳指定数量的条目 N，定义如下：

```
VECTOR(N)

```

每个条目是一个 4 字节（单精度）浮点数值。

默认长度为 2048；最大条目数为 16383。要声明一个使用默认长度的 VECTOR 列，应定义为

```
VECTOR
```
而不带括号；尝试以
```
VECTOR()
```
（空括号）方式定义列将引发语法错误。
VECTOR 不能与任何其他类型进行比较。它可以与其他 VECTOR 进行相等性比较，但不支持其他比较操作。

- 支持创建包含向量类型的表：

```
CREATE TABLE items (id int, embedding vector(3));

```

- 支持插入向量类型的记录（注意：这里需要支持
 ```
 STRING_TO_VECTOR
 ```
 函数将字符串类型的值转换为向量类型存储）：

```
INSERT INTO items VALUES (1, STRING_TO_VECTOR('[1,2,3]'));

```

- 支持用于处理 VECTOR 值的 SQL 函数。
|名称|描述|
|--|--|
|DISTANCE()|根据指定的方法计算两个向量之间的距离|
|STRING_TO_VECTOR()|获取由符合格式的字符串表示的 VECTOR 列的二进制值|
|VECTOR_TO_STRING()|获取 VECTOR 列的字符串表示，给定其二进制值|

**DISTANCE(vector, vector, string)**

计算两个向量之间的距离，根据指定的计算方法。它接受以下参数：

- 一个 VECTOR 数据类型的列。
- 一个 VECTOR 数据类型的输入查询。
- 一个字符串，指定了距离度量方式。支持的值有 COSINE、DOT 和 EUCLIDEAN。由于该参数是字符串，因此必须加引号。
- l2_distance

 - 语法：l2_distance(vector A, vector B)
 - 计算公式：\([ D = \sqrt{\sum_{i=1}^{n} (A_{i} - B_{i})^2} ]\)
- cosine_distance：

 - 语法：cosine_distance(vector A, vector B)
 - 计算公式：\([ D = 1 - \frac{\mathbf{A} \cdot \mathbf{B}}{|\mathbf{A}| |\mathbf{B}|} = 1 - \frac{\sum_{i=1}^{n} A_i B_i}{\sqrt{\sum_{i=1}^{n} A_i^2} \sqrt{\sum_{i=1}^{n} B_i^2}} ]\)
- inner_product：

 - 语法：inner_product(vector A, vector B)
 - 计算公式：\([ D = \mathbf{A} \cdot \mathbf{B} = a_1 b_1 + a_2 b_2 + ... + a_n b_n = \sum_{i=1}^{n} a_i b_i ]\)

**VECTOR_DISTANCE 是此函数的同义词。**

```
SELECT DISTANCE(STRING_TO_VECTOR("[1.01231, 2.0123123, 3.0123123, 4.01231231]"), STRING_TO_VECTOR("[1, 2, 3, 4]"), "COSINE");

```

**STRING_TO_VECTOR(string)**

将向量的字符串表示转换为二进制形式。字符串的预期格式是一个或多个逗号分隔的浮点数值列表，用方括号（[ ]）包围。值可以用十进制或科学记数法表示。由于该参数是字符串，因此必须加引号。

**TO_VECTOR() 是此函数的同义词。**

VECTOR_TO_STRING() 是此函数的逆操作：

```
SELECT VECTOR_TO_STRING(STRING_TO_VECTOR("[1.05, -17.8, 32]"));

```

此类值中的所有空白字符（数字后、方括号前或后，或两者的任意组合）在使用时都会被修剪。

**VECTOR_TO_STRING(vector)**

给定一个 VECTOR 列值的二进制表示，此函数返回其字符串表示，该格式与 STRING_TO_VECTOR() 函数的参数格式相同。

**FROM_VECTOR() 被接受为此函数的同义词。**

无法解析为向量值的参数会引发错误。

此函数的输出最大大小为 262128（16 * 16383）字节。

## 参考资料¶

支持 MiniOB 的 ann-benchmarks fork 仓库


## 附录 B：视图更新规则说明（create-view 参考）

来源: https://oceanbase.github.io/miniob/game/create-view-2025/

# 视图更新规则说明¶

本规则用于判断不同类型的视图是否支持

```
INSERT
```

```
UPDATE
```
和
```
DELETE
```
操作。
核心原则：**只有当数据库能明确地将操作映射回基表的某一行时，才允许更新。**
> ⚠️ 说明： 1. 即使视图为空，不可删除的操作也会执行失败。 2. 本题目暂不考虑
> ```
> GROUP BY
> ```
> 、
> ```
> ORDER BY
> ```
> 、
> ```
> HAVING
> ```
> 、
> ```
> LIMIT
> ```
> 等子句。 3. 嵌套视图的可更新性依赖其源视图；本题目暂不考虑嵌套视图更新。

## 视图类型与操作权限对照表¶
|视图类型|定义示例|INSERT（插入）|UPDATE（更新）|DELETE（删除）|详细说明|
|--|--|--|--|--|--|
|单表视图|``` CREATE VIEW v AS SELECT * FROM t; ```|✅ 允许|✅ 允许|✅ 允许|基于单个表的完整列视图，完全可更新|
|单表视图（部分列）|``` CREATE VIEW v(id, age) AS SELECT id, age FROM t; ```|✅ 允许（仅指定列）❌ 不允许（全列且未覆盖）|✅ 允许（仅修改包含的列）❌ 不允许（修改非包含列）|✅ 允许|插入时，其他列为
``` NULL ``` ；若缺失的列为
``` NOT NULL ``` 且无默认值，则插入失败|
|多表视图|``` CREATE VIEW v AS SELECT t1.id, t2.age FROM t1, t2; ```|⚠️ 部分允许✅ 若只影响一个基表的列❌ 若涉及多个基表|⚠️ 部分允许✅ 若只更新来自同一基表的列❌ 若跨多个基表更新|❌ 不允许|插入或更新只能作用于单一基表对应的字段。例如：
``` INSERT INTO v(id) ``` 可能允许（仅 t1），但
``` INSERT INTO v VALUES(...) ``` 同时写两表则禁止|
|单表视图（含表达式）|``` CREATE VIEW v AS SELECT id, id + age AS data FROM t; ```|❌ 不允许插入|✅ 允许（仅更新基础列）❌ 不允许（更新表达式列）|✅ 允许|表达式列（如
``` id + age ``` ）是计算值，不能写入；只能对原始列（如
``` id ``` ,
``` age ``` ）进行更新|
|单表视图（含聚合）|``` CREATE VIEW v AS SELECT COUNT(*) AS cnt FROM t; ```|❌ 不允许|❌ 不允许|❌ 不允许|聚合结果无法映射回原表的具体行；此类视图为只读|
|嵌套视图|``` CREATE VIEW v1 AS SELECT id FROM v2; ```|✅ 允许（当 v2 可插入）❌ 不允许（当 v2 不可插入）|✅ 允许（当 v2 可更新）❌ 不允许（当 v2 不可更新）|✅ 允许（当 v2 可删除）❌ 不允许（当 v2 不可删除）|嵌套视图的操作权限完全依赖源视图。若源视图
``` v2 ``` 支持某操作，则
``` v1 ``` 可能支持；否则一律禁止|

## 关键术语解释¶
- **基表（Base Table）**：视图所基于的真实数据表。
- **可更新视图（Updatable View）**：指对该视图的 DML 操作能够成功传递到基表并生效。
- **表达式列**：由计算生成的列，如
 ```
 price * qty
 ```
 、
 ```
 UPPER(name)
 ```
 、
 ```
 col + 1
 ```
 等。
- **聚合列**：使用聚合函数生成的列，如
 ```
 COUNT(*)
 ```
 、
 ```
 SUM(amount)
 ```
 、
 ```
 AVG(score)
 ```
 等。

## 判断流程建议¶

面对任意视图定义，请按以下顺序判断其可更新性：

1. **看来源**：是单表还是多表？
 → 多表 → 插入/删除通常 ❌，更新需谨慎。
2. **看列**：是否有表达式或聚合函数？
 → 有表达式 → 表达式列 ❌ 不可更新
 → 有聚合 → 整个视图 ❌ 不可插入、不可更新、不可删除
3. **看结构**：是否只是原表的一部分列？
 → 是 → 插入时注意缺失列是否允许为
 ```
 NULL
 ```
4. **看嵌套**：是否基于另一个视图？
 → 是 → 权限继承自源视图
5. **最终结论**：
 只有当操作能唯一、明确地映射回基表的一行，并且不涉及虚拟列时，才允许更新。

## 示例速查¶
|操作语句|是否允许|原因简述|
|--|--|--|
|``` INSERT INTO v(id) VALUES(1); ``` （v 是
``` SELECT id, age FROM t ``` ）|✅|仅插入允许的列，其余列设为 NULL|
|``` INSERT INTO v VALUES(1, 2); ``` （v 是多表连接视图）|❌|涉及多个基表，无法确定插入目标|
|``` UPDATE v SET data = 10; ``` （v 含
``` id+age AS data ``` ）|❌|``` data ``` 是表达式列，不可写|
|``` UPDATE v SET id = 2; ``` （v 是单表部分列视图）|✅|``` id ``` 是原始列，可正常更新|
|``` DELETE FROM v; ``` （v 是聚合视图）|❌|聚合视图无具体行对应，不可删|
|``` INSERT INTO v1(id) ... ``` （v1 基于可插入的 v2）|✅|源视图可插入，嵌套视图也可插入|


## 附录 C：MemTracer 内存限制文档（big-order-by 及全局 350MB 限制参考）

来源: https://oceanbase.github.io/miniob/game/miniob-memtracer/

# MemTracer¶

MemTracer 是一个动态链接库，被用于监控 MiniOB 内存使用；用于在内存受限的条件下，运行和调试 MiniOB。MemTracer 通过 hook 内存分配释放函数，记录 MiniOB 进程中的内存分配情况。

## 原理介绍¶

MemTracer 对内存分配释放函数进行了覆盖（override），以达到对内存动态分配释放（如

```
malloc/free
```

```
new/delete
```
）的监控。除此之外，MemTracer 还会将 MiniOB 进程中代码段等内存占用统计在内。
通过在

```
LD_PRELOAD
```
环境变量中指定 MemTracer 动态库来覆盖 glibc 中的符号，可以实现可插拔方式监控 MiniOB 进程的内存占用。
MemTracer 支持设置最大内存限额，当 MiniOB 进程申请超过内存限额的内存时，MemTracer 会调用

```
exit(-1)
```
使 MiniOB 进程退出。

## 使用介绍¶

### 编译¶

可通过指定

```
WITH_MEMTRACER
```
控制 MemTracer 的编译（默认编译），MemTracer 动态库默认输出在
```
${CMAKE_BINARY_DIR}/lib
```
目录下。
下述示例将关闭MemTracer 的编译。

```
sudo bash build.sh init
bash build.sh release -DWITH_MEMTRACER=OFF

```

### 运行¶

通过指定

```
LD_PRELOAD
```
环境变量， 将 MemTracer 动态库加载到 MiniOB 进程中。如：

```
LD_PRELOAD=./lib/libmemtracer.so ./bin/observer

```

通过指定
```
MT_PRINT_INTERVAL_MS
```
环境变量，设置内存使用的打印间隔时间，单位为毫秒（ms），默认为 5000 ms（5s）。通过指定
```
MT_MEMORY_LIMIT
```
环境变量，设置内存使用的上限，单位为字节，当超过该值，MiniOB 进程会立即退出。下述示例表明设置内存使用情况的打印间隔为 1000 ms（1s），内存使用上限为1000 字节。

```
MT_PRINT_INTERVAL_MS=1000 MT_MEMORY_LIMIT=1000 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer

```

### 使用场景示例¶
1. 通过指定 MiniOB 进程的最大内存限额，可以模拟在内存受限的情况下运行、调试 MiniOB。当超出最大内存限额后，MiniOB 进程会自动退出。

指定最大内存限额：

```
MT_MEMORY_LIMIT=100000000 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer

```

当由于申请内存超过限额退出，则在退出时会打印相关日志：

```
[MEMTRACER] alloc memory:24, allocated_memory: 31653580, memory_limit: 31653600, Memory limit exceeded!

```

2. 通过链接 MemTracer，可以在 MiniOB 进程的任意位置获取当前内存使用情况。

步骤1: 链接 libmemtracer.so 到 MiniOB 进程中。

步骤2: 在需要获取内存使用情况的位置，调用

```
memtracer/mt_info.h
```
头文件中的
```
memtracer::allocated_memory()
```
函数，获取当前内存使用情况。
示例代码如下：

```
diff --git a/src/observer/CMakeLists.txt b/src/observer/CMakeLists.txt
index c62ac3c..d105fbf 100644
--- a/src/observer/CMakeLists.txt
+++ b/src/observer/CMakeLists.txt
@@ -20,7 +20,7 @@ FIND_PACKAGE(Libevent CONFIG REQUIRED)

 # JsonCpp cannot work correctly with FIND_PACKAGE

-SET(LIBRARIES common pthread dl libevent::core libevent::pthreads libjsoncpp.a)
+SET(LIBRARIES common pthread dl libevent::core libevent::pthreads libjsoncpp.a memtracer)

 # 指定目标文件位置
 SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
diff --git a/src/observer/sql/parser/parse.cpp b/src/observer/sql/parser/parse.cpp
index def0ed1..4f22799 100644
--- a/src/observer/sql/parser/parse.cpp
+++ b/src/observer/sql/parser/parse.cpp
@@ -15,6 +15,7 @@ See the Mulan PSL v2 for more details. */
 #include "sql/parser/parse.h"
 #include "common/log/log.h"
 #include "sql/expr/expression.h"
+#include "memtracer/mt_info.h"

 RC parse(char *st, ParsedSqlNode *sqln);

@@ -41,6 +42,7 @@ int sql_parse(const char *st, ParsedSqlResult *sql_result);

 RC parse(const char *st, ParsedSqlResult *sql_result)
 {
+ LOG_ERROR("parse sql `%s`, allocated: %lu\n", st, memtracer::allocated_memory());
 sql_parse(st, sql_result);
 return RC::SUCCESS;
 }

```

### 注意¶
1. MemTracer 会记录
 ```
 mmap
 ```
 映射的整个虚拟内存占用, 因此不建议使用
 ```
 mmap
 ```
 管理内存。
2. 不允许使用绕过常规内存分配（
 ```
 malloc
 ```
 /
 ```
 free
 ```
 ,
 ```
 new
 ```
 /
 ```
 delete
 ```
 ）的方式申请并使用内存。如使用
 ```
 brk/sbrk/syscall
 ```
 等。
3. MemTracer 不支持与 sanitizers （ASAN等）一起使用。
4. MemTracer 除统计动态内存的申请释放，也会将进程的代码段等内存占用统计在内。
5. 引入 MemTracer 后，会对内存申请释放函数的性能产生一定影响，在 Github Action 中测试结果如下：

 ```
 // MemTracer 开启 ----------------------------------------------------------------------------------- Benchmark Time CPU Iterations UserCounters... ----------------------------------------------------------------------------------- BM_MallocFree/8 42.1 ns 42.1 ns 17090212 bytes_per_second=181.357M/s BM_MallocFree/64 39.9 ns 39.9 ns 16763826 bytes_per_second=1.49326G/s BM_MallocFree/512 40.1 ns 40.1 ns 17553391 bytes_per_second=11.8925G/s BM_MallocFree/1024 39.9 ns 39.9 ns 17548820 bytes_per_second=23.9052G/s BM_MallocFree/1048576 53.4 ns 53.4 ns 13150036 bytes_per_second=17.8628T/s BM_MallocFree/8388608 53.3 ns 53.3 ns 13155704 bytes_per_second=143.106T/s BM_MallocFree/1073741824 24877 ns 24750 ns 28814 bytes_per_second=39.4565T/s BM_NewDelete/8 40.6 ns 40.6 ns 17270523 bytes_per_second=187.866M/s BM_NewDelete/64 40.5 ns 40.5 ns 17279131 bytes_per_second=1.47047G/s BM_NewDelete/512 40.5 ns 40.5 ns 17276789 bytes_per_second=11.7686G/s BM_NewDelete/1024 40.5 ns 40.5 ns 17279484 bytes_per_second=23.5455G/s BM_NewDelete/1048576 53.8 ns 53.8 ns 13003320 bytes_per_second=17.7138T/s BM_NewDelete/8388608 54.0 ns 54.0 ns 13011425 bytes_per_second=141.183T/s BM_NewDelete/1073741824 24844 ns 24611 ns 29271 bytes_per_second=39.6796T/s // MemTracer 关闭 ----------------------------------------------------------------------------------- Benchmark Time CPU Iterations UserCounters... ----------------------------------------------------------------------------------- BM_MallocFree/8 10.1 ns 10.1 ns 70194674 bytes_per_second=753.492M/s BM_MallocFree/64 10.8 ns 10.8 ns 64456192 bytes_per_second=5.50213G/s BM_MallocFree/512 10.9 ns 10.9 ns 64672143 bytes_per_second=43.8294G/s BM_MallocFree/1024 10.8 ns 10.8 ns 64616114 bytes_per_second=88.0599G/s BM_MallocFree/1048576 23.5 ns 23.5 ns 29758318 bytes_per_second=40.5388T/s BM_MallocFree/8388608 23.5 ns 23.5 ns 29743010 bytes_per_second=324.33T/s BM_MallocFree/1073741824 20574 ns 20436 ns 34738 bytes_per_second=47.7856T/s BM_NewDelete/8 13.4 ns 13.4 ns 50775388 bytes_per_second=570.14M/s BM_NewDelete/64 14.2 ns 14.2 ns 49231328 bytes_per_second=4.18804G/s BM_NewDelete/512 14.3 ns 14.3 ns 49198936 bytes_per_second=33.4308G/s BM_NewDelete/1024 14.3 ns 14.3 ns 49215085 bytes_per_second=66.7825G/s BM_NewDelete/1048576 26.9 ns 26.9 ns 25987611 bytes_per_second=35.4036T/s BM_NewDelete/8388608 26.9 ns 26.9 ns 25997689 bytes_per_second=283.11T/s BM_NewDelete/1073741824 20694 ns 20648 ns 34975 bytes_per_second=47.2954T/s
 ```
