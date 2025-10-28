-- 测试表达式视图的插入
DROP TABLE IF EXISTS create_view_t1;
DROP VIEW IF EXISTS create_view_v5;

CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));

-- 创建包含表达式的视图
CREATE VIEW create_view_v5 AS SELECT id, age, id+age AS data FROM create_view_t1;

-- 这个应该 SUCCESS（只插入基础字段 id 和 age）
INSERT INTO create_view_v5 (id, age) VALUES(1, 2);

-- 验证数据
SELECT * FROM create_view_t1;
SELECT * FROM create_view_v5;

-- 这个应该 FAILURE（插入所有字段，包括表达式字段）
INSERT INTO create_view_v5 VALUES(3, 4, 7);

-- 这个应该 FAILURE（尝试插入表达式字段）
INSERT INTO create_view_v5 (id, data) VALUES(5, 10);

-- 清理
DROP VIEW IF EXISTS create_view_v5;
DROP TABLE IF EXISTS create_view_t1;

