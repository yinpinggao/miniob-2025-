/***************************************************************
 *                                                             *
 * @Author      : Koschei                                      *
 * @Email       : nitianzero@gmail.com                         *
 * @Date        : 2024/10/18                                     *
 * @Description : Brief description of the file's purpose      *
 *                                                             *
 * Copyright (c) 2024 Koschei                                  *
 * All rights reserved.                                        *
 *                                                             *
 ***************************************************************/

#pragma once

#include "sql/optimizer/rewrite_rule.h"

enum class NormalFunctionType;

/**
 * @brief 向量索引重写规则
 * @ingroup Rewriter
 * @details 识别 orderby limit 重写为向量索引
 */
class VectorIndexScanRewrite : public RewriteRule
{
public:
  VectorIndexScanRewrite()          = default;
  virtual ~VectorIndexScanRewrite() = default;

  /// IVF 返回 L2/COSINE 的较小值、INNER_PRODUCT 的较大值。
  /// 仅当索引顺序与 SQL ORDER BY 方向一致时才能移除排序算子。
  static bool is_order_compatible(NormalFunctionType function_type, bool is_asc);

  RC rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made) override;
};
