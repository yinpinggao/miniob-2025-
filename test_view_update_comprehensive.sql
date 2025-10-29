-- ============================================================
-- 视图更新规则完整测试用例
-- ============================================================

-- ============================================================
-- 准备测试数据
-- ============================================================
DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;
DROP TABLE IF EXISTS single_table_t1;

CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));
CREATE TABLE create_view_t2(id INT, name CHAR(10));
CREATE TABLE single_table_t1(id INT, age INT, score INT);

INSERT INTO create_view_t1 VALUES(1, 20, 'Alice');
INSERT INTO create_view_t1 VALUES(2, 25, 'Bob');
INSERT INTO create_view_t2 VALUES(1, 'TestA');
INSERT INTO create_view_t2 VALUES(2, 'TestB');
INSERT INTO single_table_t1 VALUES(1, 20, 90);
INSERT INTO single_table_t1 VALUES(2, 25, 85);

-- ============================================================
-- 测试1: 多表视图 - 无字段列表插入（应该失败）
-- ============================================================
DROP VIEW IF EXISTS create_view_v4;
CREATE VIEW create_view_v4 AS 
  SELECT t1.id AS id, t1.age AS age, t2.name AS name 
  FROM create_view_t1 t1, create_view_t2 t2 
  WHERE t1.id=t2.id;

-- 测试1.1: 无字段列表，插入所有字段（期望 FAILURE）
-- 原因：多表视图不允许不指定字段列表的插入
INSERT INTO create_view_v4 VALUES(120, 120, 'JFDGYF89DA');

-- 测试1.2: 指定字段列表，所有字段来自同一个基表 t1（期望 SUCCESS）
-- 原因：id 和 age 都来自 create_view_t1
INSERT INTO create_view_v4(id, age) VALUES(175, 175);

-- 验证插入结果
SELECT * FROM create_view_t1 WHERE id=175;

-- 测试1.3: 指定字段列表，所有字段来自同一个基表 t2（期望 SUCCESS）
-- 原因：name 来自 create_view_t2
INSERT INTO create_view_v4(name) VALUES('NewName');

-- 测试1.4: 指定字段列表，字段来自多个基表（期望 FAILURE）
-- 原因：id 来自 t1，name 来自 t2
INSERT INTO create_view_v4(id, name) VALUES(200, 'MultiTable');

-- 测试1.5: 指定字段列表，字段来自多个基表（期望 FAILURE）
-- 原因：id, age 来自 t1，name 来自 t2
INSERT INTO create_view_v4(id, age, name) VALUES(300, 30, 'AllFields');

-- ============================================================
-- 测试2: 单表视图（完整列）
-- ============================================================
DROP VIEW IF EXISTS single_view_v1;
CREATE VIEW single_view_v1 AS SELECT * FROM single_table_t1;

-- 测试2.1: 无字段列表插入（期望 SUCCESS）
INSERT INTO single_view_v1 VALUES(10, 30, 95);
SELECT * FROM single_table_t1 WHERE id=10;

-- 测试2.2: 指定部分字段插入（期望 SUCCESS）
INSERT INTO single_view_v1(id, age) VALUES(11, 31);
SELECT * FROM single_table_t1 WHERE id=11;

-- ============================================================
-- 测试3: 单表视图（部分列）
-- ============================================================
DROP VIEW IF EXISTS single_view_v2;
CREATE VIEW single_view_v2(id, age) AS SELECT id, age FROM single_table_t1;

-- 测试3.1: 无字段列表插入（期望 SUCCESS）
-- score 字段会被设置为 NULL
INSERT INTO single_view_v2 VALUES(20, 40);
SELECT * FROM single_table_t1 WHERE id=20;

-- 测试3.2: 指定字段列表插入（期望 SUCCESS）
INSERT INTO single_view_v2(id, age) VALUES(21, 41);
SELECT * FROM single_table_t1 WHERE id=21;

-- ============================================================
-- 测试4: 单表视图（含表达式）
-- ============================================================
DROP VIEW IF EXISTS expr_view_v1;
CREATE VIEW expr_view_v1 AS SELECT id, age, age+score AS total FROM single_table_t1;

-- 测试4.1: 无字段列表插入（期望 FAILURE）
-- 原因：视图包含表达式字段 total
INSERT INTO expr_view_v1 VALUES(30, 50, 140);

-- 测试4.2: 指定基础列插入（期望 SUCCESS）
-- 原因：只插入基础列 id 和 age，不涉及表达式列
INSERT INTO expr_view_v1(id, age) VALUES(31, 51);
SELECT * FROM single_table_t1 WHERE id=31;

-- 测试4.3: 尝试插入表达式列（期望 FAILURE）
-- 原因：total 是表达式字段，不可插入
INSERT INTO expr_view_v1(id, total) VALUES(32, 100);

