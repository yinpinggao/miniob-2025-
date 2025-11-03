/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/index/fulltext_index.h"
#include "common/fulltext/jieba_util.h"
#include "common/log/log.h"
#include <algorithm>
#include <cmath>

FullTextIndex::FullTextIndex() : total_tokens_(0), average_idf_(0.0), average_idf_cached_(false) {}

RC FullTextIndex::add_document(const RID &doc_rid, const std::string &text, const std::vector<std::string> *tokens)
{
  std::vector<std::string> doc_tokens;
  
  if (tokens != nullptr) {
    doc_tokens = *tokens;
  } else {
    // 如果没有提供分词结果，则使用jieba分词
    RC rc = JiebaUtil::instance().tokenize(text, doc_tokens);
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to tokenize document text");
      return rc;
    }
  }
  
  // 如果文档已经存在，先删除
  if (doc_stats_.find(doc_rid) != doc_stats_.end()) {
    remove_document(doc_rid);
  }
  
  // 统计词条频率
  std::unordered_map<std::string, int> term_freq_map;
  for (const auto &token : doc_tokens) {
    if (!token.empty()) {
      term_freq_map[token]++;
    }
  }
  
  // 更新倒排索引
  for (const auto &pair : term_freq_map) {
    const std::string &term = pair.first;
    int term_freq = pair.second;
    
    // 在倒排列表中找到或创建该文档的条目
    PostingList &posting_list = inverted_index_[term];
    auto it = std::find_if(posting_list.begin(), posting_list.end(),
                          [&doc_rid](const PostingEntry &entry) {
                            return entry.doc_rid == doc_rid;
                          });
    
    if (it != posting_list.end()) {
      // 更新现有条目
      it->term_freq = term_freq;
    } else {
      // 添加新条目
      posting_list.emplace_back(doc_rid, term_freq);
    }
  }
  
  // 更新文档统计信息
  int doc_length = static_cast<int>(doc_tokens.size());
  doc_stats_[doc_rid] = DocumentStats(doc_rid, doc_length);
  total_tokens_ += doc_length;
  
  // 索引发生变化，使 average_idf 缓存失效
  average_idf_cached_ = false;
  
  return RC::SUCCESS;
}

RC FullTextIndex::remove_document(const RID &doc_rid)
{
  auto doc_it = doc_stats_.find(doc_rid);
  if (doc_it == doc_stats_.end()) {
    return RC::SUCCESS;  // 文档不存在，不需要删除
  }
  
  int doc_length = doc_it->second.doc_length;
  
  // 从倒排索引中删除该文档的所有条目
  for (auto &pair : inverted_index_) {
    PostingList &posting_list = pair.second;
    posting_list.erase(
      std::remove_if(posting_list.begin(), posting_list.end(),
                    [&doc_rid](const PostingEntry &entry) {
                      return entry.doc_rid == doc_rid;
                    }),
      posting_list.end()
    );
    
    // 如果倒排列表为空，可以考虑删除该词条（为了简化，暂时保留）
  }
  
  // 删除文档统计信息
  doc_stats_.erase(doc_it);
  total_tokens_ -= doc_length;
  
  // 索引发生变化，使 average_idf 缓存失效
  average_idf_cached_ = false;
  
  return RC::SUCCESS;
}

RC FullTextIndex::update_document(const RID &doc_rid, const std::string &text, const std::vector<std::string> *tokens)
{
  RC rc = remove_document(doc_rid);
  if (OB_FAIL(rc)) {
    return rc;
  }
  
  return add_document(doc_rid, text, tokens);
}

