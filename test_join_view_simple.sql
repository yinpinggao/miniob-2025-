-- 简单的多表视图测试
DROP VIEW IF EXISTS v_test;
DROP TABLE IF EXISTS t1_test;
DROP TABLE IF EXISTS t2_test;

CREATE TABLE t1_test(id INT, age INT);
CREATE TABLE t2_test(id INT, name CHAR(10));

INSERT INTO t1_test VALUES(1, 20);
INSERT INTO t2_test VALUES(1, 'Alice');

CREATE VIEW v_test AS SELECT t1_test.id AS id1, t1_test.age AS age, t2_test.id AS id2, t2_test.name AS name 
FROM t1_test, t2_test WHERE t1_test.id=t2_test.id;

-- 查询视图
SELECT * FROM v_test;

-- 测试：只插入 t1 的字段（应该成功）
INSERT INTO v_test(id1, age) VALUES(2, 25);

-- 验证
SELECT * FROM t1_test;
SELECT * FROM t2_test;
SELECT * FROM v_test;

-- 清理
DROP VIEW v_test;
DROP TABLE t1_test;
DROP TABLE t2_test;

