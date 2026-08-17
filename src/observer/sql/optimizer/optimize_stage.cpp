/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Longda on 2021/4/13.
//

#include <string.h>
#include <string>

#include "optimize_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/operator/logical_operator.h"
#include "sql/stmt/stmt.h"

using namespace std;
using namespace common;

RC OptimizeStage::handle_request(SQLStageEvent *sql_event)
{
  // 优化阶段分为三件事：
  // 1. Stmt -> LogicalOperator：描述“要做什么”；
  // 2. Rewrite/Optimize：做等价改写或算法选择；
  // 3. LogicalOperator -> PhysicalOperator：决定“具体怎么做”。
  unique_ptr<LogicalOperator> logical_operator;

  RC rc = create_logical_plan(sql_event, logical_operator);
  if (rc != RC::SUCCESS) {
    if (rc != RC::UNIMPLEMENTED) {
      LOG_WARN("failed to create logical plan. rc=%s", strrc(rc));
    }
    return rc;
  }

  ASSERT(logical_operator, "logical operator is null");

  rc = rewrite(logical_operator);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to rewrite plan. rc=%s", strrc(rc));
    return rc;
  }

  rc = optimize(logical_operator);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to optimize plan. rc=%s", strrc(rc));
    return rc;
  }

  unique_ptr<PhysicalOperator> physical_operator;
  rc = generate_physical_plan(logical_operator, physical_operator, sql_event->session_event()->session());
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to generate physical plan. rc=%s", strrc(rc));
    return rc;
  }

  sql_event->set_operator(std::move(physical_operator));

  return rc;
}

RC OptimizeStage::optimize(unique_ptr<LogicalOperator> &oper)
{
  // 当前没有成熟的基于代价优化器（CBO）。工业数据库通常会结合行数、NDV、
  // 选择率、I/O 成本等统计信息枚举访问路径和 Join Order；MiniOB 主要依赖规则
  // 与物理计划生成器中的启发式选择。
  return RC::SUCCESS;
}

RC OptimizeStage::generate_physical_plan(
    unique_ptr<LogicalOperator> &logical_operator, unique_ptr<PhysicalOperator> &physical_operator, Session *session)
{
  RC rc = RC::SUCCESS;
  // MiniOB 同时保留了逐行 Volcano Iterator 和批量 Chunk Iterator 两条执行路径。
  // 向量化路径一次处理一列/一批数据，可减少虚函数调用并利于 SIMD，但并非所有
  // 逻辑算子都有对应实现，因此不支持时仍回退到 tuple iterator。
  if (session->get_execution_mode() == ExecutionMode::CHUNK_ITERATOR &&
      LogicalOperator::can_generate_vectorized_operator(logical_operator->type())) {
    LOG_INFO("use chunk iterator");
    session->set_used_chunk_mode(true);
    rc = physical_plan_generator_.create_vec(*logical_operator, physical_operator);
  } else {
    LOG_INFO("use tuple iterator");
    session->set_used_chunk_mode(false);
    rc = physical_plan_generator_.create(*logical_operator, physical_operator);
  }
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create physical operator. rc=%s", strrc(rc));
  }
  return rc;
}

RC OptimizeStage::rewrite(unique_ptr<LogicalOperator> &logical_operator)
{
  RC rc = RC::SUCCESS;

  bool change_made = false;
  // 一个规则可能为另一个规则创造新的匹配机会，因此反复执行到不再发生变化。
  // 这要求每条 rewrite rule 都能收敛，否则会在这里形成无限循环。
  do {
    change_made = false;
    rc          = rewriter_.rewrite(logical_operator, change_made);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to do expression rewrite on logical plan. rc=%s", strrc(rc));
      return rc;
    }
  } while (change_made);

  return rc;
}

RC OptimizeStage::create_logical_plan(SQLStageEvent *sql_event, unique_ptr<LogicalOperator> &logical_operator)
{
  Stmt *stmt = sql_event->stmt();
  if (nullptr == stmt) {
    // CREATE TABLE、DROP TABLE 等命令没有关系算子树，返回 UNIMPLEMENTED 后
    // 由 ExecuteStage/CommandExecutor 走直接执行路径。
    return RC::UNIMPLEMENTED;
  }

  return logical_plan_generator_.create(stmt, logical_operator);
}
