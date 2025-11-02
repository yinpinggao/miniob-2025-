-- ===================================================================
-- BM25 Epsilon处理测试
-- 基于 rank_bm25 (https://github.com/dorianbrown/rank_bm25)
-- ===================================================================
-- 
-- rank_bm25的BM25Okapi实现使用epsilon处理负IDF：
-- - epsilon = 0.25 (默认值)
-- - 当IDF < 0时（词在超过一半文档中出现），替换为 epsilon * average_idf
--
-- IDF公式: log((N - df + 0.5) / (df + 0.5))
-- 参数: k1 = 1.5, b = 0.75
-- ===================================================================

-- 测试1: 常见词的epsilon处理
DROP TABLE IF EXISTS epsilon_test1;
CREATE TABLE epsilon_test1 (id INT, content TEXT);

-- 创建5个文档，让"常见词"出现在4个文档中（超过一半）
INSERT INTO epsilon_test1 VALUES (1, '常见词 苹果');
INSERT INTO epsilon_test1 VALUES (2, '常见词 香蕉');
INSERT INTO epsilon_test1 VALUES (3, '常见词 橙子');
INSERT INTO epsilon_test1 VALUES (4, '常见词 葡萄');
INSERT INTO epsilon_test1 VALUES (5, '稀有词 西瓜');

ALTER TABLE epsilon_test1 ADD FULLTEXT INDEX idx1 (content) WITH PARSER jieba;

SELECT '=== 测试1: 常见词的epsilon处理 ===' AS test;
SELECT id, content, MATCH(content) AGAINST('常见词') AS score 
FROM epsilon_test1 
ORDER BY score DESC, id;

-- 手工计算验证：
-- N = 5, df(常见词) = 4
-- 原始IDF = log((5-4+0.5)/(4+0.5)) = log(1.5/4.5) = log(0.333) ≈ -1.099 (负数！)
--
-- 需要计算average_idf:
-- 假设只查询"常见词"一个词，average_idf = -1.099
-- eps = 0.25 * (-1.099) = -0.275
-- 
-- 由于实现中IDF<0会被替换，最终使用的IDF = eps = -0.275
-- （或者取绝对值后为正值）


-- 测试2: 对比稀有词和常见词
SELECT '=== 测试2: 稀有词 vs 常见词的IDF对比 ===' AS test;

-- 查询稀有词（只在1个文档中）
SELECT 'Query: 稀有词' AS query_type;
SELECT id, content, MATCH(content) AGAINST('稀有词') AS score 
FROM epsilon_test1 
WHERE MATCH(content) AGAINST('稀有词') > 0
ORDER BY score DESC, id;

-- 查询常见词（在4个文档中，触发epsilon处理）
SELECT 'Query: 常见词' AS query_type;
SELECT id, content, MATCH(content) AGAINST('常见词') AS score 
FROM epsilon_test1 
WHERE MATCH(content) AGAINST('常见词') > 0
ORDER BY score DESC, id;

-- 预期：
-- 稀有词的IDF：log((5-1+0.5)/(1+0.5)) = log(4.5/1.5) = log(3) ≈ 1.099 (正数)
-- 常见词的IDF：会被epsilon处理后的值替换


-- 测试3: 多词查询的average_idf计算
DROP TABLE IF EXISTS epsilon_test2;
CREATE TABLE epsilon_test2 (id INT, content TEXT);

INSERT INTO epsilon_test2 VALUES (1, '数据库 系统 管理');
INSERT INTO epsilon_test2 VALUES (2, '数据库 系统 设计');
INSERT INTO epsilon_test2 VALUES (3, '数据库 系统 优化');
INSERT INTO epsilon_test2 VALUES (4, '计算机 网络 协议');

ALTER TABLE epsilon_test2 ADD FULLTEXT INDEX idx2 (content) WITH PARSER jieba;

SELECT '=== 测试3: 多词查询的average_idf ===' AS test;
SELECT id, content, 
       MATCH(content) AGAINST('数据库 系统') AS score 
FROM epsilon_test2 
ORDER BY score DESC, id;

-- 手工计算：
-- N = 4
-- df(数据库) = 3, IDF(数据库) = log((4-3+0.5)/(3+0.5)) = log(1.5/3.5) ≈ -0.847 (负数)
-- df(系统) = 3, IDF(系统) = log((4-3+0.5)/(3+0.5)) ≈ -0.847 (负数)
-- 
-- average_idf = (-0.847 + -0.847) / 2 = -0.847
-- eps = 0.25 * (-0.847) = -0.212
-- 
-- 两个词的IDF都被替换为 -0.212（或取绝对值）
-- 
-- 文档1-3都包含"数据库"和"系统"各1次，应该得分相同


-- 测试4: 混合查询（稀有词 + 常见词）
DROP TABLE IF EXISTS epsilon_test3;
CREATE TABLE epsilon_test3 (id INT, content TEXT);

INSERT INTO epsilon_test3 VALUES (1, '常见词 稀有词 内容');
INSERT INTO epsilon_test3 VALUES (2, '常见词 一般词 内容');
INSERT INTO epsilon_test3 VALUES (3, '常见词 中等词 内容');
INSERT INTO epsilon_test3 VALUES (4, '常见词 普通词 内容');
INSERT INTO epsilon_test3 VALUES (5, '其他词 测试词 内容');

ALTER TABLE epsilon_test3 ADD FULLTEXT INDEX idx3 (content) WITH PARSER jieba;

SELECT '=== 测试4: 混合查询（稀有词+常见词）===' AS test;
SELECT id, content, 
       MATCH(content) AGAINST('常见词 稀有词') AS score 
FROM epsilon_test3 
WHERE MATCH(content) AGAINST('常见词 稀有词') > 0
ORDER BY score DESC, id;

-- 计算说明：
-- N = 5
-- df(常见词) = 4, 原始IDF ≈ -1.099 (负数)
-- df(稀有词) = 1, 原始IDF = log(4.5/1.5) ≈ 1.099 (正数)
-- 
-- average_idf = (-1.099 + 1.099) / 2 = 0.0
-- eps = 0.25 * 0.0 = 0.0
-- 
-- "常见词"的IDF被替换为0.0
-- "稀有词"的IDF保持1.099
-- 
-- 文档1包含两个词，但"常见词"贡献0分，只有"稀有词"有贡献
-- 预期：文档1的分数主要来自"稀有词"


-- 测试5: 极端情况 - 所有词都很常见
DROP TABLE IF EXISTS epsilon_test4;
CREATE TABLE epsilon_test4 (id INT, content TEXT);

INSERT INTO epsilon_test4 VALUES (1, '的 是 在 有');
INSERT INTO epsilon_test4 VALUES (2, '的 是 在 有');
INSERT INTO epsilon_test4 VALUES (3, '的 是 在 有');

ALTER TABLE epsilon_test4 ADD FULLTEXT INDEX idx4 (content) WITH PARSER jieba;

SELECT '=== 测试5: 停用词已被过滤 ===' AS test;
SELECT id, content, 
       MATCH(content) AGAINST('的 是 在') AS score 
FROM epsilon_test4;

-- 注意：这些都是停用词，应该在分词时就被过滤掉
-- 预期：所有文档分数都是0（因为没有有效的查询词）


-- 测试6: 测评平台用例模拟（带epsilon处理）
DROP TABLE IF EXISTS texts;
CREATE TABLE texts (id INT, content TEXT);

INSERT INTO texts VALUES (0, 'ORDERED和LEADINGHint有什么区别？如果同时使用这两个Hint，哪一个会生效？');
INSERT INTO texts VALUES (1, 'USER_AUDIT_OBJECT视图中的TIMESTAMP和EXTENDED_TIMESTAMP字段有什么区别？');
INSERT INTO texts VALUES (2, 'OceanBaseV4.0.0版本在性能优化方面取得了哪些成果？');
INSERT INTO texts VALUES (3, 'Locality属性在高可用架构中起到什么作用？');
INSERT INTO texts VALUES (4, 'LEADINGHint在查询优化中的应用场景有哪些？');

ALTER TABLE texts ADD FULLTEXT INDEX idx_texts (content) WITH PARSER jieba;

SELECT '=== 测试6: 测评用例（期望文档0得分约1.85）===' AS test;
SELECT id, content, 
       MATCH(content) AGAINST('LEADINGHint') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('LEADINGHint') > 0 
ORDER BY score DESC, id;

-- 计算说明：
-- N = 5, df(LEADINGHint) = 2 (文档0和4)
-- IDF = log((5-2+0.5)/(2+0.5)) = log(3.5/2.5) = log(1.4) ≈ 0.336
-- 这是正数，不需要epsilon处理
--
-- 文档0: "LEADINGHint"出现2次，文档长度约24词（分词后）
-- 文档4: "LEADINGHint"出现1次，文档长度约11词
--
-- 文档0应该得分更高（词频更高）


-- 清理说明
SELECT '=== BM25 Epsilon测试完成 ===' AS result;
SELECT '说明: epsilon=0.25 用于处理负IDF（常见词）' AS note1;
SELECT '当词出现在超过一半文档中时，IDF<0，会被替换为 epsilon*average_idf' AS note2;
SELECT '这确保了常见词不会产生负贡献' AS note3;

