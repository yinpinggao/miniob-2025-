#!/bin/bash

# ===================================================================
# BM25 测试运行脚本
# ===================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BIN_DIR="$PROJECT_DIR/bin"
OBCLIENT="$BIN_DIR/obclient"
PORT=6789

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   BM25 全文索引测试套件${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查 obclient 是否存在
if [ ! -f "$OBCLIENT" ]; then
    echo -e "${RED}错误: 找不到 obclient${NC}"
    echo -e "${YELLOW}请先运行: bash build.sh --make${NC}"
    exit 1
fi

# 检查 observer 是否在运行
if ! pgrep -x "observer" > /dev/null; then
    echo -e "${YELLOW}警告: observer 未运行${NC}"
    echo -e "${YELLOW}请先启动: ./bin/observer -f ./etc/observer.ini${NC}"
    exit 1
fi

# 函数：运行测试
run_test() {
    local test_file=$1
    local test_name=$2
    
    echo ""
    echo -e "${GREEN}>>> 运行: $test_name${NC}"
    echo -e "${BLUE}文件: $test_file${NC}"
    echo "----------------------------------------"
    
    if [ -f "$test_file" ]; then
        if $OBCLIENT -p $PORT < "$test_file"; then
            echo -e "${GREEN}✓ $test_name 完成${NC}"
            return 0
        else
            echo -e "${RED}✗ $test_name 失败${NC}"
            return 1
        fi
    else
        echo -e "${RED}错误: 找不到测试文件 $test_file${NC}"
        return 1
    fi
}

# 显示菜单
show_menu() {
    echo ""
    echo -e "${BLUE}请选择要运行的测试:${NC}"
    echo "1) 快速验证测试 (推荐首选)"
    echo "2) 详细计算验证测试"
    echo "3) 全面测试集"
    echo "4) 运行所有测试"
    echo "5) 退出"
    echo ""
}

# 主循环
while true; do
    show_menu
    read -p "请输入选项 (1-5): " choice
    
    case $choice in
        1)
            run_test "$SCRIPT_DIR/quick_bm25_test.sql" "快速验证测试"
            echo ""
            echo -e "${YELLOW}检查要点:${NC}"
            echo "  - BM25分数不应该是固定值 (如0.693)"
            echo "  - 添加文档后，分数应该变化"
            echo "  - 包含查询词多次的文档分数应该更高"
            ;;
        2)
            run_test "$SCRIPT_DIR/bm25_score_validation.sql" "详细计算验证测试"
            echo ""
            echo -e "${YELLOW}检查要点:${NC}"
            echo "  - 对比手工计算的BM25值"
            echo "  - 验证文档长度归一化"
            echo "  - 验证IDF计算"
            ;;
        3)
            run_test "$SCRIPT_DIR/fulltext_bm25_test.sql" "全面测试集"
            echo ""
            echo -e "${YELLOW}检查要点:${NC}"
            echo "  - 测试各种边界情况"
            echo "  - 验证停用词过滤"
            echo "  - 验证多词查询"
            ;;
        4)
            echo -e "${BLUE}运行所有测试...${NC}"
            failed=0
            
            run_test "$SCRIPT_DIR/quick_bm25_test.sql" "快速验证测试" || ((failed++))
            run_test "$SCRIPT_DIR/bm25_score_validation.sql" "详细计算验证测试" || ((failed++))
            run_test "$SCRIPT_DIR/fulltext_bm25_test.sql" "全面测试集" || ((failed++))
            
            echo ""
            echo "========================================"
            if [ $failed -eq 0 ]; then
                echo -e "${GREEN}✓ 所有测试通过！${NC}"
            else
                echo -e "${RED}✗ $failed 个测试失败${NC}"
            fi
            echo "========================================"
            ;;
        5)
            echo -e "${GREEN}再见！${NC}"
            exit 0
            ;;
        *)
            echo -e "${RED}无效选项，请重新选择${NC}"
            ;;
    esac
    
    echo ""
    read -p "按 Enter 键继续..."
done

