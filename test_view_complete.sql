-- ============================================================
-- 完整的视图更新功能测试用例
-- 基于 MySQL 视图更新规则和 OceanBase 文档要求
-- ============================================================

-- 清理环境
DROP VIEW IF EXISTS v_single;
DROP VIEW IF EXISTS v_expr;
DROP VIEW IF EXISTS v_agg;
DROP VIEW IF EXISTS v_join;
DROP VIEW IF EXISTS v_partial;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;

-- ============================================================
-- 测试1: 单表视图 - 完整列
-- ============================================================
CREATE TABLE t1(id INT, age INT, name CHAR(10));
CREATE VIEW v_single AS SELECT * FROM t1;

-- 应该成功：插入所有字段
INSERT INTO v_single VALUES(1, 20, 'Alice');
SELECT * FROM t1;

-- 应该成功：更新
UPDATE v_single SET age=21 WHERE id=1;
SELECT * FROM t1;

-- 应该成功：删除
DELETE FROM v_single WHERE id=1;
SELECT * FROM t1;

DROP VIEW v_single;
DROP TABLE t1;

-- ============================================================
-- 测试2: 单表视图 - 部分列
-- ============================================================
CREATE TABLE t1(id INT, age INT, name CHAR(10));
CREATE VIEW v_partial AS SELECT id, age FROM t1;

-- 应该成功：插入指定字段（name 为 NULL）
INSERT INTO v_partial(id, age) VALUES(2, 25);
SELECT * FROM t1;

-- 应该成功：插入所有视图字段
INSERT INTO v_partial VALUES(3, 30);
SELECT * FROM t1;

-- 应该成功：更新
UPDATE v_partial SET age=26 WHERE id=2;
SELECT * FROM t1;

-- 应该成功：删除
DELETE FROM v_partial WHERE id=2;
SELECT * FROM t1;

DROP VIEW v_partial;
DROP TABLE t1;

-- ============================================================
-- 测试3: 表达式视图
-- ============================================================
CREATE TABLE t1(id INT, age INT);
INSERT INTO t1 VALUES(10, 20);

CREATE VIEW v_expr AS SELECT id, age, id+age AS total FROM t1;

-- 应该成功：只插入基础字段
INSERT INTO v_expr(id, age) VALUES(11, 21);
SELECT * FROM t1;
SELECT * FROM v_expr;

-- 应该失败：插入所有字段（包含表达式）
INSERT INTO v_expr VALUES(12, 22, 34);

-- 应该失败：插入表达式字段
INSERT INTO v_expr(id, total) VALUES(13, 50);

-- 应该成功：更新基础字段
UPDATE v_expr SET age=25 WHERE id=10;
SELECT * FROM t1;

-- 应该失败：更新表达式字段
UPDATE v_expr SET total=100 WHERE id=10;

-- 应该成功：删除
DELETE FROM v_expr WHERE id=10;
SELECT * FROM t1;

DROP VIEW v_expr;
DROP TABLE t1;

-- ============================================================
-- 测试4: 聚合视图（只读）
-- ============================================================
CREATE TABLE t1(id INT, score INT);
INSERT INTO t1 VALUES(1, 90);
INSERT INTO t1 VALUES(2, 85);

CREATE VIEW v_agg AS SELECT COUNT(*) AS cnt, AVG(score) AS avg_score FROM t1;

-- 应该失败：所有修改操作都不允许
INSERT INTO v_agg VALUES(10, 80);
UPDATE v_agg SET cnt=10;
DELETE FROM v_agg;

DROP VIEW v_agg;
DROP TABLE t1;

-- ============================================================
-- 测试5: 多表视图 - 单基表字段
-- ============================================================
CREATE TABLE t1(id INT, age INT, name CHAR(10));
CREATE TABLE t2(id INT, salary INT);
INSERT INTO t1 VALUES(1, 20, 'Alice');
INSERT INTO t2 VALUES(1, 5000);

CREATE VIEW v_join AS SELECT t1.id AS id, t1.age AS age, t2.salary AS salary 
FROM t1, t2 WHERE t1.id = t2.id;

-- 应该失败：无字段列表插入
INSERT INTO v_join VALUES(2, 25, 6000);

-- 应该成功：所有字段来自 t1
INSERT INTO v_join(id, age) VALUES(3, 30);
SELECT * FROM t1;

-- 应该成功：更新 t1 的字段
UPDATE v_join SET age=21 WHERE id=1;
SELECT * FROM t1;

-- 应该失败：更新多个表的字段
UPDATE v_join SET age=22, salary=5500 WHERE id=1;

-- 应该失败：删除（多表视图）
DELETE FROM v_join WHERE id=1;

DROP VIEW v_join;
DROP TABLE t1;
DROP TABLE t2;

-- ============================================================
-- 测试6: 多表视图 - 跨表字段
-- ============================================================
CREATE TABLE t1(id INT, name CHAR(10));
CREATE TABLE t2(id INT, age INT);
INSERT INTO t1 VALUES(1, 'Bob');
INSERT INTO t2 VALUES(1, 30);

CREATE VIEW v_join AS SELECT t1.id AS id1, t1.name AS name, t2.id AS id2, t2.age AS age
FROM t1, t2 WHERE t1.id = t2.id;

-- 应该失败：字段来自多个表
INSERT INTO v_join(id1, name, age) VALUES(2, 'Charlie', 35);

-- 应该失败：更新多个表的字段
UPDATE v_join SET name='David', age=40 WHERE id1=1;

DROP VIEW v_join;
DROP TABLE t1;
DROP TABLE t2;

-- ============================================================
-- 清理
-- ============================================================
DROP VIEW IF EXISTS v_single;
DROP VIEW IF EXISTS v_expr;
DROP VIEW IF EXISTS v_agg;
DROP VIEW IF EXISTS v_join;
DROP VIEW IF EXISTS v_partial;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;

