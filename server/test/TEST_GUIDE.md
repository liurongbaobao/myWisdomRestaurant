# 智慧餐厅摄像头测试指南

本指南将帮助您测试智慧餐厅的摄像头拍照和菜品推荐功能。

## 快速开始

### 1. 编译测试程序

```bash
cd server/test
make clean
make
```

这将编译：
- `simple_camera_test` - C版本摄像头程序（稳定）
- `simple_restaurant_test` - 简单客户端测试程序（推荐）

### 2. 启动服务器

在另一个终端中：

```bash
cd server

# 方法1：直接启动（需要先设置环境变量）
export DASHSCOPE_API_KEY=your-api-key-here
export DB_USER=wisdom_user
export DB_PASS=WisdomRestaurant2024!
export DB_HOST=localhost
export DB_NAME=wisdom_restaurant
./build/bin/WisdomRestaurantServer

# 方法2：使用配置文件启动
# 确保config.env中的配置正确，然后：
source config.env && ./build/bin/WisdomRestaurantServer
```

### 3. 运行测试

```bash
cd server/test

# 方法1：基础测试脚本（最兼容，推荐）
make test-basic

# 方法2：简单客户端测试
make test-simple

# 方法3：手动运行
./simple_restaurant_test

# 如果遇到CURL问题，先运行诊断
./diagnose_curl.sh
```

## 详细步骤

### 步骤1：检查摄像头设备

```bash
# 检查摄像头设备
ls /dev/video*

# 查看摄像头信息
v4l2-ctl --list-devices

# 设置摄像头权限（如果需要）
sudo chmod 666 /dev/video0
```

### 步骤2：测试摄像头捕获

```bash
# 测试C版本摄像头程序
./simple_camera_test /dev/video0 test.jpg

# 检查生成的图片
ls -la *.jpg
file test.jpg
```

### 步骤3：测试服务器连接

```bash
# 测试服务器健康状态
curl -X POST http://localhost:8080/api/v1/health \
  -H "Content-Type: application/json" \
  -d '{}'
```

### 步骤4：完整功能测试

```bash
# 运行完整的客户端测试
./simple_restaurant_test

# 指定服务器地址
./simple_restaurant_test http://192.168.1.100:8080
```

## 测试流程说明

完整的测试流程包括：

1. **服务器连接测试**
   - 发送健康检查请求到 `/api/v1/health`
   - 验证服务器是否正常响应

2. **图像捕获**
   - 调用摄像头程序捕获图片
   - 生成 `test_capture.jpg` 文件

3. **图像编码**
   - 将图片文件编码为Base64格式
   - 准备上传数据

4. **菜品推荐请求**
   - 发送POST请求到 `/api/v1/recommendation`
   - 包含Base64编码的图片和测试参数

5. **结果显示**
   - 解析服务器响应
   - 显示推荐的菜品信息

## 预期输出

成功的测试应该产生类似以下的输出：

```
=== 智慧餐厅简单客户端测试 ===
服务器地址: http://localhost:8080

测试服务器连接...
服务器连接正常
响应: {"success":true,"message":"服务器运行正常","timestamp":"2024-12-02T12:34:56Z"}

正在捕获图像...
图像捕获成功: test_capture.jpg

图像编码完成，大小: 12345 字符

发送菜品推荐请求...
请求数据大小: 12500 字节
推荐请求成功！

=== 服务器响应 ===
{
  "success": true,
  "data": {
    "vision_result": {
      "people_num": "2",
      "customer_portrait": [
        {"age_grades": "青年", "gender": "男"},
        {"age_grades": "青年", "gender": "女"}
      ]
    },
    "recommendations": [
      {"dish_name": "宫保鸡丁", "reason": "适合年轻人的经典川菜"},
      {"dish_name": "麻婆豆腐", "reason": "营养丰富，口感丰富"},
      {"dish_name": "糖醋里脊", "reason": "酸甜口味，老少皆宜"}
    ]
  }
}

=== 测试完成 ===
```

## 故障排除

### 1. 摄像头问题

**问题**: `打开设备失败`
```bash
# 解决方案
sudo chmod 666 /dev/video0
# 或者
sudo usermod -a -G video $USER
# 然后重新登录
```

**问题**: `设置格式失败`
```bash
# 检查摄像头支持的格式
v4l2-ctl --list-formats-ext
```

### 2. 服务器连接问题

**问题**: `服务器连接失败`
```bash
# 检查服务器是否运行
ps aux | grep WisdomRestaurantServer

# 检查端口是否监听
netstat -tlnp | grep 8080

# 检查防火墙
sudo ufw status
```

### 3. 编译问题

**问题**: `找不到curl库`
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev curl

# CentOS/RHEL
sudo yum install libcurl-devel curl

# 检查安装
curl-config --version
curl-config --protocols
```

**问题**: `找不到头文件`
```bash
# 安装开发包
sudo apt-get install build-essential
```

**问题**: `HTTP请求失败: Unsupported protocol`
```bash
# 诊断CURL问题
chmod +x diagnose_curl.sh
./diagnose_curl.sh

# 使用基础测试脚本（推荐）
make test-basic
```

### 4. 数据库连接问题

**问题**: `数据库连接失败`
```bash
# 检查MySQL服务
sudo systemctl status mysql

# 测试数据库连接
mysql -u wisdom_user -p'WisdomRestaurant2024!' wisdom_restaurant -e "SELECT 1;"
```

## 高级测试

### 1. 性能测试

```bash
# 连续测试多次
for i in {1..5}; do
  echo "=== 测试 $i ==="
  ./simple_restaurant_test
  sleep 2
done
```

### 2. 不同服务器测试

```bash
# 测试本地服务器
./simple_restaurant_test http://localhost:8080

# 测试远程服务器
./simple_restaurant_test http://192.168.1.100:8080
```

### 3. 手动API测试

```bash
# 1. 捕获图片
./simple_camera_test /dev/video0 manual_test.jpg

# 2. 编码为Base64
base64 -w 0 manual_test.jpg > image_base64.txt

# 3. 构建JSON请求
cat > request.json << EOF
{
  "image": "$(cat image_base64.txt)",
  "table_number": "MANUAL_001",
  "user_info": {
    "user_id": "manual_test",
    "preferences": []
  }
}
EOF

# 4. 发送请求
curl -X POST http://localhost:8080/api/v1/recommendation \
  -H "Content-Type: application/json" \
  -d @request.json
```

## 文件说明

- `simple_camera_test.c` - C版本摄像头程序（稳定）
- `camera_test.cpp` - C++版本摄像头程序（功能更丰富但可能有编译问题）
- `simple_restaurant_test.cpp` - 简单客户端测试程序（推荐使用）
- `restaurant_client_test.cpp` - 完整客户端测试程序（需要更多依赖）
- `base64_utils.h` - Base64编码工具
- `Makefile` - 编译配置文件

## 注意事项

1. **摄像头权限**: 确保当前用户有访问摄像头的权限
2. **网络连接**: 确保能够访问服务器地址
3. **API密钥**: 确保DASHSCOPE_API_KEY已正确设置
4. **数据库**: 确保数据库服务正常运行且用户权限正确
5. **依赖库**: 确保安装了libcurl和相关开发库

## 联系支持

如果遇到问题，请检查：
1. 服务器日志输出
2. 摄像头设备状态
3. 网络连接
4. 数据库连接

提供错误信息时，请包含完整的错误输出和系统环境信息。
