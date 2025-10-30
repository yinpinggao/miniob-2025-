#!/bin/bash
# 使用MemTracer运行observer的便捷脚本

# 设置脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 默认值
MEMORY_LIMIT=""
PRINT_INTERVAL="5000"
BUILD_DIR="$PROJECT_ROOT/build"
OBSERVER_BIN="$BUILD_DIR/bin/observer"
MEMTRACER_LIB="$BUILD_DIR/lib/libmemtracer.so"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 使用说明
usage() {
    cat << EOF
使用说明: $0 [选项]

选项:
    -m, --memory-limit BYTES    设置内存限制（字节）
                                示例: -m 104857600  (100MB)
                                     -m 33554432   (32MB)
    -i, --interval MS           设置内存打印间隔（毫秒，默认5000）
    -b, --build-dir DIR         指定构建目录（默认: build）
    -h, --help                  显示此帮助信息

示例:
    # 设置100MB内存限制，每1秒打印一次
    $0 -m 104857600 -i 1000

    # 设置32MB内存限制
    $0 -m 33554432

    # 只监控内存，不设置限制
    $0 -i 1000

常用内存大小:
    32MB  = 33554432 bytes
    64MB  = 67108864 bytes
    100MB = 104857600 bytes
    128MB = 134217728 bytes
    256MB = 268435456 bytes
    512MB = 536870912 bytes
    1GB   = 1073741824 bytes

EOF
    exit 0
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -m|--memory-limit)
            MEMORY_LIMIT="$2"
            shift 2
            ;;
        -i|--interval)
            PRINT_INTERVAL="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            OBSERVER_BIN="$BUILD_DIR/bin/observer"
            MEMTRACER_LIB="$BUILD_DIR/lib/libmemtracer.so"
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

# 检查observer是否存在
if [ ! -f "$OBSERVER_BIN" ]; then
    echo -e "${RED}错误: observer不存在: $OBSERVER_BIN${NC}"
    echo "请先编译项目: bash build.sh release"
    exit 1
fi

# 检查memtracer库是否存在
if [ ! -f "$MEMTRACER_LIB" ]; then
    echo -e "${RED}错误: libmemtracer.so不存在: $MEMTRACER_LIB${NC}"
    echo "请使用以下命令重新编译:"
    echo "  bash build.sh release -DWITH_MEMTRACER=ON"
    exit 1
fi

# 构建环境变量
ENV_VARS="MT_PRINT_INTERVAL_MS=$PRINT_INTERVAL"
if [ -n "$MEMORY_LIMIT" ]; then
    ENV_VARS="$ENV_VARS MT_MEMORY_LIMIT=$MEMORY_LIMIT"
fi

# 打印配置信息
echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}  MemTracer 配置${NC}"
echo -e "${GREEN}================================${NC}"
echo "Observer路径: $OBSERVER_BIN"
echo "MemTracer库: $MEMTRACER_LIB"
echo "打印间隔: ${PRINT_INTERVAL}ms"
if [ -n "$MEMORY_LIMIT" ]; then
    # 转换为MB显示
    MEMORY_MB=$((MEMORY_LIMIT / 1048576))
    echo -e "内存限制: ${YELLOW}${MEMORY_LIMIT} bytes (~${MEMORY_MB}MB)${NC}"
else
    echo "内存限制: 未设置"
fi
echo -e "${GREEN}================================${NC}"
echo ""

# 运行observer
echo "启动observer..."
echo "命令: $ENV_VARS LD_PRELOAD=$MEMTRACER_LIB $OBSERVER_BIN"
echo ""

# 执行
env $ENV_VARS LD_PRELOAD="$MEMTRACER_LIB" "$OBSERVER_BIN"

EXIT_CODE=$?
echo ""
if [ $EXIT_CODE -ne 0 ]; then
    echo -e "${RED}observer退出，退出码: $EXIT_CODE${NC}"
    if [ -n "$MEMORY_LIMIT" ]; then
        echo -e "${YELLOW}提示: 可能是内存超限导致退出，检查日志中的 'Memory limit exceeded'${NC}"
    fi
else
    echo -e "${GREEN}observer正常退出${NC}"
fi

exit $EXIT_CODE

