/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <algorithm>

#include <gtest/gtest.h>
#include <json/value.h>

#include "storage/index/ivfflat_index.h"

TEST(IndexMeta, vector_configuration_round_trip)
{
  FieldMeta field("v", AttrType::VECTORS, 0, 2 * sizeof(float), true, 0, false);
  IndexMeta meta;
  ASSERT_EQ(RC::SUCCESS,
      meta.init_vector("idx", IndexType::VectorIVFFlatIndex, {field}, NormalFunctionType::COSINE_DISTANCE, 7, 3));

  Json::Value json;
  meta.to_json(json);

  IndexMeta restored;
  ASSERT_EQ(RC::SUCCESS, IndexMeta::from_json(json, restored));
  EXPECT_EQ(IndexType::VectorIVFFlatIndex, restored.index_type());
  EXPECT_EQ(NormalFunctionType::COSINE_DISTANCE, restored.vector_distance_type());
  EXPECT_EQ(7, restored.vector_lists());
  EXPECT_EQ(3, restored.vector_probes());
}

TEST(IvfflatIndex, clamps_probes_and_deletes_entries)
{
  FieldMeta field("v", AttrType::VECTORS, 0, 2 * sizeof(float), true, 0, false);
  IndexMeta meta;
  ASSERT_EQ(RC::SUCCESS,
      meta.init_vector("idx", IndexType::VectorIVFFlatIndex, {field}, NormalFunctionType::L2_DISTANCE, 1, 5));

  IvfflatIndex index;
  ASSERT_EQ(RC::SUCCESS, index.create(nullptr, "", meta, field));

  std::vector<std::pair<std::vector<float>, RID>> data = {
      {{1.0F, 0.0F}, RID(1, 1)}, {{0.0F, 1.0F}, RID(1, 2)}, {{2.0F, 0.0F}, RID(1, 3)}};
  ASSERT_EQ(RC::SUCCESS, index.build_index(data, NormalFunctionType::L2_DISTANCE, {1, 5}));

  auto result = index.ann_search({1.0F, 0.0F}, 3);
  ASSERT_EQ(3U, result.size());
  EXPECT_EQ(RID(1, 1), result[0]);

  RID deleted(1, 1);
  ASSERT_EQ(RC::SUCCESS, index.delete_entry(nullptr, &deleted));
  result = index.ann_search({1.0F, 0.0F}, 3);
  EXPECT_EQ(result.end(), std::find(result.begin(), result.end(), deleted));
}

TEST(IvfflatIndex, inner_product_orders_larger_scores_first)
{
  FieldMeta field("v", AttrType::VECTORS, 0, 2 * sizeof(float), true, 0, false);
  IndexMeta meta;
  ASSERT_EQ(RC::SUCCESS,
      meta.init_vector("idx", IndexType::VectorIVFFlatIndex, {field}, NormalFunctionType::INNER_PRODUCT, 1, 1));

  IvfflatIndex index;
  ASSERT_EQ(RC::SUCCESS, index.create(nullptr, "", meta, field));

  std::vector<std::pair<std::vector<float>, RID>> data = {
      {{1.0F, 0.0F}, RID(1, 1)}, {{3.0F, 0.0F}, RID(1, 2)}, {{2.0F, 0.0F}, RID(1, 3)}};
  ASSERT_EQ(RC::SUCCESS, index.build_index(data, NormalFunctionType::INNER_PRODUCT, {1, 1}));

  auto result = index.ann_search({1.0F, 0.0F}, 3);
  ASSERT_EQ(3U, result.size());
  EXPECT_EQ(RID(1, 2), result[0]);
  EXPECT_EQ(RID(1, 3), result[1]);
  EXPECT_EQ(RID(1, 1), result[2]);
}
