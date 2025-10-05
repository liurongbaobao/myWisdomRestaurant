#define _POSIX_C_SOURCE 199309L  // 启用POSIX功能

// C标准库头文件
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>      // 包含usleep声明
#include <sys/time.h>    // 解决timespec问题
#include <time.h>        // 解决timespec问题
#include <linux/types.h> // 解决__s32类型问题
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

// C++标准库头文件
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>

class CameraCapture {
private:
    int fd;
    unsigned char *mptr[4];
    unsigned int size[4];
    bool is_streaming;
    
public:
    CameraCapture() : fd(-1), is_streaming(false) {
        memset(mptr, 0, sizeof(mptr));
        memset(size, 0, sizeof(size));
    }
    
    ~CameraCapture() {
        cleanup();
    }
    
    bool initialize(const char* device = "/dev/video0", int width = 640, int height = 480) {
        // 打开摄像头设备
        fd = open(device, O_RDWR);
        if (fd < 0) {
            perror("打开设备失败");
            return false;
        }
        
        // 查询设备能力
        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
            perror("查询设备能力失败");
            close(fd);
            fd = -1;
            return false;
        }
        
        printf("设备信息:\n");
        printf("  驱动名称: %s\n", cap.driver);
        printf("  设备名称: %s\n", cap.card);
        printf("  总线信息: %s\n", cap.bus_info);
        printf("  版本: %u.%u.%u\n", 
               (cap.version >> 16) & 0xFF,
               (cap.version >> 8) & 0xFF,
               cap.version & 0xFF);
        
        // 检查设备是否支持视频捕获
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
            fprintf(stderr, "设备不支持视频捕获\n");
            close(fd);
            fd = -1;
            return false;
        }
        
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf(stderr, "设备不支持流式IO\n");
            close(fd);
            fd = -1;
            return false;
        }
        
        // 获取并设置摄像头格式
        struct v4l2_format vfmt;
        memset(&vfmt, 0, sizeof(vfmt));
        vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        
        // 先获取当前格式
        if (ioctl(fd, VIDIOC_G_FMT, &vfmt) < 0) {
            perror("获取格式失败");
            close(fd);
            fd = -1;
            return false;
        }
        
        printf("当前格式: %dx%d, 像素格式: %c%c%c%c\n",
               vfmt.fmt.pix.width, vfmt.fmt.pix.height,
               vfmt.fmt.pix.pixelformat & 0xFF,
               (vfmt.fmt.pix.pixelformat >> 8) & 0xFF,
               (vfmt.fmt.pix.pixelformat >> 16) & 0xFF,
               (vfmt.fmt.pix.pixelformat >> 24) & 0xFF);
        
        // 设置想要的格式
        vfmt.fmt.pix.width = width;
        vfmt.fmt.pix.height = height;
        vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        
        if (ioctl(fd, VIDIOC_S_FMT, &vfmt) < 0) {
            perror("设置格式失败");
            close(fd);
            fd = -1;
            return false;
        }
        
        printf("设置格式: %dx%d, 像素格式: MJPEG\n", width, height);
        
        // 申请内核缓冲区
        struct v4l2_requestbuffers reqbuffer;
        memset(&reqbuffer, 0, sizeof(reqbuffer));
        reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqbuffer.count = 4;
        reqbuffer.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(fd, VIDIOC_REQBUFS, &reqbuffer) < 0) {
            perror("申请缓冲区失败");
            close(fd);
            fd = -1;
            return false;
        }
        
        printf("申请了 %d 个缓冲区\n", reqbuffer.count);
        
        // 映射缓冲区并放入队列
        struct v4l2_buffer mapbuffer;
        for(int i = 0; i < 4; i++) {
            memset(&mapbuffer, 0, sizeof(mapbuffer));
            mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            mapbuffer.index = i;
            mapbuffer.memory = V4L2_MEMORY_MMAP;
            
            if (ioctl(fd, VIDIOC_QUERYBUF, &mapbuffer) < 0) {
                perror("查询缓冲区失败");
                cleanup();
                return false;
            }
            
            size[i] = mapbuffer.length;
            
            mptr[i] = static_cast<unsigned char*>(
                mmap(NULL, mapbuffer.length, PROT_READ | PROT_WRITE, 
                     MAP_SHARED, fd, mapbuffer.m.offset));
            
            if (mptr[i] == MAP_FAILED) {
                perror("内存映射失败");
                cleanup();
                return false;
            }
            
            // 将缓冲区放入队列
            if (ioctl(fd, VIDIOC_QBUF, &mapbuffer) < 0) {
                perror("缓冲区入队失败");
                cleanup();
                return false;
            }
        }
        
        printf("缓冲区映射完成\n");
        return true;
    }
    
    bool startStreaming() {
        if (fd < 0 || is_streaming) {
            return false;
        }
        
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
            perror("开始采集失败");
            return false;
        }
        
        is_streaming = true;
        printf("开始视频采集\n");
        return true;
    }
    
    bool stopStreaming() {
        if (fd < 0 || !is_streaming) {
            return false;
        }
        
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
            perror("停止采集失败");
            return false;
        }
        
        is_streaming = false;
        printf("停止视频采集\n");
        return true;
    }
    
    bool captureFrame(const char* filename = nullptr) {
        if (fd < 0 || !is_streaming) {
            fprintf(stderr, "设备未初始化或未开始采集\n");
            return false;
        }
        
        // 从队列中提取一帧数据
        struct v4l2_buffer readbuffer;
        memset(&readbuffer, 0, sizeof(readbuffer));
        readbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        readbuffer.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(fd, VIDIOC_DQBUF, &readbuffer) < 0) {
            perror("读取帧数据失败");
            return false;
        }
        
        printf("捕获帧: 索引=%d, 大小=%d字节\n", 
               readbuffer.index, readbuffer.bytesused);
        
        // 生成文件名（如果未提供）
        std::string output_filename;
        if (filename) {
            output_filename = filename;
        } else {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *localtime(&time_t);  // 使用C版本的localtime
            
            std::ostringstream oss;
            oss << "capture_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".jpg";
            output_filename = oss.str();
        }
        
        // 保存图像
        FILE *file = fopen(output_filename.c_str(), "wb");
        if (!file) {
            perror("创建输出文件失败");
            // 将缓冲区放回队列
            ioctl(fd, VIDIOC_QBUF, &readbuffer);
            return false;
        }
        
        // 写入实际捕获的数据
        size_t written = fwrite(mptr[readbuffer.index], readbuffer.bytesused, 1, file);
        fclose(file);
        
        if (written != 1) {
            fprintf(stderr, "写入文件失败\n");
            // 将缓冲区放回队列
            ioctl(fd, VIDIOC_QBUF, &readbuffer);
            return false;
        }
        
        printf("图像已保存到: %s\n", output_filename.c_str());
        
        // 将缓冲区放回队列
        if (ioctl(fd, VIDIOC_QBUF, &readbuffer) < 0) {
            perror("缓冲区入队失败");
            return false;
        }
        
        return true;
    }
    
    bool captureMultipleFrames(int count = 5, int interval_ms = 1000) {
        if (!startStreaming()) {
            return false;
        }
        
        printf("开始连续捕获 %d 帧，间隔 %d 毫秒\n", count, interval_ms);
        
        for (int i = 0; i < count; i++) {
            std::ostringstream filename;
            filename << "frame_" << std::setfill('0') << std::setw(3) << i << ".jpg";
            
            if (!captureFrame(filename.str().c_str())) {
                fprintf(stderr, "捕获第 %d 帧失败\n", i + 1);
                break;
            }
            
            if (i < count - 1) {
                usleep(interval_ms * 1000); // 转换为微秒
            }
        }
        
        stopStreaming();
        return true;
    }
    
    void cleanup() {
        if (is_streaming) {
            stopStreaming();
        }
        
        // 释放内存映射
        for(int i = 0; i < 4; i++) {
            if (mptr[i] && mptr[i] != MAP_FAILED) {
                munmap(mptr[i], size[i]);
                mptr[i] = nullptr;
            }
        }
        
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }
    
    void printUsage() {
        printf("\n摄像头测试程序使用说明:\n");
        printf("  ./camera_test                    - 捕获单张图片\n");
        printf("  ./camera_test single [filename]  - 捕获单张图片到指定文件\n");
        printf("  ./camera_test multi [count]      - 连续捕获多张图片\n");
        printf("  ./camera_test help               - 显示帮助信息\n");
        printf("\n");
    }
};

