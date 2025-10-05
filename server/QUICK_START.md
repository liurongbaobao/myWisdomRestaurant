# 🚀 快速开始指南

## 1分钟快速部署

### 步骤1：获取API密钥
访问 [阿里云通义千问控制台](https://dashscope.console.aliyun.com/) 获取API密钥

### 步骤2：配置环境
```bash
# 复制配置示例
cp config.env.example config.env

# 编辑配置文件，填入真实API密钥
nano config.env
```

### 步骤3：一键启动
```bash
# 给启动脚本添加执行权限
chmod +x start.sh

# 启动服务器
./start.sh
```

### 步骤4：测试服务
```bash
# 测试健康检查
curl http://localhost:8080/api/v1/health

# 测试摄像头推荐（需要摄像头设备）
cd test
make camera_recommendation_test
./camera_recommendation_test
```

## 常见问题

### Q: 提示"未设置DASHSCOPE_API_KEY环境变量"
**A:** 请确保在 `config.env` 文件中设置了正确的API密钥，或使用环境变量：
```bash
export DASHSCOPE_API_KEY="your-api-key-here"
```

### Q: 编译失败
**A:** 请确保安装了必要的依赖：
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libcurl4-openssl-dev libssl-dev

# CentOS/RHEL
sudo yum install gcc-c++ cmake libcurl-devel openssl-devel
```

### Q: 摄像头测试失败
**A:** 请检查摄像头设备权限：
```bash
# 检查摄像头设备
ls -l /dev/video*

# 测试摄像头
v4l2-ctl --list-devices
```

## 更多帮助

- 📖 完整文档：[README.md](README.md)
- 🐛 问题反馈：[GitHub Issues](https://github.com/yourusername/myWisdomRestaurant/issues)
- 💬 讨论交流：[GitHub Discussions](https://github.com/yourusername/myWisdomRestaurant/discussions)
