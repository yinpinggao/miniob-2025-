-- ============================================================
-- 问题2测试：视图链式引用查询
-- ============================================================

-- 清理环境
DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;
DROP VIEW IF EXISTS create_view_v6;
DROP VIEW IF EXISTS create_view_v9;

-- 初始化数据
CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));
CREATE TABLE create_view_t2(id INT NOT NULL, age INT, name CHAR(10));

-- 插入测试数据（这里插入少量数据用于测试）
INSERT INTO create_view_t1 VALUES(1, 10, 'Alice');
INSERT INTO create_view_t1 VALUES(2, 20, 'Bob');
INSERT INTO create_view_t1 VALUES(3, 30, 'Charlie');

INSERT INTO create_view_t2 VALUES(1, 15, 'X');
INSERT INTO create_view_t2 VALUES(2, 25, 'Y');
INSERT INTO create_view_t2 VALUES(3, 35, 'Z');

-- 创建视图v6（包含表达式字段和子查询）
CREATE VIEW create_view_v6 AS 
  SELECT id, age, id+age AS data 
  FROM create_view_t1 
  WHERE id IN (SELECT id FROM create_view_t2);

-- 测试v6
SELECT * FROM create_view_v6;
SELECT COUNT(*) FROM create_view_v6;
SELECT COUNT(id) FROM create_view_v6;
SELECT SUM(data) FROM create_view_v6;
SELECT COUNT(name) FROM create_view_v6;

-- 创建视图v9（基于v6和t1的连接视图）
CREATE VIEW create_view_v9 AS 
  SELECT t2.name, t1.data 
  FROM create_view_v6 t1, create_view_t1 t2 
  WHERE t1.id = t2.id;

-- 测试v9
SELECT * FROM create_view_v9;
SELECT COUNT(*) FROM create_view_v9;
SELECT COUNT(data) FROM create_view_v9;


