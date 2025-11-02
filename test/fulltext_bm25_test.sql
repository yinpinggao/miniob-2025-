-- ===================================================================
-- 全文索引 BM25 测试用例
-- ===================================================================

-- 清理环境
DROP TABLE IF EXISTS docs;

-- 1. 创建测试表
CREATE TABLE docs (id INT, title TEXT, content TEXT);

-- 2. 插入测试数据
-- 文档1: 包含 "数据库" 1次，总长度较短
INSERT INTO docs VALUES (1, '简介', '数据库是存储数据的系统');

-- 文档2: 包含 "数据库" 2次，总长度中等
INSERT INTO docs VALUES (2, '教程', '数据库管理系统是专门管理数据库的软件，数据库技术很重要');

-- 文档3: 包含 "数据库" 3次，总长度较长
INSERT INTO docs VALUES (3, '详解', '数据库系统包括数据库、数据库管理系统和应用程序。学习数据库需要理解其原理。数据库设计是关键环节');

-- 文档4: 不包含 "数据库"，用于对比
INSERT INTO docs VALUES (4, '其他', '这是一篇关于计算机网络的文章，讲述了网络协议和通信技术');

-- 文档5: 包含 "数据库" 1次，但文档很长
INSERT INTO docs VALUES (5, '综合', '本文介绍计算机科学的多个领域，包括算法、数据结构、操作系统、编译原理、计算机网络以及数据库等多个方面的内容');

-- 3. 创建全文索引
ALTER TABLE docs ADD FULLTEXT INDEX idx_content (content) WITH PARSER jieba;

-- 4. 基础查询测试
SELECT '=== 测试1: 基础全文搜索 ===' AS test_name;
SELECT id, content, MATCH(content) AGAINST('数据库') AS score 
FROM docs 
WHERE MATCH(content) AGAINST('数据库') > 0
ORDER BY score DESC, id ASC;

-- 预期结果分析：
-- 文档3应该得分最高（包含"数据库"3次，但文档较长）
-- 文档2应该得分第二（包含"数据库"2次，文档中等长度）
-- 文档1可能得分较高（只包含1次但文档很短，密度高）
-- 文档5得分较低（包含1次但文档很长，密度低）

-- 5. 查看所有文档的BM25分数（包括0分）
SELECT '=== 测试2: 所有文档的BM25分数 ===' AS test_name;
SELECT id, 
       MATCH(content) AGAINST('数据库') AS score,
       content
FROM docs
ORDER BY score DESC, id ASC;

-- 6. 多词查询测试
SELECT '=== 测试3: 多词查询 ===' AS test_name;
SELECT id, 
       MATCH(content) AGAINST('数据库 系统') AS score,
       content
FROM docs
WHERE MATCH(content) AGAINST('数据库 系统') > 0
ORDER BY score DESC, id ASC;

-- 预期：同时包含"数据库"和"系统"的文档得分更高

-- 7. 稀有词查询测试
SELECT '=== 测试4: 稀有词查询（高IDF）===' AS test_name;
SELECT id, 
       MATCH(content) AGAINST('编译原理') AS score,
       content
FROM docs
WHERE MATCH(content) AGAINST('编译原理') > 0
ORDER BY score DESC, id ASC;

-- 预期：只有文档5包含"编译原理"，应该得到较高分数（稀有词IDF高）

-- 8. 常见词查询测试
SELECT '=== 测试5: 更复杂的查询 ===' AS test_name;
SELECT id, title, 
       MATCH(content) AGAINST('数据库管理系统') AS score
FROM docs
WHERE MATCH(content) AGAINST('数据库管理系统') > 0
ORDER BY score DESC, id ASC;

-- 9. 验证BM25参数影响
-- 插入一个非常长的文档
INSERT INTO docs VALUES (6, '长文档', '这是一个非常长的文档用于测试BM25的文档长度归一化功能。' ||
'我们需要确保长文档不会因为包含更多词而获得不公平的优势。' ||
'BM25算法通过参数b来控制文档长度的影响。' ||
'当文档长度超过平均文档长度时会受到惩罚。' ||
'这篇文档虽然很长但只包含数据库这个词一次。' ||
'相比之下短文档如果包含相同的词应该获得更高的分数。' ||
'这就是为什么BM25比简单的TF-IDF更好的原因。');

-- 重新创建索引以包含新文档
DROP INDEX idx_content ON docs;
ALTER TABLE docs ADD FULLTEXT INDEX idx_content (content) WITH PARSER jieba;

SELECT '=== 测试6: 文档长度归一化测试 ===' AS test_name;
SELECT id, title,
       MATCH(content) AGAINST('数据库') AS score,
       LENGTH(content) AS doc_length
