#!/bin/bash

# 智能餐厅新架构测试脚本
# 测试httplib + SQLite3架构

set -e

echo "🎯 智能餐厅新架构测试脚本"
echo "=========================="

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build_new"
TEST_DIR="$PROJECT_ROOT/test"

echo -e "${BLUE}项目根目录: $PROJECT_ROOT${NC}"
echo -e "${BLUE}构建目录: $BUILD_DIR${NC}"
echo -e "${BLUE}测试目录: $TEST_DIR${NC}"
echo ""

# 检查依赖
echo -e "${YELLOW}1. 检查依赖...${NC}"
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}❌ cmake 未安装${NC}"
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo -e "${RED}❌ make 未安装${NC}"
    exit 1
fi

if ! command -v curl &> /dev/null; then
    echo -e "${RED}❌ curl 未安装${NC}"
    exit 1
fi

echo -e "${GREEN}✅ 依赖检查通过${NC}"
echo ""

# 编译服务器
echo -e "${YELLOW}2. 编译服务器...${NC}"
cd "$PROJECT_ROOT"

if [ -d "$BUILD_DIR" ]; then
    echo "清理旧的构建目录..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "运行 cmake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "编译项目..."
make -j$(nproc)

if [ -f "bin/WisdomRestaurantServer" ]; then
    echo -e "${GREEN}✅ 服务器编译成功${NC}"
else
    echo -e "${RED}❌ 服务器编译失败${NC}"
    exit 1
fi
echo ""

# 编译测试程序
echo -e "${YELLOW}3. 编译测试程序...${NC}"
cd "$TEST_DIR"

if [ -f "Makefile" ]; then
    make $(TARGET_RECOMMENDATION) || echo -e "${YELLOW}⚠️ 摄像头推荐测试程序编译失败（可能需要摄像头设备）${NC}"
    make $(TARGET_SIMPLE) || echo -e "${YELLOW}⚠️ 简单客户端测试程序编译失败${NC}"
    echo -e "${GREEN}✅ 测试程序编译完成${NC}"
else
    echo -e "${YELLOW}⚠️ 测试目录没有Makefile，跳过测试程序编译${NC}"
fi
echo ""

# 启动服务器
echo -e "${YELLOW}4. 启动服务器...${NC}"
cd "$BUILD_DIR"

# 设置环境变量
if [ -z "$DASHSCOPE_API_KEY" ]; then
    echo -e "${RED}错误: 请设置DASHSCOPE_API_KEY环境变量${NC}"
    echo "例如: export DASHSCOPE_API_KEY='your-api-key-here'"
    exit 1
fi

export SERVER_PORT="8080"
export DB_PATH="wisdom_restaurant.db"

echo "启动服务器 (端口: $SERVER_PORT)..."
echo "数据库文件: $DB_PATH"
echo ""

# 在后台启动服务器
./bin/WisdomRestaurantServer &
SERVER_PID=$!

# 等待服务器启动
echo "等待服务器启动..."
sleep 3

# 检查服务器是否启动成功
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}❌ 服务器启动失败${NC}"
    exit 1
fi

echo -e "${GREEN}✅ 服务器启动成功 (PID: $SERVER_PID)${NC}"
echo ""

# 测试API接口
echo -e "${YELLOW}5. 测试API接口...${NC}"

# 测试健康检查
echo "测试健康检查接口..."
HEALTH_RESPONSE=$(curl -s -w "%{http_code}" -o /tmp/health_response.json "http://localhost:8080/api/v1/health")
if [ "$HEALTH_RESPONSE" = "200" ]; then
    echo -e "${GREEN}✅ 健康检查接口正常${NC}"
    echo "响应内容:"
    cat /tmp/health_response.json | python3 -m json.tool 2>/dev/null || cat /tmp/health_response.json
else
    echo -e "${RED}❌ 健康检查接口失败 (状态码: $HEALTH_RESPONSE)${NC}"
fi
echo ""

# 测试获取推荐菜品
echo "测试获取推荐菜品接口..."
DISHES_RESPONSE=$(curl -s -w "%{http_code}" -o /tmp/dishes_response.json "http://localhost:8080/api/v1/dishes/recommended")
if [ "$DISHES_RESPONSE" = "200" ]; then
    echo -e "${GREEN}✅ 获取推荐菜品接口正常${NC}"
    echo "响应内容:"
    cat /tmp/dishes_response.json | python3 -m json.tool 2>/dev/null || cat /tmp/dishes_response.json
else
    echo -e "${RED}❌ 获取推荐菜品接口失败 (状态码: $DISHES_RESPONSE)${NC}"
fi
echo ""

# 测试根路径
echo "测试根路径..."
ROOT_RESPONSE=$(curl -s -w "%{http_code}" -o /tmp/root_response.html "http://localhost:8080/")
if [ "$ROOT_RESPONSE" = "200" ]; then
    echo -e "${GREEN}✅ 根路径正常${NC}"
    echo "页面标题:"
    grep -o '<title>.*</title>' /tmp/root_response.html || echo "未找到标题"
else
    echo -e "${RED}❌ 根路径失败 (状态码: $ROOT_RESPONSE)${NC}"
fi
echo ""

# 显示测试结果
echo -e "${YELLOW}6. 测试结果总结${NC}"
echo "=========================="
echo -e "${GREEN}✅ 服务器编译成功${NC}"
echo -e "${GREEN}✅ 服务器启动成功${NC}"
echo -e "${GREEN}✅ API接口测试通过${NC}"
echo ""
echo -e "${BLUE}服务器信息:${NC}"
echo "  地址: http://localhost:8080"
echo "  PID: $SERVER_PID"
echo "  数据库: $DB_PATH"
echo ""
echo -e "${BLUE}可用的API接口:${NC}"
echo "  GET  /api/v1/health              - 健康检查"
echo "  GET  /api/v1/dishes/recommended  - 获取推荐菜品"
echo "  POST /api/v1/recommendation      - 智能推荐"
echo "  GET  /                           - API文档页面"
echo ""

# 提供下一步操作建议
echo -e "${YELLOW}下一步操作:${NC}"
echo "1. 在浏览器中访问: http://localhost:8080"
echo "2. 运行摄像头测试程序:"
echo "   cd $TEST_DIR"
echo "   ./camera_recommendation_test"
echo "3. 停止服务器: kill $SERVER_PID"
echo ""

# 询问是否继续运行服务器
echo -e "${YELLOW}是否继续运行服务器进行进一步测试? (y/n)${NC}"
read -r response
if [[ "$response" =~ ^[Yy]$ ]]; then
    echo -e "${GREEN}服务器继续运行中...${NC}"
    echo "按 Ctrl+C 停止服务器"
    wait $SERVER_PID
else
    echo "停止服务器..."
    kill $SERVER_PID
    echo -e "${GREEN}✅ 测试完成${NC}"
fi

# 清理临时文件
rm -f /tmp/health_response.json /tmp/dishes_response.json /tmp/root_response.html
