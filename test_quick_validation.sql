-- ============================================================
-- 快速验证测试 - 对应你提供的测试用例
-- ============================================================

-- 准备数据
DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;

CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));
CREATE TABLE create_view_t2(id INT, name CHAR(10));

INSERT INTO create_view_t1 VALUES(1, 20, 'Alice');
INSERT INTO create_view_t1 VALUES(2, 25, 'Bob');
INSERT INTO create_view_t2 VALUES(1, 'TestA');
INSERT INTO create_view_t2 VALUES(2, 'TestB');

-- 创建多表视图
DROP VIEW IF EXISTS create_view_v4;
CREATE VIEW create_view_v4 AS 
  SELECT t1.id AS id, t1.age AS age, t2.name AS name 
  FROM create_view_t1 t1, create_view_t2 t2 
  WHERE t1.id=t2.id;

-- 查看视图内容
SELECT * FROM create_view_v4;

-- ============================================================
-- 核心测试用例
-- ============================================================

-- 测试1: 不指定字段列表插入（期望 FAILURE）
-- 错误原因：多表视图不允许不指定字段列表的插入
-- 期望错误消息：Can not insert into join view 'xxx.create_view_v4' without fields list
INSERT INTO create_view_v4 VALUES(120, 120, 'JFDGYF89DA');

-- 测试2: 指定字段列表，所有字段来自同一个基表（期望 SUCCESS）
-- id 和 age 都来自 create_view_t1
INSERT INTO create_view_v4(id, age) VALUES(175, 175);

-- 验证插入结果
SELECT * FROM create_view_t1 WHERE id=175;
SELECT * FROM create_view_v4 WHERE id=175;

-- ============================================================
-- 额外测试：其他场景
-- ============================================================

-- 测试3: 指定字段列表，字段来自多个基表（期望 FAILURE）
-- id, age 来自 t1，name 来自 t2
-- 期望错误消息：Can not insert into join view 'xxx.create_view_v4' with fields from multiple tables
INSERT INTO create_view_v4(id, age, name) VALUES(200, 30, 'TestName');

-- 测试4: 只插入来自 t2 的字段（期望 SUCCESS）
-- name 来自 create_view_t2
INSERT INTO create_view_v4(name) VALUES('OnlyT2');

-- 验证结果
SELECT * FROM create_view_t2 WHERE name='OnlyT2';

-- 测试5: 更新单一基表的字段（期望 SUCCESS）
UPDATE create_view_v4 SET age=99 WHERE id=1;
SELECT * FROM create_view_t1 WHERE id=1;

-- 测试6: 更新多个基表的字段（期望 FAILURE）
-- 期望错误消息：Can not update join view with fields from multiple tables
UPDATE create_view_v4 SET age=88, name='NewName' WHERE id=1;

-- 测试7: 删除多表视图的记录（期望 FAILURE）
-- 期望错误消息：Can not delete from join view 'xxx.create_view_v4'
DELETE FROM create_view_v4 WHERE id=2;

-- ============================================================
-- 清理
-- ============================================================
DROP VIEW IF EXISTS create_view_v4;
DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;










