#!/bin/bash

# 智能餐厅服务端启动脚本
# 使用方法: ./start.sh [配置文件路径]

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认配置
DEFAULT_CONFIG="config.env"
BUILD_DIR="build"
SERVER_BINARY="bin/WisdomRestaurantServer"

# 获取配置文件路径
CONFIG_FILE="${1:-$DEFAULT_CONFIG}"

echo -e "${BLUE}🍽️  智能餐厅服务端启动脚本${NC}"
echo "=================================="

# 检查配置文件
if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "${YELLOW}配置文件 $CONFIG_FILE 不存在，使用默认配置${NC}"
    echo -e "${YELLOW}建议: 复制 config.env.example 为 config.env 并填入真实配置${NC}"
    echo ""
    
    # 检查环境变量
    if [ -z "$DASHSCOPE_API_KEY" ]; then
        echo -e "${RED}错误: 未设置DASHSCOPE_API_KEY环境变量${NC}"
        echo "请设置API密钥: export DASHSCOPE_API_KEY='your-api-key-here'"
        echo "或创建配置文件: cp config.env.example config.env"
        exit 1
    fi
else
    echo -e "${GREEN}加载配置文件: $CONFIG_FILE${NC}"
    source "$CONFIG_FILE"
fi

# 检查API密钥
if [ -z "$DASHSCOPE_API_KEY" ] || [ "$DASHSCOPE_API_KEY" = "your-api-key-here" ]; then
    echo -e "${RED}错误: 请设置有效的DASHSCOPE_API_KEY${NC}"
    echo "请在配置文件中设置真实的API密钥"
    exit 1
fi

# 设置默认值
export SERVER_PORT="${SERVER_PORT:-8080}"
export DB_PATH="${DB_PATH:-wisdom_restaurant.db}"

# 检查构建目录
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}构建目录不存在，开始编译...${NC}"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    cd ..
fi

# 检查可执行文件
if [ ! -f "$BUILD_DIR/$SERVER_BINARY" ]; then
    echo -e "${YELLOW}可执行文件不存在，开始编译...${NC}"
    cd "$BUILD_DIR"
    make -j$(nproc)
    cd ..
fi

# 显示配置信息
echo -e "${GREEN}配置信息:${NC}"
echo "  API密钥: ${DASHSCOPE_API_KEY:0:10}..."
echo "  服务端口: $SERVER_PORT"
echo "  数据库文件: $DB_PATH"
echo ""

# 启动服务器
echo -e "${GREEN}启动智能餐厅服务端...${NC}"
echo "=================================="

cd "$BUILD_DIR"
exec "./$SERVER_BINARY"
