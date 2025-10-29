-- 精确复现测评平台的测试用例
DROP TABLE IF EXISTS create_view_t1;
DROP VIEW IF EXISTS create_view_v5;

-- init data
CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));

-- create view(expression)
CREATE VIEW create_view_v5 AS SELECT id, age, id+age AS data FROM create_view_t1;

-- 这些查询可能会影响视图状态
SELECT COUNT(*) FROM create_view_v5;
SELECT COUNT(id) FROM create_view_v5;
SELECT SUM(age) FROM create_view_v5;
SELECT SUM(data) FROM create_view_v5;

-- 关键测试：这个应该成功
INSERT INTO create_view_v5 (id, age) VALUES(1, 2);

-- 验证
SELECT * FROM create_view_t1;
SELECT * FROM create_view_v5;

-- 清理
DROP VIEW create_view_v5;
DROP TABLE create_view_t1;

