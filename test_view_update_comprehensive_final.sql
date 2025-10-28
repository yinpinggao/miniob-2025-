-- ============================================================
-- MiniOB 视图更新功能完整测试用例
-- 基于 OceanBase 文档和 MySQL 视图更新规则
-- ============================================================

-- 清理环境
DROP VIEW IF EXISTS v1;
DROP VIEW IF EXISTS v2;
DROP VIEW IF EXISTS v3;
DROP VIEW IF EXISTS v4;
DROP VIEW IF EXISTS v5;
DROP VIEW IF EXISTS v6;
DROP VIEW IF EXISTS v7;
DROP VIEW IF EXISTS v8;
DROP VIEW IF EXISTS v9;
DROP VIEW IF EXISTS v10;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
DROP TABLE IF EXISTS t3;

-- ============================================================
-- 测试1: 单表视图 - 完整列（所有 DML 操作）
-- ============================================================
CREATE TABLE t1(id INT, age INT, name CHAR(10));
CREATE VIEW v1 AS SELECT * FROM t1;

-- INSERT 测试
INSERT INTO v1 VALUES(1, 20, 'Alice');
INSERT INTO v1 VALUES(2, 25, 'Bob');
SELECT * FROM v1;
SELECT * FROM t1;

-- UPDATE 测试
UPDATE v1 SET age=21 WHERE id=1;
SELECT * FROM v1 WHERE id=1;

UPDATE v1 SET name='Bobby', age=26 WHERE id=2;
SELECT * FROM v1 WHERE id=2;

-- DELETE 测试
DELETE FROM v1 WHERE id=1;
SELECT * FROM v1;
SELECT * FROM t1;

DROP VIEW v1;
DROP TABLE t1;

-- ============================================================
-- 测试2: 单表视图 - 部分列（NULL 值处理）
-- ============================================================
CREATE TABLE t1(id INT, age INT, name CHAR(10));
CREATE VIEW v2 AS SELECT id, age FROM t1;

-- 插入部分字段，未指定的列应为 NULL
INSERT INTO v2 VALUES(10, 30);
SELECT * FROM t1;

-- 指定字段插入
INSERT INTO v2(id, age) VALUES(11, 31);
SELECT * FROM t1;

-- 更新部分字段
UPDATE v2 SET age=32 WHERE id=10;
SELECT * FROM t1;

-- 删除
DELETE FROM v2 WHERE id=10;
SELECT * FROM t1;

DROP VIEW v2;
DROP TABLE t1;

-- ============================================================
-- 测试3: 表达式视图 - 基础字段可更新，表达式字段不可更新
-- ============================================================
CREATE TABLE t1(id INT, age INT);
INSERT INTO t1 VALUES(1, 20);
INSERT INTO t1 VALUES(2, 25);

CREATE VIEW v3 AS SELECT id, age, id+age AS total FROM t1;

-- 成功：插入基础字段
INSERT INTO v3(id, age) VALUES(3, 30);
SELECT * FROM v3;

-- 失败：插入所有字段（包含表达式）
INSERT INTO v3 VALUES(4, 35, 39);

-- 失败：插入表达式字段
INSERT INTO v3(id, total) VALUES(5, 50);

-- 成功：更新基础字段
UPDATE v3 SET age=21 WHERE id=1;
SELECT * FROM v3 WHERE id=1;

-- 失败：更新表达式字段
UPDATE v3 SET total=100 WHERE id=1;

-- 成功：删除
DELETE FROM v3 WHERE id=3;
SELECT * FROM v3;

DROP VIEW v3;
DROP TABLE t1;

-- ============================================================
-- 测试4: 聚合视图 - 完全只读
-- ============================================================
CREATE TABLE t1(id INT, score INT);
INSERT INTO t1 VALUES(1, 90);
INSERT INTO t1 VALUES(2, 85);
INSERT INTO t1 VALUES(3, 95);

