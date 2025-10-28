-- 快速验证修复是否有效
DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;
DROP VIEW IF EXISTS create_view_v4;

-- 使用和测试平台一样的表结构
CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));
CREATE TABLE create_view_t2(id INT, age INT, name CHAR(10));

-- 先插入一些基础数据
INSERT INTO create_view_t1 VALUES(1, 10, 'TableOne');
INSERT INTO create_view_t2 VALUES(1, 20, 'TableTwo');

-- 创建多表视图
CREATE VIEW create_view_v4 AS SELECT t1.id AS id, t1.age AS age, t2.name AS name FROM create_view_t1 t1, create_view_t2 t2 WHERE t1.id=t2.id;

-- 这个应该 SUCCESS（id 和 age 都来自 t1）
INSERT INTO create_view_v4(id, age) VALUES(114, 114);

-- 验证插入是否成功
SELECT * FROM create_view_t1;
SELECT * FROM create_view_t2;
SELECT * FROM create_view_v4;

-- 清理
DROP VIEW IF EXISTS create_view_v4;
DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;

