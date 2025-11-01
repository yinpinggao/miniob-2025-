#!/bin/bash

# 全文索引功能测试脚本

# 设置颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OBCLIENT="$PROJECT_ROOT/build/bin/obclient"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  全文索引功能测试${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# 检查obclient是否存在
if [ ! -f "$OBCLIENT" ]; then
    echo -e "${RED}错误: obclient 不存在: $OBCLIENT${NC}"
    echo "请先编译项目: ./build.sh release"
    exit 1
fi

# 检查observer是否在运行
if ! pgrep -f "./build/bin/observer" > /dev/null; then
    echo -e "${YELLOW}警告: observer 进程未运行${NC}"
    echo "请先启动 observer:"
    echo "  ./build/bin/observer -f ./etc/observer.ini -n \$((50*1024*1024)) &"
    exit 1
fi

echo -e "${GREEN}开始运行测试用例...${NC}"
echo ""

# 运行简单测试用例
echo -e "${YELLOW}[测试] 运行简单测试用例...${NC}"
$OBCLIENT < "$SCRIPT_DIR/fulltext_test_simple.sql"
echo ""

# 运行完整测试用例
echo -e "${YELLOW}[测试] 运行完整测试用例...${NC}"
$OBCLIENT < "$SCRIPT_DIR/fulltext_test.sql"
echo ""

echo -e "${GREEN}测试完成！${NC}"