CREATE VIEW v4 AS SELECT COUNT(*) AS cnt, AVG(score) AS avg_score FROM t1;

-- 查询应该成功
SELECT * FROM v4;

-- 失败：所有 DML 操作都不允许
INSERT INTO v4 VALUES(10, 80);
UPDATE v4 SET cnt=10;
DELETE FROM v4;

DROP VIEW v4;
DROP TABLE t1;

-- ============================================================
-- 测试5: GROUP BY 视图 - 只读
-- ============================================================
CREATE TABLE t1(category CHAR(10), score INT);
INSERT INTO t1 VALUES('A', 90);
INSERT INTO t1 VALUES('A', 85);
INSERT INTO t1 VALUES('B', 95);

CREATE VIEW v5 AS SELECT category, AVG(score) AS avg_score FROM t1 GROUP BY category;

-- 查询成功
SELECT * FROM v5;

-- 失败：GROUP BY 视图只读
INSERT INTO v5 VALUES('C', 88);
UPDATE v5 SET avg_score=92 WHERE category='A';
DELETE FROM v5 WHERE category='A';

DROP VIEW v5;
DROP TABLE t1;

-- ============================================================
-- 测试6: 多表视图 - 无字段列表插入（应失败）
-- ============================================================
CREATE TABLE t1(id INT, age INT);
CREATE TABLE t2(id INT, name CHAR(10));
INSERT INTO t1 VALUES(1, 20);
INSERT INTO t2 VALUES(1, 'Alice');

CREATE VIEW v6 AS SELECT t1.id AS id, t1.age AS age, t2.name AS name FROM t1, t2 WHERE t1.id=t2.id;

-- 查询成功
SELECT * FROM v6;

-- 失败：多表视图不允许无字段列表插入
INSERT INTO v6 VALUES(2, 25, 'Bob');

-- 失败：多表视图不允许删除
DELETE FROM v6 WHERE id=1;

DROP VIEW v6;
DROP TABLE t1;
DROP TABLE t2;

-- ============================================================
-- 测试7: 多表视图 - 单基表字段插入/更新（应成功）
-- ============================================================
CREATE TABLE t1(id INT, age INT);
CREATE TABLE t2(id INT, name CHAR(10));
INSERT INTO t1 VALUES(1, 20);
INSERT INTO t2 VALUES(1, 'Alice');

CREATE VIEW v7 AS SELECT t1.id AS id1, t1.age AS age, t2.id AS id2, t2.name AS name FROM t1, t2 WHERE t1.id=t2.id;

-- 成功：只插入 t1 的字段
INSERT INTO v7(id1, age) VALUES(2, 25);
SELECT * FROM t1;

-- 成功：只插入 t2 的字段
INSERT INTO v7(id2, name) VALUES(3, 'Bob');
SELECT * FROM t2;

-- 成功：只更新 t1 的字段
UPDATE v7 SET age=21 WHERE id1=1;
SELECT * FROM t1;

-- 成功：只更新 t2 的字段
UPDATE v7 SET name='Alice2' WHERE id2=1;
SELECT * FROM t2;

DROP VIEW v7;
DROP TABLE t1;
DROP TABLE t2;

-- ============================================================
-- 测试8: 多表视图 - 跨表字段插入/更新（应失败）
-- ============================================================
CREATE TABLE t1(id INT, age INT);
CREATE TABLE t2(id INT, name CHAR(10));
INSERT INTO t1 VALUES(1, 20);
INSERT INTO t2 VALUES(1, 'Alice');

CREATE VIEW v8 AS SELECT t1.id AS id1, t1.age AS age, t2.id AS id2, t2.name AS name FROM t1, t2 WHERE t1.id=t2.id;

-- 失败：字段来自 t1 和 t2
INSERT INTO v8(id1, name) VALUES(2, 'Bob');

-- 失败：字段来自 t1 和 t2
INSERT INTO v8(age, name) VALUES(25, 'Charlie');

