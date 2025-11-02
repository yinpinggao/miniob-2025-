-- ===================================================================
-- 快速BM25测试 - 用于验证基本功能
-- ===================================================================

-- 清理
DROP TABLE IF EXISTS quick_test;

-- 创建表
CREATE TABLE quick_test (id INT, content TEXT);

-- 插入简单数据
INSERT INTO quick_test VALUES (1, '数据库系统');
INSERT INTO quick_test VALUES (2, '数据库系统设计');
INSERT INTO quick_test VALUES (3, '计算机网络');

-- 创建全文索引
ALTER TABLE quick_test ADD FULLTEXT INDEX idx_quick (content) WITH PARSER jieba;

-- 测试1: 基本查询
SELECT '=== 测试1: 基本MATCH查询 ===' AS test;
SELECT id, content, MATCH(content) AGAINST('数据库') AS score 
FROM quick_test 
WHERE MATCH(content) AGAINST('数据库') > 0
ORDER BY score DESC, id;

-- 预期: 文档1和2返回，文档3不返回（不包含"数据库"）
-- 文档1和2的分数应该 > 0

-- 测试2: 查看所有分数
SELECT '=== 测试2: 所有文档的BM25分数 ===' AS test;
SELECT id, content, MATCH(content) AGAINST('数据库') AS score 
FROM quick_test 
ORDER BY score DESC, id;

-- 预期: 文档1,2有正分数，文档3为0

-- 测试3: 验证分数不是固定值
SELECT '=== 测试3: 验证分数随数据变化 ===' AS test;
INSERT INTO quick_test VALUES (4, '数据库管理系统数据库技术数据库应用');
-- 重建索引（在有些实现中可能需要）
DROP INDEX idx_quick ON quick_test;
ALTER TABLE quick_test ADD FULLTEXT INDEX idx_quick (content) WITH PARSER jieba;

SELECT id, content, MATCH(content) AGAINST('数据库') AS score 
FROM quick_test 
WHERE MATCH(content) AGAINST('数据库') > 0
ORDER BY score DESC, id;

-- 预期: 加入文档4后，所有文档的分数可能会变化（因为avgdl和df变了）
-- 如果分数没变，说明没有使用真正的全文索引！

-- 测试4: 检查IDF是否动态计算
SELECT '=== 测试4: IDF动态计算验证 ===' AS test;
-- 添加更多包含"数据库"的文档，降低其IDF
INSERT INTO quick_test VALUES (5, '数据库入门');
INSERT INTO quick_test VALUES (6, '数据库优化');
INSERT INTO quick_test VALUES (7, '数据库设计原则');

DROP INDEX idx_quick ON quick_test;
ALTER TABLE quick_test ADD FULLTEXT INDEX idx_quick (content) WITH PARSER jieba;

SELECT id, MATCH(content) AGAINST('数据库') AS score_after 
FROM quick_test 
WHERE id IN (1, 2)
ORDER BY id;

-- 预期: 相比测试1，文档1和2的分数应该变化
-- 因为"数据库"现在更常见了，IDF应该降低

-- 清理
DROP TABLE quick_test;

SELECT '=== 快速测试完成！===' AS result;
SELECT '如果所有分数都是固定值（如0.69），说明还在使用静态IDF！' AS warning;
SELECT '如果分数随着数据变化而变化，说明动态BM25计算正常！' AS success;

