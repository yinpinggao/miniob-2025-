# 视图问题测试指南

## 问题描述

### 问题1: 单表视图包含表达式字段时的插入（已修复）
- **场景**: `create view create_view_v5 as select id, age, id+age as data from create_view_t1;`
- **操作**: `insert into create_view_v5 (id, age) values(1, 2);`
- **期望**: FAILURE（因为视图包含表达式字段 `data`）
- **实际**: SUCCESS（修复前）
- **状态**: 已在 `src/observer/sql/stmt/insert_stmt.cpp` 中修复

### 问题2: 视图链式引用查询失败
- **场景**: 
  - `create view create_view_v6 as select id, age, id+age as data from create_view_t1 where id in (select id from create_view_t2);`
  - `create view create_view_v9 as select t2.name, t1.data from create_view_v6 t1, create_view_t1 t2 where t1.id = t2.id;`
- **操作**: `select count(data) from create_view_v9;`
- **期望**: 返回数字（如 225）
- **实际**: FAILURE
- **状态**: 待测试和诊断

## 运行测试

### 步骤1: 重新编译
```bash
cd build_debug
make -j4
```

### 步骤2: 启动服务器
```bash
cd build_debug
./bin/observer -f ../etc/observer.ini
```

### 步骤3: 在另一个终端运行测试
```bash
cd build_debug
./bin/obclient < ../test_view_issues.sql
```

## 预期结果

### 问题1（已修复）
- 测试1.1: `INSERT INTO create_view_v5 (id, age) VALUES(100, 200)` → **FAILURE** ✓
- 测试1.2: `INSERT INTO create_view_v5 VALUES(101, 201, 302)` → **FAILURE** ✓

### 问题2（待验证）
- 测试2.1-2.4: 应该全部成功
- 测试2.5: `SELECT COUNT(data) FROM create_view_v9` → 应该返回数字而不是 FAILURE

## 如果问题2仍然存在

请提供以下信息：
1. observer 日志中的错误信息（`build_debug/observer.log.*`）
2. 具体是哪一步失败了（创建视图 v9？还是查询时？）
3. 完整的错误消息

## 代码修改位置

- `src/observer/sql/stmt/insert_stmt.cpp` 第118-130行
  - 修改了单表视图的插入检查逻辑
  - 现在会检查视图的所有字段，而不仅仅是指定的字段