-- 失败：更新多个表的字段
UPDATE v8 SET age=22, name='Alice2' WHERE id1=1;

DROP VIEW v8;
DROP TABLE t1;
DROP TABLE t2;

-- ============================================================
-- 测试9: 视图中的 DISTINCT（只读）
-- ============================================================
CREATE TABLE t1(id INT, category CHAR(10));
INSERT INTO t1 VALUES(1, 'A');
INSERT INTO t1 VALUES(2, 'A');
INSERT INTO t1 VALUES(3, 'B');

CREATE VIEW v9 AS SELECT DISTINCT category FROM t1;

-- 查询成功
SELECT * FROM v9;

-- 失败：DISTINCT 视图只读
INSERT INTO v9 VALUES('C');
UPDATE v9 SET category='D' WHERE category='A';
DELETE FROM v9 WHERE category='A';

DROP VIEW v9;
DROP TABLE t1;

-- ============================================================
-- 测试10: 视图包含子查询（只读）
-- ============================================================
CREATE TABLE t1(id INT, score INT);
CREATE TABLE t2(id INT, max_score INT);
INSERT INTO t1 VALUES(1, 90);
INSERT INTO t2 VALUES(1, 100);

CREATE VIEW v10 AS SELECT id, score FROM t1 WHERE score < (SELECT max_score FROM t2 WHERE t2.id=t1.id);

-- 查询成功
SELECT * FROM v10;

-- 失败：包含子查询的视图通常只读
INSERT INTO v10 VALUES(2, 85);
UPDATE v10 SET score=95 WHERE id=1;
DELETE FROM v10 WHERE id=1;

DROP VIEW v10;
DROP TABLE t1;
DROP TABLE t2;

-- ============================================================
-- 测试11: 嵌套视图
-- ============================================================
CREATE TABLE t1(id INT, age INT, name CHAR(10));
INSERT INTO t1 VALUES(1, 20, 'Alice');
INSERT INTO t1 VALUES(2, 25, 'Bob');

-- 第一层视图
CREATE VIEW v11_base AS SELECT id, age FROM t1;

-- 第二层视图（基于视图）
CREATE VIEW v11 AS SELECT id, age+10 AS age_plus FROM v11_base;

-- 查询成功
SELECT * FROM v11;

-- 成功：更新基础视图的基础字段
UPDATE v11_base SET age=21 WHERE id=1;
SELECT * FROM v11;

-- 失败：更新表达式字段
UPDATE v11 SET age_plus=35 WHERE id=1;

-- 成功：从基础视图插入
INSERT INTO v11_base VALUES(3, 30);
SELECT * FROM v11;

-- 失败：向表达式视图插入
INSERT INTO v11 VALUES(4, 45);

DROP VIEW v11;
DROP VIEW v11_base;
DROP TABLE t1;

-- ============================================================
-- 测试12: WITH CHECK OPTION（如果实现）
-- ============================================================
CREATE TABLE t1(id INT, age INT);
INSERT INTO t1 VALUES(1, 20);
INSERT INTO t1 VALUES(2, 25);

-- 创建带条件的视图
CREATE VIEW v12 AS SELECT * FROM t1 WHERE age > 18;

-- 成功：满足条件
INSERT INTO v12 VALUES(3, 30);
SELECT * FROM v12;

-- 成功：更新后仍满足条件
UPDATE v12 SET age=21 WHERE id=1;
SELECT * FROM v12;

DROP VIEW v12;
DROP TABLE t1;

-- ============================================================
-- 测试13: NULL 值处理
-- ============================================================
CREATE TABLE t1(id INT, age INT, name CHAR(10));
CREATE VIEW v13 AS SELECT id, age FROM t1;

-- 插入包含 NULL 的记录
INSERT INTO v13 VALUES(1, NULL);
SELECT * FROM t1;