int main(int argc, char* argv[]) {
    printf("=== 智慧餐厅摄像头测试程序 ===\n\n");
    
    CameraCapture camera;
    
    // 解析命令行参数
    std::string mode = "single";
    std::string filename;
    int frame_count = 5;
    
    if (argc > 1) {
        mode = argv[1];
        
        if (mode == "help" || mode == "-h" || mode == "--help") {
            camera.printUsage();
            return 0;
        }
        
        if (mode == "single" && argc > 2) {
            filename = argv[2];
        }
        
        if (mode == "multi" && argc > 2) {
            frame_count = atoi(argv[2]);
            if (frame_count <= 0) {
                frame_count = 5;
            }
        }
    }
    
    // 初始化摄像头
    printf("正在初始化摄像头...\n");
    if (!camera.initialize()) {
        fprintf(stderr, "摄像头初始化失败\n");
        return -1;
    }
    
    printf("\n");
    
    // 根据模式执行不同操作
    if (mode == "single") {
        printf("=== 单张图片捕获模式 ===\n");
        if (!camera.startStreaming()) {
            return -1;
        }
        
        // 等待摄像头稳定
        printf("等待摄像头稳定...\n");
        usleep(500000); // 500ms
        
        const char* output_file = filename.empty() ? nullptr : filename.c_str();
        if (camera.captureFrame(output_file)) {
            printf("图片捕获成功！\n");
        } else {
            fprintf(stderr, "图片捕获失败！\n");
            return -1;
        }
        
        camera.stopStreaming();
        
    } else if (mode == "multi") {
        printf("=== 连续图片捕获模式 ===\n");
        if (!camera.captureMultipleFrames(frame_count, 1000)) {
            fprintf(stderr, "连续捕获失败！\n");
            return -1;
        }
        printf("连续捕获完成！\n");
        
    } else {
        fprintf(stderr, "未知模式: %s\n", mode.c_str());
        camera.printUsage();
        return -1;
    }
    
    printf("\n=== 测试完成 ===\n");
    return 0;
}