RC FullTextIndex::search(const std::vector<std::string> &query_tokens, std::vector<RID> &doc_rids) const
{
  doc_rids.clear();
  
  if (query_tokens.empty()) {
    return RC::SUCCESS;
  }
  
  // 找到包含所有查询词条的文档（交集）
  std::unordered_set<RID, RIDHash> doc_set;
  
  for (size_t i = 0; i < query_tokens.size(); ++i) {
    const std::string &term = query_tokens[i];
    auto it = inverted_index_.find(term);
    
    if (it == inverted_index_.end()) {
      // 如果某个词条不存在，则没有文档匹配
      return RC::SUCCESS;  // 返回空结果
    }
    
    const PostingList &posting_list = it->second;
    std::unordered_set<RID, RIDHash> term_docs;
    
    for (const auto &entry : posting_list) {
      term_docs.insert(entry.doc_rid);
    }
    
    if (i == 0) {
      // 第一个词条，初始化结果集
      doc_set = term_docs;
    } else {
      // 后续词条，求交集
      std::unordered_set<RID, RIDHash> intersection;
      for (const auto &rid : doc_set) {
        if (term_docs.find(rid) != term_docs.end()) {
          intersection.insert(rid);
        }
      }
      doc_set = std::move(intersection);
    }
  }
  
  // 转换为向量
  doc_rids.assign(doc_set.begin(), doc_set.end());
  
  return RC::SUCCESS;
}

double FullTextIndex::calculate_bm25(const std::vector<std::string> &query_tokens, const RID &doc_rid) const
{
  auto doc_it = doc_stats_.find(doc_rid);
  if (doc_it == doc_stats_.end()) {
    return 0.0;  // 文档不存在
  }
  
  int doc_length = doc_it->second.doc_length;
  size_t total_docs = doc_stats_.size();
  double avg_doc_length = average_document_length();
  
  // BM25Okapi with epsilon handling for negative IDF
  // 参考: https://github.com/dorianbrown/rank_bm25
  const double epsilon = 0.25;  // 与rank_bm25库一致
  
  // 第一步：获取或计算 average_idf（基于所有词条，而不是只查询词）
  if (!average_idf_cached_) {
    average_idf_ = calculate_average_idf();
    average_idf_cached_ = true;
  }
  double eps = epsilon * average_idf_;
  
  // 第二步：计算查询词的IDF并应用epsilon处理
  std::unordered_map<std::string, double> idf_map;
  
  for (const std::string &term : query_tokens) {
    if (idf_map.find(term) != idf_map.end()) {
      continue;  // 已经计算过该词的IDF
    }
    
    auto index_it = inverted_index_.find(term);
    if (index_it == inverted_index_.end()) {
      continue;  // 词条不存在于任何文档中
    }
    
    size_t doc_freq = index_it->second.size();
    double idf = calculate_idf(term, doc_freq, total_docs);
    
    // 应用epsilon处理：如果IDF < 0，替换为 eps
    if (idf < 0) {
      idf = eps;
    }
    
    idf_map[term] = idf;
  }
  
  // 第三步：计算最终BM25分数
  double score = 0.0;
  
  for (const std::string &term : query_tokens) {
    auto idf_it = idf_map.find(term);
    if (idf_it == idf_map.end()) {
      continue;  // 词条不存在
    }
    
    auto index_it = inverted_index_.find(term);
    if (index_it == inverted_index_.end()) {
      continue;
    }
    
    const PostingList &posting_list = index_it->second;
    
    // 找到该文档在该词条的倒排列表中的条目
    auto entry_it = std::find_if(posting_list.begin(), posting_list.end(),
                                 [&doc_rid](const PostingEntry &entry) {
                                   return entry.doc_rid == doc_rid;
                                 });
    
    if (entry_it == posting_list.end()) {
      continue;  // 文档中不包含该词条，跳过
    }
    
    int term_freq = entry_it->term_freq;
    double idf = idf_it->second;  // 使用处理后的IDF
    
    // 计算BM25分数（单个词条）
    double term_score = calculate_term_bm25(term, doc_rid, term_freq, doc_length, avg_doc_length);
    
    // 累加所有词条的分数
    score += idf * term_score;
  }
  
  return score;
}