-- 更新为 NULL
INSERT INTO v13 VALUES(2, 20);
UPDATE v13 SET age=NULL WHERE id=2;
SELECT * FROM t1;

DROP VIEW v13;
DROP TABLE t1;

-- ============================================================
-- 测试14: 复杂 JOIN 视图
-- ============================================================
CREATE TABLE t1(id INT, t1_value INT);
CREATE TABLE t2(id INT, t2_value INT);
CREATE TABLE t3(id INT, t3_value INT);
INSERT INTO t1 VALUES(1, 100);
INSERT INTO t2 VALUES(1, 200);
INSERT INTO t3 VALUES(1, 300);

CREATE VIEW v14 AS SELECT t1.id AS id, t1.t1_value AS v1, t2.t2_value AS v2, t3.t3_value AS v3 
FROM t1, t2, t3 WHERE t1.id=t2.id AND t2.id=t3.id;

-- 查询成功
SELECT * FROM v14;

-- 成功：只更新一个表的字段
UPDATE v14 SET v1=110 WHERE id=1;
SELECT * FROM t1;

-- 失败：更新多个表的字段
UPDATE v14 SET v1=120, v2=220 WHERE id=1;

-- 失败：删除
DELETE FROM v14 WHERE id=1;

DROP VIEW v14;
DROP TABLE t1;
DROP TABLE t2;
DROP TABLE t3;

-- ============================================================
-- 测试15: 算术表达式视图
-- ============================================================
CREATE TABLE t1(id INT, price INT, quantity INT);
INSERT INTO t1 VALUES(1, 10, 5);
INSERT INTO t1 VALUES(2, 20, 3);

CREATE VIEW v15 AS SELECT id, price, quantity, price*quantity AS total FROM t1;

-- 查询成功
SELECT * FROM v15;

-- 成功：插入基础字段
INSERT INTO v15(id, price, quantity) VALUES(3, 15, 4);
SELECT * FROM v15;

-- 失败：插入计算字段
INSERT INTO v15 VALUES(4, 25, 2, 50);

-- 成功：更新基础字段
UPDATE v15 SET price=11 WHERE id=1;
SELECT * FROM v15;

-- 失败：更新计算字段
UPDATE v15 SET total=100 WHERE id=1;

DROP VIEW v15;
DROP TABLE t1;

-- ============================================================
-- 清理所有测试数据
-- ============================================================
DROP VIEW IF EXISTS v1;
DROP VIEW IF EXISTS v2;
DROP VIEW IF EXISTS v3;
DROP VIEW IF EXISTS v4;
DROP VIEW IF EXISTS v5;
DROP VIEW IF EXISTS v6;
DROP VIEW IF EXISTS v7;
DROP VIEW IF EXISTS v8;
DROP VIEW IF EXISTS v9;
DROP VIEW IF EXISTS v10;
DROP VIEW IF EXISTS v11;
DROP VIEW IF EXISTS v11_base;
DROP VIEW IF EXISTS v12;
DROP VIEW IF EXISTS v13;
DROP VIEW IF EXISTS v14;
DROP VIEW IF EXISTS v15;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
DROP TABLE IF EXISTS t3;

-- ============================================================
-- 测试总结
-- ============================================================
-- 1. 单表视图（完整列/部分列）：所有 DML 操作应成功
-- 2. 表达式视图：基础字段可更新，表达式字段不可更新
-- 3. 聚合视图：完全只读（COUNT, SUM, AVG等）
-- 4. GROUP BY 视图：只读
-- 5. DISTINCT 视图：只读
-- 6. 子查询视图：只读
-- 7. 多表视图无字段列表：不允许插入
-- 8. 多表视图单基表字段：允许插入/更新
-- 9. 多表视图跨表字段：不允许插入/更新
-- 10. 多表视图：不允许删除
-- 11. 嵌套视图：遵循基础视图的规则
-- 12. NULL 值：正确处理
-- 13. 复杂表达式：只有基础字段可更新
-- ============================================================

