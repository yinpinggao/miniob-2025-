/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "common/rc.h"
#include "storage/record/record.h"

/**
 * @brief 倒排列表中的条目
 * 存储词条在某个文档中的信息
 */
struct PostingEntry {
  RID doc_rid;        ///< 文档ID（使用RID表示）
  int term_freq;      ///< 词条在该文档中出现的频率
  std::vector<int> positions;  ///< 词条在文档中出现的位置（可选，当前简化实现不使用）
  
  PostingEntry() : term_freq(0) {}
  PostingEntry(const RID &rid, int tf) : doc_rid(rid), term_freq(tf) {}
};

/**
 * @brief 倒排列表
 * 存储包含某个词条的所有文档
 */
using PostingList = std::vector<PostingEntry>;

/**
 * @brief 倒排索引
 * 存储：词条 -> 倒排列表（包含该词条的文档列表）
 */
using InvertedIndex = std::unordered_map<std::string, PostingList>;

/**
 * @brief 文档统计信息
 */
struct DocumentStats {
  RID doc_rid;
  int doc_length;  ///< 文档长度（词条数量）
  
  DocumentStats() : doc_length(0) {}
  DocumentStats(const RID &rid, int len) : doc_rid(rid), doc_length(len) {}
};

/**
 * @brief 全文索引数据结构
 */
class FullTextIndex {
public:
  FullTextIndex();
  ~FullTextIndex() = default;
  
  /**
   * @brief 添加文档到索引
   * @param doc_rid 文档ID
   * @param text 文档内容
   * @param tokens 分词结果（如果已经分词）
   * @return RC
   */
  RC add_document(const RID &doc_rid, const std::string &text, const std::vector<std::string> *tokens = nullptr);
  
  /**
   * @brief 从索引中删除文档
   * @param doc_rid 文档ID
   * @return RC
   */
  RC remove_document(const RID &doc_rid);
  
  /**
   * @brief 更新文档（先删除再添加）
   * @param doc_rid 文档ID
   * @param text 新文档内容
   * @param tokens 分词结果（如果已经分词）
   * @return RC
   */
  RC update_document(const RID &doc_rid, const std::string &text, const std::vector<std::string> *tokens = nullptr);
  
  /**
   * @brief 搜索包含查询词条的文档
   * @param query_tokens 查询词条列表
   * @param doc_rids 输出：包含查询词条的文档ID列表
   * @return RC
   */
  RC search(const std::vector<std::string> &query_tokens, std::vector<RID> &doc_rids) const;
  
  /**
   * @brief 计算BM25分数
   * @param query_tokens 查询词条列表
   * @param doc_rid 文档ID
   * @return BM25分数
   */
  double calculate_bm25(const std::vector<std::string> &query_tokens, const RID &doc_rid) const;
  
  /**
   * @brief 搜索并计算BM25分数，按分数排序
   * @param query_tokens 查询词条列表
   * @param results 输出：(文档ID, BM25分数)对列表，按分数降序排列
   * @return RC
   */
  RC search_with_scores(const std::vector<std::string> &query_tokens, 
                        std::vector<std::pair<RID, double>> &results) const;
  
  /**
   * @brief 获取文档数量
   */
  size_t document_count() const { return doc_stats_.size(); }
  
  /**
   * @brief 获取平均文档长度
   */
  double average_document_length() const;
  
  /**
   * @brief 获取某个词条的文档频率（包含该词条的文档数量）
   */
  size_t document_frequency(const std::string &term) const;
  
  /**
   * @brief 清空索引
   */
  void clear();

private:
  InvertedIndex inverted_index_;  ///< 倒排索引
  std::unordered_map<RID, DocumentStats, RIDHash> doc_stats_;  ///< 文档统计信息
  size_t total_tokens_;  ///< 所有文档的词条总数
  
  /**
   * @brief 计算逆文档频率（IDF）
   */
  double calculate_idf(const std::string &term, size_t doc_freq, size_t total_docs) const;
  
  /**
   * @brief 计算词条的BM25分数（单个词条）
   */
  double calculate_term_bm25(const std::string &term, const RID &doc_rid, 
                            int term_freq, int doc_length, double avg_doc_length) const;
};




