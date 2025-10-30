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

#include <string>
#include <vector>

/**
 * @brief 临时文件管理器，用于创建和清理临时文件
 */
class TempFileManager
{
public:
  TempFileManager() = default;
  ~TempFileManager();

  /**
   * @brief 创建一个临时文件
   * @param prefix 文件名前缀
   * @return string 临时文件路径
   */
  std::string create_temp_file(const std::string &prefix = "miniob_sort");

  /**
   * @brief 注册一个临时文件用于清理
   * @param path 文件路径
   */
  void register_file(const std::string &path);

  /**
   * @brief 清理所有临时文件
   */
  void cleanup_all();

  /**
   * @brief 获取所有临时文件路径
   */
  const std::vector<std::string> &get_files() const { return temp_files_; }

private:
  std::vector<std::string> temp_files_;
  static int               file_counter_;
};

