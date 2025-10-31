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

#include "sql/operator/external_sort/tuple_serializer.h"
#include "common/log/log.h"
#include "sql/expr/tuple.h"
#include <cstdint>
#include <cstring>

RC TupleSerializer::serialize(std::ostream &os, const Tuple *tuple)
{
  if (tuple == nullptr) {
    return RC::INVALID_ARGUMENT;
  }

  // 写入cell数量
  int cell_num = tuple->cell_num();
  os.write(reinterpret_cast<const char *>(&cell_num), sizeof(cell_num));

  // 写入specs（写入table_name, field_name, alias）
  for (int i = 0; i < cell_num; i++) {
    TupleCellSpec spec;
    RC            rc = tuple->spec_at(i, spec);
    if (rc != RC::SUCCESS) {
      // 如果获取spec失败，写入空字符串
      int len = 0;
      os.write(reinterpret_cast<const char *>(&len), sizeof(len));  // table_name
      os.write(reinterpret_cast<const char *>(&len), sizeof(len));  // field_name
      os.write(reinterpret_cast<const char *>(&len), sizeof(len));  // alias
    } else {
      // 序列化spec的table_name, field_name, alias
      std::string table_name = spec.table_name();
      std::string field_name = spec.field_name();
      std::string alias      = spec.alias();

      // 写入table_name
      int len = table_name.length();
      os.write(reinterpret_cast<const char *>(&len), sizeof(len));
      if (len > 0) {
        os.write(table_name.c_str(), len);
      }

      // 写入field_name
      len = field_name.length();
      os.write(reinterpret_cast<const char *>(&len), sizeof(len));
      if (len > 0) {
        os.write(field_name.c_str(), len);
      }

      // 写入alias
      len = alias.length();
      os.write(reinterpret_cast<const char *>(&len), sizeof(len));
      if (len > 0) {
        os.write(alias.c_str(), len);
      }
    }
  }

  // 依次写入每个cell
  for (int i = 0; i < cell_num; i++) {
    Value value;
    RC    rc = tuple->cell_at(i, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get cell at %d", i);
      return rc;
    }

    rc = serialize_value(os, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to serialize value at %d", i);
      return rc;
    }
  }

  const auto *value_list_tuple = dynamic_cast<const ValueListTuple *>(tuple);
  uint8_t     has_order_keys   = (value_list_tuple != nullptr && value_list_tuple->has_order_keys()) ? 1 : 0;
  os.write(reinterpret_cast<const char *>(&has_order_keys), sizeof(has_order_keys));
  if (has_order_keys) {
    size_t key_count = value_list_tuple->order_keys().size();
    os.write(reinterpret_cast<const char *>(&key_count), sizeof(key_count));
    for (const Value &key : value_list_tuple->order_keys()) {
      RC rc = serialize_value(os, key);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to serialize order key");
        return rc;
      }
    }
  }

  if (!os.good()) {
    LOG_WARN("stream is not good after serialization");
    return RC::IOERR_WRITE;
  }

  return RC::SUCCESS;
}

