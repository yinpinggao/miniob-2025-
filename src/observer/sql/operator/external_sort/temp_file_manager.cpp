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

#include "sql/operator/external_sort/temp_file_manager.h"
#include "common/log/log.h"
#include <unistd.h>
#include <cstdio>
#include <sstream>

int TempFileManager::file_counter_ = 0;

TempFileManager::~TempFileManager() { cleanup_all(); }

std::string TempFileManager::create_temp_file(const std::string &prefix)
{
  std::ostringstream oss;
  oss << "/tmp/" << prefix << "_pid" << getpid() << "_" << (file_counter_++) << ".tmp";

  std::string path = oss.str();
  temp_files_.push_back(path);

  LOG_DEBUG("created temp file: %s", path.c_str());
  return path;
}

void TempFileManager::register_file(const std::string &path) { temp_files_.push_back(path); }

void TempFileManager::cleanup_all()
{
  for (const auto &path : temp_files_) {
    if (std::remove(path.c_str()) == 0) {
      LOG_DEBUG("removed temp file: %s", path.c_str());
    } else {
      LOG_WARN("failed to remove temp file: %s", path.c_str());
    }
  }
  temp_files_.clear();
}

