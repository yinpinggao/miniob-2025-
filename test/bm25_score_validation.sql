-- ===================================================================
-- BM25 分数计算验证测试
-- 本测试文件包含具体的BM25分数计算过程，便于验证实现正确性
-- ===================================================================

-- 测试场景1: 简单的3文档测试
DROP TABLE IF EXISTS test1;
CREATE TABLE test1 (id INT, content TEXT);

INSERT INTO test1 VALUES (1, '苹果 苹果');           -- 文档1: 2个词
INSERT INTO test1 VALUES (2, '苹果 香蕉 橙子');     -- 文档2: 3个词  
INSERT INTO test1 VALUES (3, '香蕉 香蕉 香蕉');     -- 文档3: 3个词

ALTER TABLE test1 ADD FULLTEXT INDEX idx1 (content) WITH PARSER jieba;

SELECT '=== 场景1: 基础BM25计算 ===' AS scenario;
SELECT id, content, MATCH(content) AGAINST('苹果') AS score FROM test1 ORDER BY score DESC, id;

-- 手工计算验证：
-- 参数: k1=1.5, b=0.75
-- 
-- 文档统计：
-- - 总文档数 N = 3
-- - 包含"苹果"的文档数 df = 2 (文档1,2)
-- - 平均文档长度 avgdl = (2+3+3)/3 = 2.67
-- 
-- IDF计算：
-- IDF(苹果) = log((N - df + 0.5) / (df + 0.5))
--          = log((3 - 2 + 0.5) / (2 + 0.5))
--          = log(1.5 / 2.5)
--          = log(0.6)
--          ≈ -0.511
-- 
-- 文档1 BM25:
-- - TF(苹果, D1) = 2
-- - |D1| = 2
-- - term_score = (2 * (1.5+1)) / (2 + 1.5*(1-0.75+0.75*2/2.67))
--              = 5 / (2 + 1.5*0.8122)
--              = 5 / 3.218
--              ≈ 1.554
-- - BM25 = IDF * term_score = -0.511 * 1.554 ≈ -0.794
--   (注意：由于IDF为负，实际实现可能调整为正值或使用修正公式)
--
-- 文档2 BM25:
-- - TF(苹果, D2) = 1
-- - |D2| = 3
-- - term_score = (1 * 2.5) / (1 + 1.5*1.0)
--              = 2.5 / 2.5
--              = 1.0
-- - BM25 = -0.511 * 1.0 ≈ -0.511
--
-- 预期排序: 文档1 > 文档2（文档1包含更多次"苹果"）


-- 测试场景2: 文档长度归一化验证
DROP TABLE IF EXISTS test2;
CREATE TABLE test2 (id INT, content TEXT);

INSERT INTO test2 VALUES (1, '数据库');                                    -- 短文档，1词
INSERT INTO test2 VALUES (2, '数据库 系统 管理 软件 技术 应用 开发');    -- 长文档，7词

ALTER TABLE test2 ADD FULLTEXT INDEX idx2 (content) WITH PARSER jieba;

SELECT '=== 场景2: 文档长度归一化 ===' AS scenario;
SELECT id, 
       LENGTH(content) as len,
       MATCH(content) AGAINST('数据库') AS score 
FROM test2 
ORDER BY score DESC, id;

-- 预期结果：
-- 文档1（短文档）的BM25分数 > 文档2（长文档）的BM25分数
-- 因为它们都只包含"数据库"1次，但文档1更短，相关性更高
--
-- 计算说明：
-- N = 2, df(数据库) = 2
-- IDF = log((2-2+0.5)/(2+0.5)) = log(0.5/2.5) = log(0.2) ≈ -1.609
-- avgdl = (1+7)/2 = 4
--
-- 文档1: TF=1, |D|=1
-- term_score = 2.5 / (1 + 1.5*(1-0.75+0.75*1/4))
--            = 2.5 / (1 + 1.5*0.4375)
--            = 2.5 / 1.656
--            ≈ 1.509
-- BM25 ≈ -1.609 * 1.509 ≈ -2.428 (或正值修正后约 2.428)
--
-- 文档2: TF=1, |D|=7
-- term_score = 2.5 / (1 + 1.5*(1-0.75+0.75*7/4))
--            = 2.5 / (1 + 1.5*1.5625)
--            = 2.5 / 3.344
--            ≈ 0.747
-- BM25 ≈ -1.609 * 0.747 ≈ -1.202 (或正值修正后约 1.202)
--
-- 因此: score(文档1) > score(文档2) ✓


-- 测试场景3: 词频影响
DROP TABLE IF EXISTS test3;
CREATE TABLE test3 (id INT, content TEXT);

INSERT INTO test3 VALUES (1, '数据库');
INSERT INTO test3 VALUES (2, '数据库 数据库');
INSERT INTO test3 VALUES (3, '数据库 数据库 数据库');

ALTER TABLE test3 ADD FULLTEXT INDEX idx3 (content) WITH PARSER jieba;