RC TupleSerializer::deserialize(std::istream &is, Tuple *&tuple)
{
  // 先检查是否EOF
  if (is.eof()) {
    return RC::RECORD_EOF;
  }
  
  // 读取cell数量
  int cell_num = 0;
  is.read(reinterpret_cast<char *>(&cell_num), sizeof(cell_num));
  if (!is.good()) {
    // 可能是EOF或其他读取错误
    if (is.eof()) {
      return RC::RECORD_EOF;
    }
    LOG_WARN("failed to read cell_num from stream");
    return RC::IOERR_READ;
  }
  
  if (cell_num < 0 || cell_num > 10000) {  // 合理性检查
    LOG_WARN("invalid cell_num: %d", cell_num);
    return RC::IOERR_READ;
  }

  // 读取specs
  std::vector<TupleCellSpec> specs;
  specs.reserve(cell_num);
  for (int i = 0; i < cell_num; i++) {
    // 读取table_name
    int len = 0;
    is.read(reinterpret_cast<char *>(&len), sizeof(len));
    if (!is.good() || len < 0 || len > 1000) {
      LOG_WARN("failed to read table_name length or invalid length: %d", len);
      return RC::IOERR_READ;
    }
    std::string table_name;
    if (len > 0) {
      std::vector<char> buf(len + 1);
      is.read(buf.data(), len);
      buf[len]   = '\0';
      table_name = std::string(buf.data(), len);
    }

    // 读取field_name
    is.read(reinterpret_cast<char *>(&len), sizeof(len));
    if (!is.good() || len < 0 || len > 1000) {
      LOG_WARN("failed to read field_name length or invalid length: %d", len);
      return RC::IOERR_READ;
    }
    std::string field_name;
    if (len > 0) {
      std::vector<char> buf(len + 1);
      is.read(buf.data(), len);
      buf[len]   = '\0';
      field_name = std::string(buf.data(), len);
    }

    // 读取alias
    is.read(reinterpret_cast<char *>(&len), sizeof(len));
    if (!is.good() || len < 0 || len > 1000) {
      LOG_WARN("failed to read alias length or invalid length: %d", len);
      return RC::IOERR_READ;
    }
    std::string alias;
    if (len > 0) {
      std::vector<char> buf(len + 1);
      is.read(buf.data(), len);
      buf[len] = '\0';
      alias    = std::string(buf.data(), len);
    }

    specs.push_back(TupleCellSpec(table_name.c_str(), field_name.c_str(), alias.empty() ? nullptr : alias.c_str()));
  }

  // 创建ValueListTuple来存储反序列化的数据
  std::vector<Value> values;
  values.reserve(cell_num);

  // 依次读取每个cell
  for (int i = 0; i < cell_num; i++) {
    Value value;
    RC    rc = deserialize_value(is, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to deserialize value at %d", i);
      return rc;
    }
    values.push_back(std::move(value));
  }

  // 创建ValueListTuple
  ValueListTuple *value_list_tuple = new ValueListTuple();
  value_list_tuple->set_cells(values);
  value_list_tuple->set_names(specs);

  uint8_t has_order_keys_flag = 0;
  is.read(reinterpret_cast<char *>(&has_order_keys_flag), sizeof(has_order_keys_flag));
  if (!is.good()) {
    LOG_WARN("failed to read order key flag");
    delete value_list_tuple;
    return RC::IOERR_READ;
  }

  if (has_order_keys_flag != 0) {
    size_t key_count = 0;
    is.read(reinterpret_cast<char *>(&key_count), sizeof(key_count));
    if (!is.good() || key_count > 10000) {
      LOG_WARN("failed to read order key count: %zu", key_count);
      delete value_list_tuple;
      return RC::IOERR_READ;
    }
    std::vector<Value> order_keys;
    order_keys.reserve(key_count);
    for (size_t i = 0; i < key_count; i++) {
      Value key;
      RC    rc = deserialize_value(is, key);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to deserialize order key %zu", i);
        delete value_list_tuple;
        return rc;
      }
      order_keys.push_back(std::move(key));
    }
    value_list_tuple->set_order_keys(std::move(order_keys));
  }

  tuple = value_list_tuple;
  return RC::SUCCESS;
}

