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
// Created for external sort
//

#pragma once

#include "sql/expr/tuple.h"
#include "common/rc.h"
#include <iostream>
#include <vector>

/**
 * @brief Tuple序列化器，用于将Tuple写入磁盘和从磁盘读取
 * @details 用于外部排序时将Tuple存储到临时文件
 */
class TupleSerializer
{
public:
  /**
   * @brief 序列化Tuple到输出流
   * @param os 输出流
   * @param tuple 要序列化的tuple
   * @return RC 操作结果
   */
  static RC serialize(std::ostream &os, const Tuple *tuple);

  /**
   * @brief 从输入流反序列化Tuple
   * @param is 输入流
   * @param tuple 输出的tuple（调用者负责释放）
   * @return RC 操作结果
   */
  static RC deserialize(std::istream &is, Tuple *&tuple);

  /**
   * @brief 估算Tuple序列化后的大小
   * @param tuple 要估算的tuple
   * @return size_t 估算的字节数
   */
  static size_t estimate_size(const Tuple *tuple);

private:
  /**
   * @brief 序列化单个Value
   */
  static RC serialize_value(std::ostream &os, const Value &value);

  /**
   * @brief 反序列化单个Value
   */
  static RC deserialize_value(std::istream &is, Value &value);
};