RC FullTextIndex::search_with_scores(const std::vector<std::string> &query_tokens,
                                     std::vector<std::pair<RID, double>> &results) const
{
  results.clear();
  
  if (query_tokens.empty()) {
    return RC::SUCCESS;
  }
  
  // 找到所有包含至少一个查询词条的文档（并集）
  std::unordered_set<RID, RIDHash> doc_set;
  
  for (const std::string &term : query_tokens) {
    auto it = inverted_index_.find(term);
    if (it != inverted_index_.end()) {
      for (const auto &entry : it->second) {
        doc_set.insert(entry.doc_rid);
      }
    }
  }
  
  // 计算每个文档的BM25分数
  for (const RID &doc_rid : doc_set) {
    double score = calculate_bm25(query_tokens, doc_rid);
    if (score > 0.0) {  // 只包含分数大于0的文档
      results.emplace_back(doc_rid, score);
    }
  }
  
  // 按分数降序排序
  std::sort(results.begin(), results.end(),
            [](const std::pair<RID, double> &a, const std::pair<RID, double> &b) {
              if (std::abs(a.second - b.second) < 1e-9) {
                // 分数相同，按RID排序（升序）
                int cmp = RID::compare(&a.first, &b.first);
                return cmp < 0;
              }
              return a.second > b.second;  // 分数降序
            });
  
  return RC::SUCCESS;
}

double FullTextIndex::average_document_length() const
{
  if (doc_stats_.empty()) {
    return 0.0;
  }
  return static_cast<double>(total_tokens_) / doc_stats_.size();
}

size_t FullTextIndex::document_frequency(const std::string &term) const
{
  auto it = inverted_index_.find(term);
  if (it == inverted_index_.end()) {
    return 0;
  }
  return it->second.size();
}

void FullTextIndex::clear()
{
  inverted_index_.clear();
  doc_stats_.clear();
  total_tokens_ = 0;
  average_idf_cached_ = false;
}

double FullTextIndex::calculate_idf(const std::string &term, size_t doc_freq, size_t total_docs) const
{
  if (doc_freq == 0 || total_docs == 0) {
    return 0.0;
  }
  
  // IDF = log((N - df + 0.5) / (df + 0.5))
  // 其中 N 是文档总数，df 是包含该词条的文档数量
  double numerator = static_cast<double>(total_docs) - doc_freq + 0.5;
  double denominator = static_cast<double>(doc_freq) + 0.5;
  
  return std::log(numerator / denominator);
}

double FullTextIndex::calculate_term_bm25(const std::string &term, const RID &doc_rid,
                                          int term_freq, int doc_length, double avg_doc_length) const
{
  const double k1 = 1.5;
  const double b = 0.75;
  
  // BM25公式: (f(qi, D) * (k1 + 1)) / (f(qi, D) + k1 * (1 - b + b * |D| / avgdl))
  // 其中 f(qi, D) 是词条在文档中的频率，|D| 是文档长度，avgdl 是平均文档长度
  
  double numerator = term_freq * (k1 + 1.0);
  double denominator = term_freq + k1 * (1.0 - b + b * (static_cast<double>(doc_length) / avg_doc_length));
  
  if (denominator < 1e-9) {
    return 0.0;
  }
  
  return numerator / denominator;
}

double FullTextIndex::calculate_average_idf() const
{
  if (inverted_index_.empty() || doc_stats_.empty()) {
    return 0.0;
  }
  
  size_t total_docs = doc_stats_.size();
  double idf_sum = 0.0;
  
  // 遍历所有词条，计算所有词的IDF之和（与rank_bm25一致）
  for (const auto &pair : inverted_index_) {
    const std::string &term = pair.first;
    size_t doc_freq = pair.second.size();
    double idf = calculate_idf(term, doc_freq, total_docs);
    idf_sum += idf;
  }
  
  return idf_sum / inverted_index_.size();
}

