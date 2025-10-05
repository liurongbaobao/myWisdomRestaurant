#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

// 打印设备信息
void print_device_info(int fd) {
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
        printf("设备信息:\n");
        printf("  驱动: %s\n", cap.driver);
        printf("  设备: %s\n", cap.card);
        printf("  总线: %s\n", cap.bus_info);
        
        if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
            printf("  支持视频捕获\n");
        if (cap.capabilities & V4L2_CAP_STREAMING)
            printf("  支持流式IO\n");
        printf("\n");
    }
}

// 生成带时间戳的文件名
void generate_filename(char* filename, size_t size) {
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    snprintf(filename, size, "capture_%04d%02d%02d_%02d%02d%02d.jpg",
             timeinfo->tm_year + 1900,
             timeinfo->tm_mon + 1,
             timeinfo->tm_mday,
             timeinfo->tm_hour,
             timeinfo->tm_min,
             timeinfo->tm_sec);
}

int main(int argc, char* argv[]) {
    printf("=== 简单摄像头测试程序 ===\n\n");
    
    // 设备路径
    const char* device_path = "/dev/video0";
    if (argc > 1) {
        device_path = argv[1];
    }
    
    printf("正在打开设备: %s\n", device_path);
    
    // 打开摄像头设备
    int fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("打开设备失败");
        printf("提示: 请确保摄像头已连接并且有访问权限\n");
        printf("      sudo chmod 666 /dev/video0\n");
        return -1;
    }
    
    // 显示设备信息
    print_device_info(fd);
    
    // 获取并设置摄像头格式
    struct v4l2_format vfmt;
    memset(&vfmt, 0, sizeof(vfmt));
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    // 先获取当前格式
    if (ioctl(fd, VIDIOC_G_FMT, &vfmt) < 0) {
        perror("获取格式失败");
        close(fd);
        return -1;
    }
    
    printf("当前格式: %dx%d\n", vfmt.fmt.pix.width, vfmt.fmt.pix.height);
    
    // 设置想要的格式
    vfmt.fmt.pix.width = 640;
    vfmt.fmt.pix.height = 480;
    vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    
    if (ioctl(fd, VIDIOC_S_FMT, &vfmt) < 0) {
        perror("设置格式失败");
        printf("尝试使用YUYV格式...\n");
        vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        if (ioctl(fd, VIDIOC_S_FMT, &vfmt) < 0) {
            perror("设置YUYV格式也失败");
            close(fd);
            return -1;
        }
    }
    
    printf("设置格式: %dx%d\n", vfmt.fmt.pix.width, vfmt.fmt.pix.height);
    
    // 申请内核空间
    struct v4l2_requestbuffers reqbuffer;
    memset(&reqbuffer, 0, sizeof(reqbuffer));
    reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffer.count = 4;
    reqbuffer.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbuffer) < 0) {
        perror("申请空间失败");
        close(fd);
        return -1;
    }
    
    printf("申请了 %d 个缓冲区\n", reqbuffer.count);
    
    // 映射缓冲区
    unsigned char *mptr[4];
    unsigned int size[4];
    struct v4l2_buffer mapbuffer;
    
    for(int i = 0; i < 4; i++) {
        memset(&mapbuffer, 0, sizeof(mapbuffer));
        mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mapbuffer.index = i;
        mapbuffer.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(fd, VIDIOC_QUERYBUF, &mapbuffer) < 0) {
            perror("查询内核空间失败");
            close(fd);
            return -1;
        }
        
        size[i] = mapbuffer.length; // 保存缓冲区大小
        
        mptr[i] = (unsigned char*)mmap(NULL, mapbuffer.length, 
                                      PROT_READ | PROT_WRITE, MAP_SHARED, 
                                      fd, mapbuffer.m.offset);
        if (mptr[i] == MAP_FAILED) {
            perror("mmap失败");
            close(fd);
            return -1;
        }
        
        // 将缓冲区放入队列
        if (ioctl(fd, VIDIOC_QBUF, &mapbuffer) < 0) {
            perror("放回失败");
            close(fd);
            return -1;
        }
    }
    
    printf("缓冲区映射完成\n");
    
    // 开始采集
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("开启失败");
        close(fd);
        return -1;
    }
    
    printf("开始视频采集...\n");
    
    // 等待摄像头稳定
    printf("等待摄像头稳定...\n");
    usleep(500000); // 500ms
    
    // 从队列中提取一帧数据
    struct v4l2_buffer readbuffer;
    memset(&readbuffer, 0, sizeof(readbuffer));
    readbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    readbuffer.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(fd, VIDIOC_DQBUF, &readbuffer) < 0) {
        perror("读取数据失败");
        close(fd);
        return -1;
    }
    
    printf("捕获帧数据: 索引=%d, 大小=%d字节\n", 
           readbuffer.index, readbuffer.bytesused);
    
    // 生成输出文件名
    char filename[256];
    if (argc > 2) {
        strncpy(filename, argv[2], sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    } else {
        generate_filename(filename, sizeof(filename));
    }
    
    // 保存图像
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("打开文件失败");
        close(fd);
        return -1;
    }
    
    // 使用实际捕获的数据长度（bytesused）而不是缓冲区长度
    size_t written = fwrite(mptr[readbuffer.index], readbuffer.bytesused, 1, file);
    fclose(file);
    
    if (written == 1) {
        printf("图像已保存到: %s\n", filename);
    } else {
        printf("写入文件失败\n");
    }
    
    // 将缓冲区放回队列
    if (ioctl(fd, VIDIOC_QBUF, &readbuffer) < 0) {
        perror("放回队列失败");
    }
    
    // 停止采集
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("停止采集失败");
    }
    
    printf("停止视频采集\n");
    
    // 释放映射
    for(int i = 0; i < 4; i++) {
        if (mptr[i] != MAP_FAILED) {
            munmap(mptr[i], size[i]);
        }
    }
    
    close(fd);
    
    printf("\n=== 测试完成 ===\n");
    printf("使用方法:\n");
    printf("  %s                    - 使用默认设备和文件名\n", argv[0]);
    printf("  %s /dev/video0        - 指定设备\n", argv[0]);
    printf("  %s /dev/video0 my.jpg - 指定设备和文件名\n", argv[0]);
    
    return 0;
}
