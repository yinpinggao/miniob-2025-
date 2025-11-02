-- 全文索引功能测试用例

-- ============================================
-- 测试1: TOKENIZE 函数测试
-- ============================================

-- 1.1 基本TOKENIZE测试
SELECT TOKENIZE('information_schema.SCHEMATA表的主要功能是什么？', 'jieba') as text_tokens;

-- 1.2 简单中文分词测试
SELECT TOKENIZE('你好世界', 'jieba') as tokens;

-- 1.3 英文分词测试
SELECT TOKENIZE('hello world', 'jieba') as tokens;

-- 1.4 中英文混合测试
SELECT TOKENIZE('MiniOB是一个数据库系统', 'jieba') as tokens;

-- 1.5 复杂中文测试
SELECT TOKENIZE('全文索引是用于在大量文本数据中进行高效搜索的技术', 'jieba') as tokens;

-- 1.6 空字符串测试
SELECT TOKENIZE('', 'jieba') as tokens;

-- ============================================
-- 测试2: 创建全文索引
-- ============================================

-- 2.1 创建测试表
CREATE TABLE texts (
    id INT,
    content TEXT
);

-- 2.2 插入测试数据
INSERT INTO texts VALUES (1, '你好世界，这是一个测试');
INSERT INTO texts VALUES (2, '全文索引是数据库的重要功能');
INSERT INTO texts VALUES (3, 'MiniOB是一个优秀的数据库系统');
INSERT INTO texts VALUES (4, '你好，欢迎使用MiniOB数据库');
INSERT INTO texts VALUES (5, '数据库系统提供了高效的数据管理功能');

-- 2.3 创建全文索引
ALTER TABLE texts ADD FULLTEXT INDEX idx_texts_jieba (content) WITH PARSER jieba;

-- ============================================
-- 测试3: MATCH...AGAINST 查询测试
-- ============================================

-- 3.1 基本全文搜索（包含score）
SELECT id, content, MATCH(content) AGAINST('你好') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('你好') > 0 
ORDER BY MATCH(content) AGAINST('你好') DESC, id ASC;

-- 3.2 搜索多个词条
SELECT id, content, MATCH(content) AGAINST('数据库') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('数据库') > 0 
ORDER BY MATCH(content) AGAINST('数据库') DESC, id ASC;

-- 3.3 搜索"功能"
SELECT id, content, MATCH(content) AGAINST('功能') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('功能') > 0 
ORDER BY MATCH(content) AGAINST('功能') DESC, id ASC;

-- 3.4 搜索不存在的内容（应该返回空结果）
SELECT id, content, MATCH(content) AGAINST('不存在的内容') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('不存在的内容') > 0 
ORDER BY MATCH(content) AGAINST('不存在的内容') DESC, id ASC;

-- 3.5 仅查询score（不使用WHERE条件）
SELECT id, content, MATCH(content) AGAINST('你好') AS score 
FROM texts 
ORDER BY MATCH(content) AGAINST('你好') DESC, id ASC;

-- 3.6 多词搜索（BM25评分测试）
SELECT id, content, MATCH(content) AGAINST('数据库系统') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('数据库系统') > 0 
ORDER BY MATCH(content) AGAINST('数据库系统') DESC, id ASC;

-- ============================================
-- 测试4: 综合测试场景
-- ============================================

-- 4.1 插入更多数据用于测试BM25评分
INSERT INTO texts VALUES (6, '数据库数据库数据库系统系统系统');  -- 高频词条，测试TF影响
INSERT INTO texts VALUES (7, '简单短文本');  -- 短文档，测试文档长度归一化

-- 4.2 测试BM25评分排序
SELECT id, content, MATCH(content) AGAINST('数据库') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('数据库') > 0 
ORDER BY MATCH(content) AGAINST('数据库') DESC, id ASC;

-- 4.3 测试相同score时按id排序
SELECT id, content, MATCH(content) AGAINST('你好') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('你好') > 0 
ORDER BY MATCH(content) AGAINST('你好') DESC, id ASC;

-- ============================================
-- 测试5: 边界情况测试
-- ============================================

-- 5.1 空内容测试
INSERT INTO texts VALUES (8, '');

-- 5.2 NULL内容测试
INSERT INTO texts VALUES (9, NULL);

-- 5.3 查询空内容
SELECT id, content, MATCH(content) AGAINST('测试') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('测试') > 0;

-- ============================================
-- 测试6: 更新和删除测试
-- ============================================

-- 6.1 更新文档内容
UPDATE texts SET content = '更新后的内容，包含新的关键词' WHERE id = 1;

-- 6.2 查询更新后的内容
SELECT id, content, MATCH(content) AGAINST('新的') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('新的') > 0 
ORDER BY MATCH(content) AGAINST('新的') DESC, id ASC;

-- 6.3 删除文档
DELETE FROM texts WHERE id = 2;

-- 6.4 验证删除后索引更新
SELECT id, content, MATCH(content) AGAINST('你好') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('你好') > 0 
ORDER BY MATCH(content) AGAINST('你好') DESC, id ASC;