RC TupleSerializer::deserialize_with_cached_specs(std::istream &is, Tuple *&tuple,
                                                   std::vector<TupleCellSpec> &cached_specs,
                                                   bool &specs_cached)
{
  // 读取cell数量
  int cell_num = 0;
  is.read(reinterpret_cast<char *>(&cell_num), sizeof(cell_num));
  if (!is.good() || cell_num < 0 || cell_num > 10000) {
    LOG_WARN("failed to read cell_num or invalid cell_num: %d", cell_num);
    return RC::IOERR_READ;
  }

  // 如果specs未缓存，读取并缓存
  if (!specs_cached) {
    cached_specs.clear();
    cached_specs.reserve(cell_num);
    
    for (int i = 0; i < cell_num; i++) {
      // 读取table_name
      int len = 0;
      is.read(reinterpret_cast<char *>(&len), sizeof(len));
      if (!is.good() || len < 0 || len > 1000) {
        LOG_WARN("failed to read table_name length or invalid length: %d", len);
        return RC::IOERR_READ;
      }
      std::string table_name;
      if (len > 0) {
        std::vector<char> buf(len + 1);
        is.read(buf.data(), len);
        buf[len]   = '\0';
        table_name = std::string(buf.data(), len);
      }

      // 读取field_name
      is.read(reinterpret_cast<char *>(&len), sizeof(len));
      if (!is.good() || len < 0 || len > 1000) {
        LOG_WARN("failed to read field_name length or invalid length: %d", len);
        return RC::IOERR_READ;
      }
      std::string field_name;
      if (len > 0) {
        std::vector<char> buf(len + 1);
        is.read(buf.data(), len);
        buf[len]   = '\0';
        field_name = std::string(buf.data(), len);
      }

      // 读取alias
      is.read(reinterpret_cast<char *>(&len), sizeof(len));
      if (!is.good() || len < 0 || len > 1000) {
        LOG_WARN("failed to read alias length or invalid length: %d", len);
        return RC::IOERR_READ;
      }
      std::string alias;
      if (len > 0) {
        std::vector<char> buf(len + 1);
        is.read(buf.data(), len);
        buf[len] = '\0';
        alias    = std::string(buf.data(), len);
      }

      cached_specs.push_back(TupleCellSpec(table_name.c_str(), field_name.c_str(), alias.empty() ? nullptr : alias.c_str()));
    }
    
    specs_cached = true;
    LOG_DEBUG("cached %d TupleCellSpecs for RunReader", cell_num);
  } else {
    // specs已缓存，跳过读取specs部分
    for (int i = 0; i < cell_num; i++) {
      // 跳过table_name
      int len = 0;
      is.read(reinterpret_cast<char *>(&len), sizeof(len));
      if (!is.good() || len < 0 || len > 1000) {
        LOG_WARN("failed to skip table_name length");
        return RC::IOERR_READ;
      }
      if (len > 0) {
        is.seekg(len, std::ios::cur);  // 跳过
      }

      // 跳过field_name
      is.read(reinterpret_cast<char *>(&len), sizeof(len));
      if (!is.good() || len < 0 || len > 1000) {
        LOG_WARN("failed to skip field_name length");
        return RC::IOERR_READ;
      }
      if (len > 0) {
        is.seekg(len, std::ios::cur);  // 跳过
      }

      // 跳过alias
      is.read(reinterpret_cast<char *>(&len), sizeof(len));
      if (!is.good() || len < 0 || len > 1000) {
        LOG_WARN("failed to skip alias length");
        return RC::IOERR_READ;
      }
      if (len > 0) {
        is.seekg(len, std::ios::cur);  // 跳过
      }
    }
  }

  // 读取values（每次都要读取）
  std::vector<Value> values;
  values.reserve(cell_num);
  for (int i = 0; i < cell_num; i++) {
    Value value;
    RC    rc = deserialize_value(is, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to deserialize value at %d", i);
      return rc;
    }
    values.push_back(std::move(value));
  }

  uint8_t has_order_keys_flag = 0;
  is.read(reinterpret_cast<char *>(&has_order_keys_flag), sizeof(has_order_keys_flag));
  if (!is.good()) {
    LOG_WARN("failed to read order key flag");
    return RC::IOERR_READ;
  }

  std::vector<Value> order_keys;
  if (has_order_keys_flag != 0) {
    size_t key_count = 0;
    is.read(reinterpret_cast<char *>(&key_count), sizeof(key_count));
    if (!is.good() || key_count > 10000) {
      LOG_WARN("failed to read order key count: %zu", key_count);
      return RC::IOERR_READ;
    }
    order_keys.reserve(key_count);
    for (size_t i = 0; i < key_count; i++) {
      Value key;
      RC    rc = deserialize_value(is, key);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to deserialize order key %zu", i);
        return rc;
      }
      order_keys.push_back(std::move(key));
    }
  }

  // 创建ValueListTuple，使用缓存的specs
  ValueListTuple *value_list_tuple = new ValueListTuple();
  value_list_tuple->set_cells(values);
  value_list_tuple->set_names(cached_specs);  // 使用缓存的specs
  if (!order_keys.empty()) {
    value_list_tuple->set_order_keys(std::move(order_keys));
  }

  tuple = value_list_tuple;
  return RC::SUCCESS;
}

size_t TupleSerializer::estimate_size(const Tuple *tuple)
{
  if (tuple == nullptr) {
    return 0;
  }

  size_t size = sizeof(int);  // cell_num

  int cell_num = tuple->cell_num();
  for (int i = 0; i < cell_num; i++) {
    Value value;
    if (tuple->cell_at(i, value) != RC::SUCCESS) {
      continue;
    }

    // AttrType + length + is_null
    size += sizeof(AttrType) + sizeof(int) + sizeof(bool);

    // 数据部分
    if (!value.is_null()) {
      size += value.length();
    }
  }

  const auto *value_list_tuple = dynamic_cast<const ValueListTuple *>(tuple);
  if (value_list_tuple != nullptr && value_list_tuple->has_order_keys()) {
    size += sizeof(uint8_t);  // flag
    size += sizeof(size_t);   // count
    for (const Value &key : value_list_tuple->order_keys()) {
      size += sizeof(AttrType) + sizeof(int) + sizeof(bool);
      if (!key.is_null()) {
        size += key.length();
      }
    }
  } else {
    size += sizeof(uint8_t);  // flag
  }

  return size;
}