FROM docs
WHERE MATCH(content) AGAINST('数据库') > 0
ORDER BY score DESC, id ASC;

-- 预期：
-- 文档1（短文档，包含1次）应该比文档6（长文档，包含1次）得分高
-- 这验证了BM25的文档长度归一化功能

-- 10. 组合查询：在WHERE和ORDER BY中使用
SELECT '=== 测试7: 组合查询 ===' AS test_name;
SELECT id, title,
       MATCH(content) AGAINST('计算机') AS score1,
       MATCH(content) AGAINST('数据库') AS score2,
       MATCH(content) AGAINST('计算机') + MATCH(content) AGAINST('数据库') AS total_score
FROM docs
WHERE MATCH(content) AGAINST('计算机') > 0 OR MATCH(content) AGAINST('数据库') > 0
ORDER BY total_score DESC, id ASC;

-- 11. 精确的BM25计算验证
-- 创建一个简单的场景用于手工验证BM25计算
DROP TABLE IF EXISTS simple_docs;
CREATE TABLE simple_docs (id INT, content TEXT);

INSERT INTO simple_docs VALUES (1, '苹果');
INSERT INTO simple_docs VALUES (2, '苹果 苹果');
INSERT INTO simple_docs VALUES (3, '苹果 苹果 苹果');
INSERT INTO simple_docs VALUES (4, '香蕉');

ALTER TABLE simple_docs ADD FULLTEXT INDEX idx_simple (content) WITH PARSER jieba;

SELECT '=== 测试8: 简单BM25验证 ===' AS test_name;
SELECT id, content,
       MATCH(content) AGAINST('苹果') AS score
FROM simple_docs
ORDER BY score DESC, id ASC;

-- BM25计算（手工验证）：
-- 参数: k1=1.5, b=0.75
-- N = 4 (总文档数)
-- df(苹果) = 3 (包含"苹果"的文档数)
-- IDF(苹果) = log((4-3+0.5)/(3+0.5)) = log(1.5/3.5) = log(0.4286) ≈ -0.848
-- 
-- avgdl = (1+2+3+1)/4 = 1.75
--
-- 文档1: TF=1, |D|=1
--   term_score = (1*(1.5+1)) / (1 + 1.5*(1-0.75+0.75*1/1.75))
--              = 2.5 / (1 + 1.5*0.6786) = 2.5 / 2.018 ≈ 1.239
--   BM25 = -0.848 * 1.239 ≈ -1.051 (但应该取绝对值或修正IDF公式)
--
-- 注意：如果IDF是负数，说明词太常见。标准BM25会调整IDF公式或设置下限。

-- 12. 测试停用词过滤
DROP TABLE IF EXISTS stopword_test;
CREATE TABLE stopword_test (id INT, content TEXT);

INSERT INTO stopword_test VALUES (1, '我的数据库系统');
INSERT INTO stopword_test VALUES (2, '数据库系统很重要');
INSERT INTO stopword_test VALUES (3, '这个那个数据库系统');

ALTER TABLE stopword_test ADD FULLTEXT INDEX idx_stop (content) WITH PARSER jieba;

SELECT '=== 测试9: 停用词过滤测试 ===' AS test_name;
SELECT id, content,
       MATCH(content) AGAINST('数据库') AS score
FROM stopword_test
ORDER BY score DESC, id ASC;

-- 预期：停用词（的、我、这、那）应该被过滤，不影响BM25计算

-- 13. 边界情况测试
SELECT '=== 测试10: 空查询 ===' AS test_name;
SELECT id, MATCH(content) AGAINST('') AS score FROM docs LIMIT 3;

SELECT '=== 测试11: 查询不存在的词 ===' AS test_name;
SELECT id, MATCH(content) AGAINST('量子物理学') AS score 
FROM docs 
WHERE MATCH(content) AGAINST('量子物理学') > 0;

-- 14. 性能测试（可选）
SELECT '=== 测试12: 批量数据BM25 ===' AS test_name;
SELECT COUNT(*) as total_matches,
       AVG(MATCH(content) AGAINST('数据库')) as avg_score,
       MAX(MATCH(content) AGAINST('数据库')) as max_score,
       MIN(MATCH(content) AGAINST('数据库')) as min_score
FROM docs
WHERE MATCH(content) AGAINST('数据库') > 0;

-- 清理
-- DROP TABLE docs;
-- DROP TABLE simple_docs;
-- DROP TABLE stopword_test;

SELECT '=== 所有测试完成 ===' AS test_name;

