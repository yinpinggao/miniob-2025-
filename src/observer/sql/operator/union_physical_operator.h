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

#pragma once

#include <unordered_set>
#include <vector>

#include "sql/operator/physical_operator.h"

class UnionPhysicalOperator : public PhysicalOperator
{
public:
  explicit UnionPhysicalOperator(bool union_all) : union_all_(union_all) {}

  PhysicalOperatorType type() const override { return PhysicalOperatorType::UNION; }
  std::string          name() const override { return "UNION"; }

  RC    open(Trx *trx) override;
  RC    next() override;
  RC    close() override;
  Tuple *current_tuple() override { return current_tuple_; }
  RC    tuple_schema(TupleSchema &schema) const override;

private:
  struct TupleDistinctKey
  {
    std::vector<Value> values;

    bool operator==(const TupleDistinctKey &other) const;
  };

  struct TupleDistinctKeyHash
  {
    size_t operator()(const TupleDistinctKey &key) const noexcept;
  };

  RC fetch_from_child(PhysicalOperator *child);
  RC make_key(const Tuple &tuple, TupleDistinctKey &key) const;

private:
  bool union_all_ = false;

  size_t current_child_index_ = 0;
  Tuple *current_tuple_       = nullptr;

  std::unordered_set<TupleDistinctKey, TupleDistinctKeyHash> seen_keys_;
  mutable TupleSchema                                           cached_schema_;
  mutable bool                                                  schema_initialized_ = false;
  int                                                           expected_cell_num_ = -1;
};

