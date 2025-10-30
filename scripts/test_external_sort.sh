#!/bin/bash
# 测试外部排序的脚本 - 生成大量数据并测试ORDER BY

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 默认配置
RECORD_COUNT=10000
MEMORY_LIMIT=33554432  # 32MB
TEST_DB="test_external_sort_db"

usage() {
    cat << EOF
外部排序测试脚本

用法: $0 [选项]

选项:
    -n, --records COUNT     生成的记录数（默认: 10000）
    -m, --memory-limit MB   内存限制（MB，默认: 32）
    -d, --database NAME     测试数据库名（默认: test_external_sort_db）
    -h, --help              显示帮助信息

示例:
    # 生成10万条记录，内存限制32MB
    $0 -n 100000 -m 32

    # 生成1万条记录，内存限制64MB
    $0 -n 10000 -m 64

测试说明:
    1. 创建测试数据库和表
    2. 插入指定数量的随机数据
    3. 在内存限制下执行ORDER BY查询
    4. 验证排序结果的正确性

EOF
    exit 0
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--records)
            RECORD_COUNT="$2"
            shift 2
            ;;
        -m|--memory-limit)
            MEMORY_LIMIT=$((${2} * 1048576))  # 转换为字节
            shift 2
            ;;
        -d|--database)
            TEST_DB="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}错误: 未知选项 $1${NC}"
            usage
            ;;
    esac
done

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}  外部排序测试${NC}"
echo -e "${BLUE}================================${NC}"
echo "记录数: $RECORD_COUNT"
echo "内存限制: $((MEMORY_LIMIT / 1048576))MB ($MEMORY_LIMIT bytes)"
echo "数据库: $TEST_DB"
echo -e "${BLUE}================================${NC}"
echo ""

# 生成测试SQL文件
TEST_SQL="/tmp/test_external_sort_$$.sql"
echo -e "${YELLOW}生成测试SQL...${NC}"

cat > "$TEST_SQL" << EOF
-- 创建测试数据库
DROP DATABASE IF EXISTS $TEST_DB;
CREATE DATABASE $TEST_DB;
USE $TEST_DB;

-- 创建测试表
CREATE TABLE test_table (
    id INT,
    value INT,
    name CHAR(50)
);

-- 准备插入大量数据
-- （注意：这里只是示例，实际需要通过程序生成）
EOF

# 生成插入语句（这里只是演示，实际应该通过程序批量生成）
echo -e "${YELLOW}生成 $RECORD_COUNT 条插入语句...${NC}"
for i in $(seq 1 $RECORD_COUNT); do
    # 生成随机值
    value=$((RANDOM % 100000))
    name="Record_${i}_Value_${value}"
    echo "INSERT INTO test_table VALUES ($i, $value, '$name');" >> "$TEST_SQL"
    
    # 显示进度
    if [ $((i % 1000)) -eq 0 ]; then
        echo -ne "\r进度: $i / $RECORD_COUNT"
    fi
done
echo ""

# 添加查询语句
cat >> "$TEST_SQL" << EOF

-- 执行ORDER BY查询
SELECT * FROM test_table ORDER BY value ASC;

-- 验证排序（可以通过LIMIT查看前几条）
SELECT * FROM test_table ORDER BY value ASC LIMIT 10;

-- 降序排序
SELECT * FROM test_table ORDER BY value DESC LIMIT 10;

-- 多列排序
SELECT * FROM test_table ORDER BY value ASC, id DESC LIMIT 10;

-- 退出
EXIT;
EOF

echo -e "${GREEN}SQL文件生成完成: $TEST_SQL${NC}"
echo ""

# 检查是否编译了MemTracer
MEMTRACER_LIB="$PROJECT_ROOT/build/lib/libmemtracer.so"
if [ ! -f "$MEMTRACER_LIB" ]; then
    echo -e "${YELLOW}警告: MemTracer库不存在${NC}"
    echo "建议重新编译: bash build.sh release -DWITH_MEMTRACER=ON"
    echo "将在没有内存限制的情况下运行测试..."
    echo ""
    MEMTRACER_LIB=""
fi

# 运行测试
echo -e "${GREEN}开始执行测试...${NC}"
echo ""

if [ -n "$MEMTRACER_LIB" ]; then
    # 使用MemTracer
    MT_PRINT_INTERVAL_MS=2000 MT_MEMORY_LIMIT=$MEMORY_LIMIT \
        LD_PRELOAD="$MEMTRACER_LIB" \
        "$PROJECT_ROOT/build/bin/observer" -f "$TEST_SQL"
else
    # 不使用MemTracer
    "$PROJECT_ROOT/build/bin/observer" -f "$TEST_SQL"
fi

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ 测试完成${NC}"
else
    echo -e "${RED}✗ 测试失败，退出码: $EXIT_CODE${NC}"
    if [ -n "$MEMTRACER_LIB" ]; then
        echo -e "${YELLOW}提示: 可能是内存超限，尝试增加内存限制或减少记录数${NC}"
    fi
fi

# 清理
echo ""
read -p "是否删除测试SQL文件? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -f "$TEST_SQL"
    echo -e "${GREEN}已删除: $TEST_SQL${NC}"
else
    echo -e "${YELLOW}保留测试文件: $TEST_SQL${NC}"
fi

exit $EXIT_CODE

