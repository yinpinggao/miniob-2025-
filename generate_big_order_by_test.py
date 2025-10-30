#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
大数据集ORDER BY测试用例生成器

用法:
    python3 generate_big_order_by_test.py --rows 50 --tables 4 --output my_test.sql
    python3 generate_big_order_by_test.py --rows 100 --tables 3 --output huge_test.sql
"""

import random
import argparse

def generate_test_sql(num_rows_per_table, num_tables, output_file):
    """
    生成big order by测试SQL文件
    
    参数:
        num_rows_per_table: 每个表的记录数
        num_tables: 表的数量 (2-4)
        output_file: 输出文件名
    """
    
    NUM_FIELDS = 20  # 每个表20个字段
    
    with open(output_file, 'w', encoding='utf-8') as f:
        # 写入文件头
        f.write("-- " + "=" * 60 + "\n")
        f.write(f"-- Big Order By 测试用例 (自动生成)\n")
        f.write(f"-- 表数量: {num_tables}\n")
        f.write(f"-- 每表记录数: {num_rows_per_table}\n")
        f.write(f"-- 预计笛卡尔积大小: {num_rows_per_table ** num_tables:,} 条记录\n")
        f.write("-- " + "=" * 60 + "\n\n")
        
        # 第一部分: 创建表
        f.write("-- 第一步: 创建表\n")
        for i in range(num_tables):
            table_name = f"big_order_by_{i}"
            fields = ", ".join([f"i{j} int" for j in range(NUM_FIELDS)])
            f.write(f"CREATE TABLE {table_name}({fields});\n")
        f.write("\n")
        
        # 第二部分: 插入数据
        for table_idx in range(num_tables):
            table_name = f"big_order_by_{table_idx}"
            f.write(f"-- 第二步: 插入数据到 {table_name}\n")
            
            for row_idx in range(num_rows_per_table):
                # 生成随机数据
                values = []
                for field_idx in range(NUM_FIELDS):
                    # 生成不同范围的随机数，确保有一定的重复和差异
                    if field_idx < 5:
                        # 前5个字段：小范围，容易重复
                        value = random.randint(1, 100)
                    elif field_idx < 10:
                        # 中间5个字段：中等范围
                        value = random.randint(100, 10000)
                    else:
                        # 后10个字段：大范围
                        value = random.randint(10000, 200000)
                    values.append(str(value))
                
                values_str = ", ".join(values)
                f.write(f"INSERT INTO {table_name} VALUES ({values_str});\n")
            
            f.write("\n")
        
        # 第三部分: 测试查询
        f.write("-- 第三步: 执行ORDER BY测试\n\n")
        
        # 测试1: 单表排序
        f.write("-- 测试1: 单表排序\n")
        f.write(f"SELECT * FROM big_order_by_0 ORDER BY i0;\n\n")
        
        # 测试2: 单表多列排序
        f.write("-- 测试2: 单表多列排序\n")
        f.write(f"SELECT * FROM big_order_by_0 ORDER BY i0, i5, i10;\n\n")
        
        # 测试3: 双表JOIN
        if num_tables >= 2:
            f.write(f"-- 测试3: 双表JOIN排序 ({num_rows_per_table}x{num_rows_per_table}={num_rows_per_table*num_rows_per_table}条)\n")
            f.write(f"SELECT * FROM big_order_by_0, big_order_by_1 ORDER BY big_order_by_0.i0, big_order_by_1.i5;\n\n")
        
        # 测试4: 三表JOIN
        if num_tables >= 3:
            result_rows = num_rows_per_table ** 3
            f.write(f"-- 测试4: 三表JOIN排序 ({result_rows:,}条)\n")
            f.write(f"SELECT * FROM big_order_by_0, big_order_by_1, big_order_by_2 ORDER BY big_order_by_0.i0, big_order_by_1.i5, big_order_by_2.i3;\n\n")
        
        # 测试5: 四表JOIN (如果有)
        if num_tables >= 4:
            result_rows = num_rows_per_table ** 4
            f.write(f"-- 测试5: 四表JOIN简单排序 ({result_rows:,}条记录)\n")
            f.write(f"SELECT * FROM big_order_by_0, big_order_by_1, big_order_by_2, big_order_by_3 ORDER BY big_order_by_0.i0;\n\n")
            
            f.write(f"-- 测试6: 四表JOIN复杂多列排序\n")
            f.write(f"SELECT * FROM big_order_by_0, big_order_by_1, big_order_by_2, big_order_by_3 ORDER BY big_order_by_3.i19, big_order_by_0.i18, big_order_by_3.i2, big_order_by_1.i5;\n\n")
            
            f.write(f"-- 测试7: 四表JOIN极限多列排序 (复现原始错误)\n")
            f.write(f"SELECT * FROM big_order_by_0, big_order_by_1, big_order_by_2, big_order_by_3 ORDER BY big_order_by_3.i19, big_order_by_0.i18, big_order_by_3.i2, big_order_by_1.i5, big_order_by_0.i0, big_order_by_1.i11, big_order_by_2.i3, big_order_by_1.i18;\n\n")
        
        # 第四部分: 清理
        f.write("-- 第四步: 清理测试表\n")
        for i in range(num_tables):
            table_name = f"big_order_by_{i}"
            f.write(f"DROP TABLE {table_name};\n")
        
        f.write("\n-- " + "=" * 60 + "\n")
        f.write("-- 测试完成\n")
        f.write("-- " + "=" * 60 + "\n")
    
    print(f"✅ 测试文件已生成: {output_file}")
    print(f"   - 表数量: {num_tables}")
    print(f"   - 每表记录数: {num_rows_per_table}")
    if num_tables >= 2:
        cartesian_size = num_rows_per_table ** num_tables
        estimated_size_mb = (cartesian_size * NUM_FIELDS * 4) / (1024 * 1024)
        print(f"   - 最大笛卡尔积: {cartesian_size:,} 条记录")
        print(f"   - 预计内存占用: ~{estimated_size_mb:.2f} MB")
    print(f"\n运行方式:")
    print(f"   正常模式: ./build/bin/observer < {output_file}")
    print(f"   内存限制模式 (32MB): MT_MEMORY_LIMIT=33554432 LD_PRELOAD=./build/lib/libmemtracer.so ./build/bin/observer < {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description='生成Big Order By测试用例',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
示例:
  生成小规模测试 (10条/表, 4个表):
    python3 generate_big_order_by_test.py --rows 10 --tables 4 --output small_test.sql
  
  生成中等规模测试 (20条/表, 4个表, 160,000条结果):
    python3 generate_big_order_by_test.py --rows 20 --tables 4 --output medium_test.sql
  
  生成大规模测试 (50条/表, 4个表, 6,250,000条结果):
    python3 generate_big_order_by_test.py --rows 50 --tables 4 --output large_test.sql
  
  生成超大规模测试 (100条/表, 3个表):
    python3 generate_big_order_by_test.py --rows 100 --tables 3 --output huge_test.sql

内存占用估算:
  - 20x20x20x20 = 160,000条 ≈ 12.8 MB (会触发32MB限制)
  - 30x30x30x30 = 810,000条 ≈ 64.8 MB (必须使用外部排序)
  - 50x50x50x50 = 6,250,000条 ≈ 500 MB (严重压力测试)
        '''
    )
    
    parser.add_argument(
        '--rows',
        type=int,
        default=20,
        help='每个表的记录数 (默认: 20)'
    )
    
    parser.add_argument(
        '--tables',
        type=int,
        choices=[2, 3, 4],
        default=4,
        help='表的数量: 2, 3, 或 4 (默认: 4)'
    )
    
    parser.add_argument(
        '--output',
        type=str,
        default='big_order_by_generated.sql',
        help='输出文件名 (默认: big_order_by_generated.sql)'
    )
    
    parser.add_argument(
        '--seed',
        type=int,
        help='随机数种子 (用于可重现的测试数据)'
    )
    
    args = parser.parse_args()
    
    # 设置随机种子
    if args.seed is not None:
        random.seed(args.seed)
        print(f"🎲 使用随机种子: {args.seed}")
    
    # 生成测试文件
    generate_test_sql(args.rows, args.tables, args.output)


if __name__ == '__main__':
    main()