RC TupleSerializer::serialize_value(std::ostream &os, const Value &value)
{
  // 写入类型
  AttrType attr_type = value.attr_type();
  os.write(reinterpret_cast<const char *>(&attr_type), sizeof(attr_type));

  // 写入is_null标志
  bool is_null = value.is_null();
  os.write(reinterpret_cast<const char *>(&is_null), sizeof(is_null));

  if (is_null) {
    // NULL值不需要写入数据
    return RC::SUCCESS;
  }

  // 写入长度
  int length = value.length();
  os.write(reinterpret_cast<const char *>(&length), sizeof(length));

  // 写入数据
  switch (attr_type) {
    case AttrType::INTS: {
      int int_val = value.get_int();
      os.write(reinterpret_cast<const char *>(&int_val), sizeof(int_val));
    } break;

    case AttrType::FLOATS: {
      float float_val = value.get_float();
      os.write(reinterpret_cast<const char *>(&float_val), sizeof(float_val));
    } break;

    case AttrType::BOOLEANS: {
      bool bool_val = value.get_boolean();
      os.write(reinterpret_cast<const char *>(&bool_val), sizeof(bool_val));
    } break;

    case AttrType::CHARS: {
      string str_val = value.get_string();
      os.write(str_val.c_str(), length);
    } break;

    case AttrType::DATES: {
      int date_val = value.get_date();
      os.write(reinterpret_cast<const char *>(&date_val), sizeof(date_val));
    } break;

    case AttrType::VECTORS: {
      // 向量类型：先写向量长度，再写每个元素
      int vec_length = value.get_vector_length();
      os.write(reinterpret_cast<const char *>(&vec_length), sizeof(vec_length));
      std::vector<float> vec = value.get_vector();
      for (int i = 0; i < vec_length; i++) {
        float elem = vec[i];
        os.write(reinterpret_cast<const char *>(&elem), sizeof(elem));
      }
    } break;

    default:
      LOG_WARN("unsupported attr type: %d", static_cast<int>(attr_type));
      return RC::UNSUPPORTED;
  }

  if (!os.good()) {
    LOG_WARN("stream is not good after writing value");
    return RC::IOERR_WRITE;
  }

  return RC::SUCCESS;
}

RC TupleSerializer::deserialize_value(std::istream &is, Value &value)
{
  // 读取类型
  AttrType attr_type;
  is.read(reinterpret_cast<char *>(&attr_type), sizeof(attr_type));

  // 读取is_null标志
  bool is_null;
  is.read(reinterpret_cast<char *>(&is_null), sizeof(is_null));

  if (is_null) {
    value = Value(NullValue());
    return RC::SUCCESS;
  }

  // 读取长度
  int length;
  is.read(reinterpret_cast<char *>(&length), sizeof(length));

  if (!is.good()) {
    LOG_WARN("failed to read value header");
    return RC::IOERR_READ;
  }

  // 读取数据
  switch (attr_type) {
    case AttrType::INTS: {
      int int_val;
      is.read(reinterpret_cast<char *>(&int_val), sizeof(int_val));
      value = Value(int_val);
    } break;

    case AttrType::FLOATS: {
      float float_val;
      is.read(reinterpret_cast<char *>(&float_val), sizeof(float_val));
      value = Value(float_val);
    } break;

    case AttrType::BOOLEANS: {
      bool bool_val;
      is.read(reinterpret_cast<char *>(&bool_val), sizeof(bool_val));
      value = Value(bool_val);
    } break;

    case AttrType::CHARS: {
      std::vector<char> str_buf(length + 1);
      is.read(str_buf.data(), length);
      str_buf[length] = '\0';
      value           = Value(str_buf.data(), length);
    } break;

    case AttrType::DATES: {
      int date_val;
      is.read(reinterpret_cast<char *>(&date_val), sizeof(date_val));
      // Date需要特殊处理，暂时存为int
      value = Value(AttrType::DATES, reinterpret_cast<char *>(&date_val), sizeof(date_val));
    } break;

    case AttrType::VECTORS: {
      int vec_length;
      is.read(reinterpret_cast<char *>(&vec_length), sizeof(vec_length));
      std::vector<float> vec;
      vec.reserve(vec_length);
      for (int i = 0; i < vec_length; i++) {
        float elem;
        is.read(reinterpret_cast<char *>(&elem), sizeof(elem));
        vec.push_back(elem);
      }
      value = Value(vec);
    } break;

    default:
      LOG_WARN("unsupported attr type: %d", static_cast<int>(attr_type));
      return RC::UNSUPPORTED;
  }

  if (!is.good()) {
    LOG_WARN("failed to read value data");
    return RC::IOERR_READ;
  }

  return RC::SUCCESS;
}
