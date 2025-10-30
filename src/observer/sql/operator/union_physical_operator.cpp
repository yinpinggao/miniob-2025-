/* Copyright (c) 2024 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Codex on 2024/07/20.
//

#include "sql/operator/union_physical_operator.h"

#include <functional>

#include "common/log/log.h"

using namespace std;

bool UnionPhysicalOperator::TupleDistinctKey::operator==(const TupleDistinctKey &other) const
{
  if (values.size() != other.values.size()) {
    return false;
  }

  for (size_t i = 0; i < values.size(); ++i) {
    if (values[i].compare(other.values[i]) != 0) {
      return false;
    }
  }
  return true;
}

size_t UnionPhysicalOperator::TupleDistinctKeyHash::operator()(const TupleDistinctKey &key) const noexcept
{
  size_t seed = key.values.size();
  for (const Value &value : key.values) {
    size_t component = static_cast<size_t>(value.attr_type());
    if (value.is_null()) {
      component ^= 0x9e3779b97f4a7c15ULL;
    } else {
      switch (value.attr_type()) {
        case AttrType::INTS:
        case AttrType::DATES: component ^= std::hash<int>()(value.get_int()); break;
        case AttrType::FLOATS: component ^= std::hash<float>()(value.get_float()); break;
        case AttrType::BOOLEANS: component ^= std::hash<bool>()(value.get_boolean()); break;
        case AttrType::CHARS:
        case AttrType::TEXTS: component ^= std::hash<std::string>()(value.get_string()); break;
        case AttrType::VECTORS: {
          const int len = value.get_vector_length();
          size_t     vec_hash = std::hash<int>()(len);
          for (int i = 0; i < len; ++i) {
            float element = value.get_vector_element(i);
            vec_hash ^= std::hash<float>()(element) + 0x9e3779b97f4a7c15ULL + (vec_hash << 6) + (vec_hash >> 2);
          }
          component ^= vec_hash;
        } break;
        default: component ^= std::hash<std::string>()(value.to_string()); break;
      }
    }
    seed ^= component + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
  }
  return seed;
}

RC UnionPhysicalOperator::open(Trx *trx)
{
  if (children_.size() != 2) {
    LOG_WARN("union operator expects exactly two children, got %zu", children_.size());
    return RC::INVALID_ARGUMENT;
  }

  current_child_index_ = 0;
  current_tuple_       = nullptr;
  expected_cell_num_   = -1;
  seen_keys_.clear();
  schema_initialized_ = false;
  cached_schema_       = TupleSchema();

  for (size_t idx = 0; idx < children_.size(); ++idx) {
    RC rc = children_[idx]->open(trx);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to open child %zu for union operator. rc=%s", idx, strrc(rc));
      for (size_t j = 0; j < idx; ++j) {
        children_[j]->close();
      }
      return rc;
    }
  }
  return RC::SUCCESS;
}

RC UnionPhysicalOperator::make_key(const Tuple &tuple, TupleDistinctKey &key) const
{
  key.values.clear();
  const int cell_num = tuple.cell_num();
  key.values.reserve(cell_num);
  for (int i = 0; i < cell_num; ++i) {
    Value cell_value;
    RC    rc = tuple.cell_at(i, cell_value);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to fetch cell %d while building union distinct key. rc=%s", i, strrc(rc));
      return rc;
    }
    key.values.emplace_back(std::move(cell_value));
  }
  return RC::SUCCESS;
}

RC UnionPhysicalOperator::fetch_from_child(PhysicalOperator *child)
{
  while (true) {
    RC rc = child->next();
    if (rc == RC::RECORD_EOF) {
      return RC::RECORD_EOF;
    }

    if (OB_FAIL(rc)) {
      return rc;
    }

    Tuple *tuple = child->current_tuple();
    if (tuple == nullptr) {
      LOG_WARN("child returned null tuple in union operator");
      return RC::INTERNAL;
    }

    const int cell_num = tuple->cell_num();
    if (expected_cell_num_ == -1) {
      expected_cell_num_ = cell_num;
    } else if (expected_cell_num_ != cell_num) {
      LOG_WARN("union branch column size mismatch. expected=%d, got=%d", expected_cell_num_, cell_num);
      return RC::SCHEMA_FIELD_MISSING;
    }

    if (union_all_) {
      current_tuple_ = tuple;
      return RC::SUCCESS;
    }

    TupleDistinctKey key;
    rc = make_key(*tuple, key);
    if (OB_FAIL(rc)) {
      return rc;
    }

    auto insert_result = seen_keys_.emplace(std::move(key));
    if (!insert_result.second) {
      // duplicate, continue fetching
      continue;
    }

    current_tuple_ = tuple;
    return RC::SUCCESS;
  }
}

RC UnionPhysicalOperator::next()
{
  while (current_child_index_ < children_.size()) {
    PhysicalOperator *child = children_[current_child_index_].get();
    RC                rc    = fetch_from_child(child);
    if (rc == RC::SUCCESS) {
      return RC::SUCCESS;
    }

    if (rc == RC::RECORD_EOF) {
      ++current_child_index_;
      continue;
    }

    return rc;
  }

  current_tuple_ = nullptr;
  return RC::RECORD_EOF;
}

RC UnionPhysicalOperator::close()
{
  RC rc = RC::SUCCESS;

  for (auto &child : children_) {
    RC child_rc = child->close();
    if (OB_FAIL(child_rc) && OB_SUCC(rc)) {
      rc = child_rc;
    }
  }

  current_child_index_ = 0;
  current_tuple_       = nullptr;
  expected_cell_num_   = -1;
  seen_keys_.clear();
  schema_initialized_ = false;
  cached_schema_       = TupleSchema();

  return rc;
}

RC UnionPhysicalOperator::tuple_schema(TupleSchema &schema) const
{
  if (!schema_initialized_) {
    if (children_.empty()) {
      return RC::SUCCESS;
    }

    TupleSchema child_schema;
    RC          rc = children_.front()->tuple_schema(child_schema);
    if (OB_FAIL(rc)) {
      return rc;
    }
    cached_schema_     = std::move(child_schema);
    schema_initialized_ = true;
  }

  schema = cached_schema_;
  return RC::SUCCESS;
}

