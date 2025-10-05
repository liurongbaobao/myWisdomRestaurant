# 摄像头测试程序

这是一个基于V4L2（Video4Linux2）的摄像头测试程序，用于测试Linux系统下的USB摄像头功能。该程序可以用于智慧餐厅项目中的图像采集功能测试。

## 功能特性

- ✅ 自动检测和初始化摄像头设备
- ✅ 支持MJPEG格式图像捕获
- ✅ 单张图片捕获
- ✅ 连续多张图片捕获
- ✅ 自动生成带时间戳的文件名
- ✅ 完整的错误处理和资源清理
- ✅ 详细的设备信息显示

## 系统要求

- Linux操作系统
- USB摄像头或内置摄像头
- V4L2驱动支持
- C++17编译器

## 安装依赖

### Ubuntu/Debian系统
```bash
# 安装V4L2开发库和工具
sudo apt-get update
sudo apt-get install -y v4l-utils libv4l-dev build-essential

# 或者使用Makefile安装
make install-deps
```

### CentOS/RHEL系统
```bash
sudo yum install -y v4l-utils v4l-utils-devel gcc-c++
```

## 编译程序

```bash
# 进入测试目录
cd server/test

# 编译程序
make

# 或者手动编译
g++ -Wall -Wextra -std=c++17 -O2 -o camera_test camera_test.cpp
```

## 使用方法

### 1. 检查摄像头设备
```bash
# 检查摄像头是否被系统识别
make check-camera

# 或者手动检查
ls /dev/video*
v4l2-ctl --list-devices
```

### 2. 基本使用

```bash
# 捕获单张图片（自动命名）
./camera_test

# 捕获单张图片到指定文件
./camera_test single my_photo.jpg

# 连续捕获5张图片（默认）
./camera_test multi

# 连续捕获10张图片
./camera_test multi 10

# 显示帮助信息
./camera_test help
```

### 3. 快速测试

```bash
# 单张捕获测试
make test

# 连续捕获测试
make test-multi
```

## 输出文件

- **单张捕获**: `capture_YYYYMMDD_HHMMSS.jpg`
- **指定文件名**: 用户指定的文件名
- **连续捕获**: `frame_000.jpg`, `frame_001.jpg`, ...

## 程序输出示例

```
=== 智慧餐厅摄像头测试程序 ===

正在初始化摄像头...
设备信息:
  驱动名称: uvcvideo
  设备名称: USB2.0 HD UVC WebCam
  总线信息: usb-0000:00:14.0-5
  版本: 4.15.0
当前格式: 640x480, 像素格式: YUYV
设置格式: 640x480, 像素格式: MJPEG
申请了 4 个缓冲区
缓冲区映射完成

=== 单张图片捕获模式 ===
开始视频采集
等待摄像头稳定...
捕获帧: 索引=0, 大小=15234字节
图像已保存到: test_image.jpg
图片捕获成功！
停止视频采集

=== 测试完成 ===
```

## 故障排除

### 1. 设备权限问题
```bash
# 添加用户到video组
sudo usermod -a -G video $USER

# 或者临时修改权限
sudo chmod 666 /dev/video0
```

### 2. 设备被占用
```bash
# 检查哪个进程在使用摄像头
sudo lsof /dev/video0

# 停止可能占用摄像头的程序
sudo pkill -f cheese  # 停止cheese程序
sudo pkill -f guvcview # 停止guvcview程序
```

### 3. 驱动问题
```bash
# 检查USB设备
lsusb | grep -i camera

# 检查内核模块
lsmod | grep uvc
```

### 4. 格式不支持
```bash
# 查看支持的格式
v4l2-ctl --device=/dev/video0 --list-formats-ext
```

## 集成到智慧餐厅项目

这个测试程序可以作为智慧餐厅项目中图像采集功能的基础：

1. **图像采集**: 为AI推荐功能提供图像数据
2. **设备检测**: 确保摄像头设备正常工作
3. **格式验证**: 验证图像格式是否符合AI模型要求
4. **性能测试**: 测试图像采集的速度和质量

## 技术说明

- **像素格式**: 使用MJPEG格式，压缩率高，传输效率好
- **缓冲区**: 使用4个缓冲区进行内存映射，提高采集效率
- **分辨率**: 默认640x480，可根据需要调整
- **错误处理**: 完整的错误检查和资源清理机制

## 许可证

本程序遵循MIT许可证，可自由使用和修改。
