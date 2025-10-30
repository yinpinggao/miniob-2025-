-- ============================================================
-- 测试脚本：验证视图插入和查询问题
-- ============================================================

-- 清理环境
DROP TABLE IF EXISTS create_view_t1;
DROP TABLE IF EXISTS create_view_t2;
DROP VIEW IF EXISTS create_view_v5;
DROP VIEW IF EXISTS create_view_v6;
DROP VIEW IF EXISTS create_view_v9;

-- ============================================================
-- 问题1: 单表视图包含表达式字段时的插入测试
-- ============================================================
ECHO "=== 测试问题1: 单表视图包含表达式字段的插入 ===";

-- 初始化数据
CREATE TABLE create_view_t1(id INT, age INT, name CHAR(10));
INSERT INTO create_view_t1 VALUES(1, 10, 'Alice');
INSERT INTO create_view_t1 VALUES(2, 20, 'Bob');
INSERT INTO create_view_t1 VALUES(3, 30, 'Charlie');

-- 创建包含表达式字段的视图
CREATE VIEW create_view_v5 AS SELECT id, age, id+age AS data FROM create_view_t1;

-- 查看视图内容
SELECT * FROM create_view_v5;

-- 测试1.1: 插入指定字段（只包含非表达式字段）
-- 期望：FAILURE（因为视图包含表达式字段data，即使不插入该字段也应该失败）
ECHO "测试1.1: INSERT INTO create_view_v5 (id, age) VALUES(100, 200) - 期望 FAILURE";
INSERT INTO create_view_v5 (id, age) VALUES(100, 200);

-- 测试1.2: 插入所有字段
-- 期望：FAILURE（因为视图包含表达式字段）
ECHO "测试1.2: INSERT INTO create_view_v5 VALUES(101, 201, 302) - 期望 FAILURE";
INSERT INTO create_view_v5 VALUES(101, 201, 302);

-- ============================================================
-- 问题2: 视图引用另一个包含表达式字段的视图
-- ============================================================
ECHO "=== 测试问题2: 视图链式引用和查询 ===";

-- 初始化第二个表
CREATE TABLE create_view_t2(id INT NOT NULL, age INT, name CHAR(10));
INSERT INTO create_view_t2 VALUES(1, 15, 'X');
INSERT INTO create_view_t2 VALUES(2, 25, 'Y');
INSERT INTO create_view_t2 VALUES(3, 35, 'Z');

-- 创建视图v6（包含表达式字段和子查询）
CREATE VIEW create_view_v6 AS 
  SELECT id, age, id+age AS data 
  FROM create_view_t1 
  WHERE id IN (SELECT id FROM create_view_t2);

-- 测试v6
ECHO "测试2.1: 查询视图v6";
SELECT * FROM create_view_v6;
SELECT COUNT(*) FROM create_view_v6;
SELECT COUNT(id) FROM create_view_v6;
SELECT SUM(data) FROM create_view_v6;

-- 创建视图v9（基于v6和t1的连接视图）
ECHO "测试2.2: 创建视图v9（基于v6）";
CREATE VIEW create_view_v9 AS 
  SELECT t2.name, t1.data 
  FROM create_view_v6 t1, create_view_t1 t2 
  WHERE t1.id = t2.id;

-- 测试v9的查询
ECHO "测试2.3: 查询视图v9";
SELECT * FROM create_view_v9;

ECHO "测试2.4: SELECT COUNT(*) FROM create_view_v9 - 期望返回数字";
SELECT COUNT(*) FROM create_view_v9;

ECHO "测试2.5: SELECT COUNT(data) FROM create_view_v9 - 期望返回数字（如225），实际可能返回FAILURE";
SELECT COUNT(data) FROM create_view_v9;

-- ============================================================
-- 总结
-- ============================================================
ECHO "=== 测试完成 ===";
ECHO "问题1: 如果测试1.1和1.2都返回FAILURE，则问题已解决";
ECHO "问题2: 如果测试2.5返回数字而不是FAILURE，则问题已解决";


