/***************************************************************
 *                                                             *
 * @Author      : Koschei                                      *
 * @Email       : nitianzero@gmail.com                         *
 * @Date        : 2024/9/11                                    *
 * @Description : DateType source file                         *
 *                                                             *
 * Copyright (c) 2024 Koschei                                  *
 * All rights reserved.                                        *
 *                                                             *
 ***************************************************************/

#include "date_type.h"

#include <common/value.h>
#include <common/lang/comparator.h>
#include <common/lang/sstream.h>
#include <iomanip>

int DateType::compare(const Value &left, const Value &right) const
{
  // 【赛题 4 date】DATE 使用 YYYYMMDD 的 int32_t 编码，而不是 time_t，避免
  // 1970 起点、2038 溢出和时区问题；整数顺序与日期顺序一致。
  return common::compare_int((void *)&left.value_.int_value_, (void *)&right.value_.int_value_);
}

RC DateType::to_string(const Value &val, string &result) const
{
  // 将内部 YYYYMMDD 编码格式化为 SQL 输出所需的 YYYY-MM-DD。
  // 提取年、月、日
  int year  = val.value_.int_value_ / 10000;        // 获取年份
  int month = (val.value_.int_value_ / 100) % 100;  // 获取月份
  int day   = val.value_.int_value_ % 100;          // 获取日期

  // 使用字符串流构建结果字符串
  std::ostringstream oss;
  oss << std::setw(4) << std::setfill('0') << year << '-' << std::setw(2) << std::setfill('0') << month << '-'
      << std::setw(2) << std::setfill('0') << day;

  // 将结果赋值给 result
  result = oss.str();
  return RC::SUCCESS;
}

int DateType::cast_cost(AttrType type)
{
  if (type == AttrType::DATES)
    return 0;  // DATE -> DATE
  if (type == AttrType::INTS)
    return 0;        // DATE -> INT (需转换)
  return INT32_MAX;  // 不支持转换
}
