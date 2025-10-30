-- 测试v9字段访问
DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;
DROP VIEW IF EXISTS create_view_v6;
DROP VIEW IF EXISTS create_view_v9;

CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));
CREATE TABLE create_view_t2(id INT NOT NULL, age INT, name CHAR(10));

INSERT INTO create_view_t1 VALUES(1, 10, 'Alice');
INSERT INTO create_view_t1 VALUES(2, 20, 'Bob');
INSERT INTO create_view_t1 VALUES(3, 30, 'Charlie');

INSERT INTO create_view_t2 VALUES(1, 15, 'X');
INSERT INTO create_view_t2 VALUES(2, 25, 'Y');
INSERT INTO create_view_t2 VALUES(3, 35, 'Z');

CREATE VIEW create_view_v6 AS 
  SELECT id, age, id+age AS data 
  FROM create_view_t1 
  WHERE id IN (SELECT id FROM create_view_t2);

CREATE VIEW create_view_v9 AS 
  SELECT t2.name, t1.data 
  FROM create_view_v6 t1, create_view_t1 t2 
  WHERE t1.id = t2.id;

-- 测试不同的字段访问方式
SELECT * FROM create_view_v9;

-- 尝试访问 name 字段
SELECT name FROM create_view_v9;
SELECT COUNT(name) FROM create_view_v9;

-- 尝试访问 data 字段（这个失败）
SELECT data FROM create_view_v9;
SELECT COUNT(data) FROM create_view_v9;

-- 尝试使用带表名的方式
SELECT t2.name FROM create_view_v9;
SELECT t1.data FROM create_view_v9;


