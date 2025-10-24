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

#include "sql/operator/logical_operator.h"

class UnionLogicalOperator : public LogicalOperator
{
public:
  explicit UnionLogicalOperator(bool union_all) : union_all_(union_all) {}

  LogicalOperatorType type() const override { return LogicalOperatorType::UNION; }

  bool union_all() const { return union_all_; }

private:
  bool union_all_ = false;
};

