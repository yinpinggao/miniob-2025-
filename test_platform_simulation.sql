-- 模拟测评平台的测试顺序
DROP TABLE IF EXISTS create_view_t1;
DROP VIEW IF EXISTS create_view_v5;

-- 创建表
CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));

-- 插入一些初始数据（测评平台可能会这样做）
INSERT INTO create_view_t1 VALUES(10, 20, 'InitData');
INSERT INTO create_view_t1 VALUES(11, 21, 'Test');

-- 创建视图
CREATE VIEW create_view_v5 AS SELECT id, age, id+age AS data FROM create_view_t1;

-- 执行一些查询（测评平台会这样做）
SELECT COUNT(*) FROM create_view_v5;
SELECT COUNT(id) FROM create_view_v5;
SELECT SUM(age) FROM create_view_v5;
SELECT SUM(data) FROM create_view_v5;

-- 现在尝试插入（这里应该成功）
INSERT INTO create_view_v5 (id, age) VALUES(1, 2);

-- 验证
SELECT * FROM create_view_t1;
SELECT * FROM create_view_v5;

-- 清理
DROP VIEW IF EXISTS create_view_v5;
DROP TABLE IF EXISTS create_view_t1;