SELECT '=== 场景3: 词频对BM25的影响 ===' AS scenario;
SELECT id, content, MATCH(content) AGAINST('数据库') AS score FROM test3 ORDER BY score DESC, id;

-- 预期结果：
-- 文档3 > 文档2 > 文档1
-- 词频越高，BM25分数越高（但会因为文档长度而有所调整）
--
-- 注意：BM25的一个特点是词频的影响有饱和效应
-- TF从1增加到2的影响 > TF从2增加到3的影响


-- 测试场景4: IDF影响 - 稀有词 vs 常见词
DROP TABLE IF EXISTS test4;
CREATE TABLE test4 (id INT, content TEXT);

-- 创建多个文档，让某些词更常见
INSERT INTO test4 VALUES (1, '苹果 水果');
INSERT INTO test4 VALUES (2, '香蕉 水果');
INSERT INTO test4 VALUES (3, '橙子 水果');
INSERT INTO test4 VALUES (4, '西瓜 水果');
INSERT INTO test4 VALUES (5, '葡萄 水果');

ALTER TABLE test4 ADD FULLTEXT INDEX idx4 (content) WITH PARSER jieba;

SELECT '=== 场景4: IDF影响（常见词 vs 稀有词）===' AS scenario;

-- 查询常见词"水果"（出现在所有文档中）
SELECT 'Query: 水果 (常见词)' AS query_type;
SELECT id, MATCH(content) AGAINST('水果') AS score FROM test4 ORDER BY score DESC, id LIMIT 3;

-- 查询稀有词"苹果"（只出现在1个文档中）
SELECT 'Query: 苹果 (稀有词)' AS query_type;
SELECT id, MATCH(content) AGAINST('苹果') AS score FROM test4 ORDER BY score DESC, id;

-- 预期结果：
-- "苹果"的IDF > "水果"的IDF
-- 因为"苹果"更稀有（df=1），而"水果"很常见（df=5）
--
-- IDF(苹果) = log((5-1+0.5)/(1+0.5)) = log(4.5/1.5) = log(3) ≈ 1.099
-- IDF(水果) = log((5-5+0.5)/(5+0.5)) = log(0.5/5.5) = log(0.091) ≈ -2.398
--
-- 稀有词得分应该更高！


-- 测试场景5: 多词查询
DROP TABLE IF EXISTS test5;
CREATE TABLE test5 (id INT, content TEXT);

INSERT INTO test5 VALUES (1, '机器学习');
INSERT INTO test5 VALUES (2, '机器学习 人工智能');
INSERT INTO test5 VALUES (3, '深度学习 人工智能');
INSERT INTO test5 VALUES (4, '机器学习 深度学习 人工智能');

ALTER TABLE test5 ADD FULLTEXT INDEX idx5 (content) WITH PARSER jieba;

SELECT '=== 场景5: 多词查询BM25累加 ===' AS scenario;
SELECT id, content, 
       MATCH(content) AGAINST('机器学习 人工智能') AS score 
FROM test5 
WHERE MATCH(content) AGAINST('机器学习 人工智能') > 0
ORDER BY score DESC, id;

-- 预期结果：
-- BM25(多词) = BM25(词1) + BM25(词2) + ... + BM25(词n)
-- 文档4应该得分最高（包含两个查询词）
-- 文档2次之（包含两个词但只有2个词）


-- 测试场景6: 实际测评用例模拟
DROP TABLE IF EXISTS texts;
CREATE TABLE texts (id INT, content TEXT);

INSERT INTO texts VALUES (0, 'ORDERED和LEADINGHint有什么区别？如果同时使用这两个Hint，哪一个会生效？');
INSERT INTO texts VALUES (1, 'USER_AUDIT_OBJECT视图中的TIMESTAMP和EXTENDED_TIMESTAMP字段有什么区别？');
INSERT INTO texts VALUES (2, 'OceanBaseV4.0.0版本在性能优化方面取得了哪些成果？');
INSERT INTO texts VALUES (3, 'Locality属性在高可用架构中起到什么作用？');
INSERT INTO texts VALUES (4, 'LEADINGHint在查询优化中的应用场景有哪些？');

ALTER TABLE texts ADD FULLTEXT INDEX idx_texts_jieba (content) WITH PARSER jieba;

SELECT '=== 场景6: 测评用例模拟 ===' AS scenario;
SELECT id, content, MATCH(content) AGAINST('LEADINGHint') AS score 
FROM texts 
WHERE MATCH(content) AGAINST('LEADINGHint') > 0 
ORDER BY MATCH(content) AGAINST('LEADINGHint') DESC, id ASC;

-- 预期分析：
-- 文档0和文档4都包含"LEADINGHint"
-- 需要根据词频、文档长度、IDF等因素综合计算
-- 如果测评期望文档0得分1.85，那么你的实现应该接近这个值


SELECT '=== 所有BM25验证测试完成 ===' AS result;
SELECT '提示: 如果某些分数为负数，需要检查IDF公式是否使用了正确的BM25+版本' AS note;

