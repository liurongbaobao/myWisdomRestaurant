#!/bin/bash

# 智能餐厅服务端编译脚本
# 使用方法: ./build.sh [构建类型]

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认构建类型
BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo -e "${BLUE}🍽️  智能餐厅服务端编译脚本${NC}"
echo "=================================="

# 清理旧的构建文件
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}清理旧的构建文件...${NC}"
    rm -rf "$BUILD_DIR/CMakeCache.txt" "$BUILD_DIR/CMakeFiles" "$BUILD_DIR/Makefile" "$BUILD_DIR/cmake_install.cmake"
fi

# 创建构建目录
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}创建构建目录...${NC}"
    mkdir -p "$BUILD_DIR"
fi

# 进入构建目录
cd "$BUILD_DIR"

# 配置CMake
echo -e "${GREEN}配置CMake (构建类型: $BUILD_TYPE)...${NC}"
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# 编译项目
echo -e "${GREEN}开始编译...${NC}"
make -j$(nproc)

echo -e "${GREEN}✅ 编译完成！${NC}"
echo -e "${BLUE}可执行文件位置: $BUILD_DIR/bin/WisdomRestaurantServer${NC}"
echo ""
echo -e "${YELLOW}下一步:${NC}"
echo "1. 设置环境变量: export DASHSCOPE_API_KEY='your-api-key-here'"
echo "2. 启动服务器: ./start.sh"
echo "3. 或直接运行: ./$BUILD_DIR/bin/WisdomRestaurantServer"