-- ============================================================
-- 测试5: UPDATE 操作 - 多表视图
-- ============================================================
DROP VIEW IF EXISTS update_view_v1;
CREATE VIEW update_view_v1 AS 
  SELECT t1.id AS id1, t1.age AS age, t2.id AS id2, t2.name AS name 
  FROM create_view_t1 t1, create_view_t2 t2 
  WHERE t1.id=t2.id;

-- 测试5.1: 更新单个基表的字段（期望 SUCCESS）
-- 原因：只更新 age，它来自 t1
UPDATE update_view_v1 SET age=99 WHERE id1=1;
SELECT * FROM create_view_t1 WHERE id=1;

-- 测试5.2: 更新来自不同基表的字段（期望 FAILURE）
-- 原因：age 来自 t1，name 来自 t2
UPDATE update_view_v1 SET age=88, name='NewName' WHERE id1=1;

-- 测试5.3: 更新单个基表的多个字段（期望 SUCCESS）
-- 原因：id1 和 age 都来自 t1
UPDATE update_view_v1 SET id1=100, age=100 WHERE id1=2;

-- ============================================================
-- 测试6: UPDATE 操作 - 表达式视图
-- ============================================================
DROP VIEW IF EXISTS update_expr_view;
CREATE VIEW update_expr_view AS SELECT id, age, age*2 AS double_age FROM single_table_t1;

-- 测试6.1: 更新基础列（期望 SUCCESS）
UPDATE update_expr_view SET age=35 WHERE id=1;
SELECT * FROM single_table_t1 WHERE id=1;

-- 测试6.2: 更新表达式列（期望 FAILURE）
-- 原因：double_age 是表达式列，不可更新
UPDATE update_expr_view SET double_age=100 WHERE id=1;

-- 测试6.3: 同时更新基础列和表达式列（期望 FAILURE）
-- 原因：包含表达式列 double_age
UPDATE update_expr_view SET age=40, double_age=80 WHERE id=1;

-- ============================================================
-- 测试7: DELETE 操作 - 多表视图
-- ============================================================
DROP VIEW IF EXISTS delete_view_v1;
CREATE VIEW delete_view_v1 AS 
  SELECT t1.id AS id, t1.age AS age, t2.name AS name 
  FROM create_view_t1 t1, create_view_t2 t2 
  WHERE t1.id=t2.id;

-- 测试7.1: 删除多表视图的记录（期望 FAILURE）
-- 原因：多表视图不允许删除操作
DELETE FROM delete_view_v1 WHERE id=1;

-- ============================================================
-- 测试8: DELETE 操作 - 单表视图
-- ============================================================
DROP VIEW IF EXISTS delete_view_v2;
CREATE VIEW delete_view_v2 AS SELECT id, age FROM single_table_t1;

-- 测试8.1: 删除单表视图的记录（期望 SUCCESS）
DELETE FROM delete_view_v2 WHERE id=10;
SELECT * FROM single_table_t1 WHERE id=10;

-- 测试8.2: 删除表达式视图的记录（期望 SUCCESS）
-- 原因：即使视图包含表达式，删除操作也是允许的
DROP VIEW IF EXISTS delete_expr_view;
CREATE VIEW delete_expr_view AS SELECT id, age, age*2 AS double_age FROM single_table_t1;
DELETE FROM delete_expr_view WHERE id=11;
SELECT * FROM single_table_t1 WHERE id=11;

-- ============================================================
-- 测试9: 聚合视图（所有操作都应该失败）
-- ============================================================
DROP VIEW IF EXISTS agg_view;
CREATE VIEW agg_view AS SELECT COUNT(*) AS cnt FROM single_table_t1;

-- 测试9.1: 插入聚合视图（期望 FAILURE）
INSERT INTO agg_view VALUES(100);

-- 测试9.2: 更新聚合视图（期望 FAILURE）
UPDATE agg_view SET cnt=200;

-- 测试9.3: 删除聚合视图（期望 FAILURE）
DELETE FROM agg_view;

-- ============================================================
-- 清理测试数据
-- ============================================================
DROP VIEW IF EXISTS create_view_v4;
DROP VIEW IF EXISTS single_view_v1;
DROP VIEW IF EXISTS single_view_v2;
DROP VIEW IF EXISTS expr_view_v1;
DROP VIEW IF EXISTS update_view_v1;
DROP VIEW IF EXISTS update_expr_view;
DROP VIEW IF EXISTS delete_view_v1;
DROP VIEW IF EXISTS delete_view_v2;
DROP VIEW IF EXISTS delete_expr_view;
DROP VIEW IF EXISTS agg_view;

DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;
DROP TABLE IF EXISTS single_table_t1;










