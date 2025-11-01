-- 全文索引功能简化测试用例（按照您提供的示例）

-- 创建表
CREATE TABLE texts (
    id INT PRIMARY KEY,
    content TEXT
);

-- 插入测试数据
INSERT INTO texts VALUES (1, '你好世界');
INSERT INTO texts VALUES (2, '你好，欢迎');
INSERT INTO texts VALUES (3, '世界你好');
INSERT INTO texts VALUES (4, '其他内容');

-- 创建全文索引
ALTER TABLE texts ADD FULLTEXT INDEX idx_texts_jieba (content) WITH PARSER jieba;

-- 测试1: TOKENIZE函数
SELECT TOKENIZE('information_schema.SCHEMATA表的主要功能是什么？', 'jieba') as text_tokens;

-- 测试2: MATCH...AGAINST查询（按您的示例）
SELECT id, content, MATCH(content) AGAINST('你好') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('你好') > 0 
ORDER BY MATCH(content) AGAINST('你好') DESC, id ASC;


