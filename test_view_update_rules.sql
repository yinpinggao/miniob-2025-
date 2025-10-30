-- 测试视图更新规则
-- 执行方式: ./bin/obclient < test_view_update_rules.sql

-- ============================================================
-- 测试用例 1: 单表视图（完整列）
-- ============================================================
DROP TABLE IF EXISTS t1;
CREATE TABLE t1(id INT, name VARCHAR(50), age INT);

DROP VIEW IF EXISTS v1;
CREATE VIEW v1 AS SELECT * FROM t1;

-- 应该成功
INSERT INTO v1 VALUES(1, 'Alice', 20);
SELECT * FROM v1;

-- 应该成功
UPDATE v1 SET name='Bob' WHERE id=1;
SELECT * FROM v1;

-- 应该成功
DELETE FROM v1 WHERE id=1;
SELECT * FROM v1;

-- ============================================================
-- 测试用例 2: 单表视图（部分列）
-- ============================================================
DROP TABLE IF EXISTS t2;
CREATE TABLE t2(id INT, name VARCHAR(50), age INT);
INSERT INTO t2 VALUES(2, 'Charlie', 25);

DROP VIEW IF EXISTS v2;
CREATE VIEW v2(id, name) AS SELECT id, name FROM t2;

-- 应该成功（只插入指定列）
INSERT INTO v2(id, name) VALUES(3, 'David');
SELECT * FROM t2;

-- 应该成功（age 设为 NULL）
INSERT INTO v2 VALUES(4, 'Eve');
SELECT * FROM t2;

-- 应该成功
UPDATE v2 SET name='Frank' WHERE id=3;
SELECT * FROM t2;

-- 应该成功
DELETE FROM v2 WHERE id=3;
SELECT * FROM t2;

-- ============================================================
-- 测试用例 3: 单表视图（含表达式）
-- ============================================================
DROP TABLE IF EXISTS t3;
CREATE TABLE t3(id INT, age INT);
INSERT INTO t3 VALUES(1, 20);

DROP VIEW IF EXISTS v3;
CREATE VIEW v3 AS SELECT id, id+age AS data FROM t3;

-- 应该失败（含表达式列）
INSERT INTO v3 VALUES(2, 30);

-- 应该成功（更新基础列）
UPDATE v3 SET id=2 WHERE id=1;
SELECT * FROM v3;

-- 应该失败（更新表达式列）
UPDATE v3 SET data=30 WHERE id=2;

-- 应该成功
DELETE FROM v3 WHERE id=2;
SELECT * FROM t3;

-- ============================================================
-- 测试用例 4: 多表视图（无字段列表）
-- ============================================================
DROP TABLE IF EXISTS t4;
DROP TABLE IF EXISTS t5;
CREATE TABLE t4(id INT, name VARCHAR(50));
CREATE TABLE t5(id INT, age INT);
INSERT INTO t4 VALUES(1, 'Alice');
INSERT INTO t5 VALUES(1, 20);

DROP VIEW IF EXISTS v4;
CREATE VIEW v4 AS SELECT t4.id, t4.name, t5.age FROM t4, t5 WHERE t4.id=t5.id;

-- 应该失败（join 视图无字段列表）
INSERT INTO v4 VALUES(2, 'Bob', 25);

-- 应该失败（join 视图）
DELETE FROM v4 WHERE id=1;

-- ============================================================
-- 测试用例 5: 多表视图（单基表字段）
-- ============================================================
DROP TABLE IF EXISTS t6;
DROP TABLE IF EXISTS t7;
CREATE TABLE t6(id INT, name VARCHAR(50));
CREATE TABLE t7(id INT, age INT);
INSERT INTO t6 VALUES(1, 'Alice');
INSERT INTO t7 VALUES(1, 20);

DROP VIEW IF EXISTS v5;
CREATE VIEW v5 AS SELECT t6.id AS id6, t6.name, t7.id AS id7, t7.age FROM t6, t7 WHERE t6.id=t7.id;

-- 应该成功（只涉及 t6）
INSERT INTO v5(id6, name) VALUES(2, 'Bob');
SELECT * FROM t6;

-- 应该成功（只更新 t6）
UPDATE v5 SET name='Charlie' WHERE id6=1;
SELECT * FROM t6;

-- ============================================================
-- 测试用例 6: 多表视图（多基表字段）
-- ============================================================
DROP TABLE IF EXISTS t8;
DROP TABLE IF EXISTS t9;
CREATE TABLE t8(id INT, name VARCHAR(50));
CREATE TABLE t9(id INT, age INT);
INSERT INTO t8 VALUES(1, 'Alice');
INSERT INTO t9 VALUES(1, 20);

DROP VIEW IF EXISTS v6;
CREATE VIEW v6 AS SELECT t8.id AS id8, t8.name, t9.id AS id9, t9.age FROM t8, t9 WHERE t8.id=t9.id;

-- 应该失败（涉及 t8 和 t9）
INSERT INTO v6(id8, name, age) VALUES(2, 'Bob', 25);

-- 应该失败（涉及 t8 和 t9）
UPDATE v6 SET name='Bob', age=21 WHERE id8=1;

-- ============================================================
-- 测试用例 7: 聚合视图
-- ============================================================
DROP TABLE IF EXISTS t10;
CREATE TABLE t10(id INT, score INT);
INSERT INTO t10 VALUES(1, 90);
INSERT INTO t10 VALUES(2, 85);

DROP VIEW IF EXISTS v7;
CREATE VIEW v7 AS SELECT COUNT(*) AS cnt FROM t10;

-- 应该失败（聚合视图）
INSERT INTO v7 VALUES(10);

-- 应该失败（聚合视图）
UPDATE v7 SET cnt=10;

-- 应该失败（聚合视图）
DELETE FROM v7;

-- ============================================================
-- 清理
-- ============================================================
DROP VIEW IF EXISTS v1;
DROP VIEW IF EXISTS v2;
DROP VIEW IF EXISTS v3;
DROP VIEW IF EXISTS v4;
DROP VIEW IF EXISTS v5;
DROP VIEW IF EXISTS v6;
DROP VIEW IF EXISTS v7;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
DROP TABLE IF EXISTS t3;
DROP TABLE IF EXISTS t4;
DROP TABLE IF EXISTS t5;
DROP TABLE IF EXISTS t6;
DROP TABLE IF EXISTS t7;
DROP TABLE IF EXISTS t8;
DROP TABLE IF EXISTS t9;
DROP TABLE IF EXISTS t10;











