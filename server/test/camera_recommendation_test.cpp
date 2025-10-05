#define _POSIX_C_SOURCE 199309L  // 启用POSIX功能

// C标准库头文件
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <linux/types.h>
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
#include <fstream>
#include <vector>

// 第三方库
#include "httplib.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// Base64编码函数
std::string base64_encode(const unsigned char* data, size_t length) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    for (size_t i = 0; i < length; i++) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (result.size() % 4) {
        result.push_back('=');
    }
    return result;
}

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
        
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
            std::cerr << "设备不支持视频捕获" << std::endl;
            close(fd);
            fd = -1;
            return false;
        }
        
        // 设置视频格式
        struct v4l2_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
        
        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
            perror("设置视频格式失败");
            close(fd);
            fd = -1;
            return false;
        }
        
        // 请求缓冲区
        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof(req));
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            perror("请求缓冲区失败");
            close(fd);
            fd = -1;
            return false;
        }
        
        // 映射缓冲区
        for (int i = 0; i < 4; i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            
            if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
                perror("查询缓冲区失败");
                cleanup();
                return false;
            }
            
            mptr[i] = (unsigned char*)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
            if (mptr[i] == MAP_FAILED) {
                perror("映射缓冲区失败");
                cleanup();
                return false;
            }
            size[i] = buf.length;
        }
        
        std::cout << "摄像头初始化成功: " << width << "x" << height << std::endl;
        return true;
    }
    
    bool startStreaming() {
        if (fd < 0) return false;
        
        // 将缓冲区加入队列
        for (int i = 0; i < 4; i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            
            if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
                perror("将缓冲区加入队列失败");
                return false;
            }
        }
        
        // 开始流
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
            perror("开始流失败");
            return false;
        }
        
        is_streaming = true;
        std::cout << "开始视频流" << std::endl;
        return true;
    }
    
    bool captureFrame(std::vector<unsigned char>& frame_data) {
        if (!is_streaming) return false;
        
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            perror("从队列取出缓冲区失败");
            return false;
        }
        
        // 复制帧数据
        frame_data.resize(buf.bytesused);
        memcpy(frame_data.data(), mptr[buf.index], buf.bytesused);
        
        // 将缓冲区重新加入队列
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("将缓冲区重新加入队列失败");
            return false;
        }
        
        return true;
    }
    
    void cleanup() {
        if (is_streaming) {
            enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ioctl(fd, VIDIOC_STREAMOFF, &type);
            is_streaming = false;
        }
        
        for (int i = 0; i < 4; i++) {
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
};

class RecommendationClient {
private:
    httplib::Client client;
    std::string server_url;
    
public:
    RecommendationClient(const std::string& url = "http://localhost:8080") 
        : client(url), server_url(url) {
    }
    
    bool testConnection() {
        auto res = client.Get("/api/v1/health");
        if (res && res->status == 200) {
            std::cout << "✅ 服务器连接正常" << std::endl;
            return true;
        } else {
            std::cout << "❌ 服务器连接失败" << std::endl;
            return false;
        }
    }
    
    bool getRecommendedDishes() {
        auto res = client.Get("/api/v1/dishes/recommended");
        if (res && res->status == 200) {
            std::cout << "✅ 获取推荐菜品成功" << std::endl;
            std::cout << "响应内容: " << res->body << std::endl;
            return true;
        } else {
            std::cout << "❌ 获取推荐菜品失败" << std::endl;
            if (res) {
                std::cout << "状态码: " << res->status << std::endl;
                std::cout << "错误信息: " << res->body << std::endl;
            }
            return false;
        }
    }
    
    bool sendRecommendationRequest(const std::string& image_base64, 
                                 const std::string& table_number = "T001",
                                 const std::string& user_id = "test_user",
                                 const std::string& season = "",
                                 const std::string& meal_time = "") {
        
        // 构建请求JSON
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();
        
        doc.AddMember("image_base64", rapidjson::Value(image_base64.c_str(), alloc), alloc);
        doc.AddMember("table_number", rapidjson::Value(table_number.c_str(), alloc), alloc);
        doc.AddMember("user_id", rapidjson::Value(user_id.c_str(), alloc), alloc);
        
        if (!season.empty()) {
            doc.AddMember("season", rapidjson::Value(season.c_str(), alloc), alloc);
        }
        if (!meal_time.empty()) {
            doc.AddMember("meal_time", rapidjson::Value(meal_time.c_str(), alloc), alloc);
        }
        
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        
        std::string json_data = buffer.GetString();
        
        // 发送请求
        auto res = client.Post("/api/v1/recommendation", json_data, "application/json");
        
        if (res && res->status == 200) {
            std::cout << "✅ 智能推荐请求成功" << std::endl;
            std::cout << "响应内容: " << res->body << std::endl;
            
            // 解析响应
            rapidjson::Document response_doc;
            response_doc.Parse(res->body.c_str());
            
            if (response_doc.IsObject() && response_doc.HasMember("data")) {
                const auto& data = response_doc["data"];
                if (data.HasMember("recommendations")) {
                    const auto& recommendations = data["recommendations"];
                    if (recommendations.IsArray()) {
                        std::cout << "\n🍽️ 推荐菜品:" << std::endl;
                        for (rapidjson::SizeType i = 0; i < recommendations.Size(); i++) {
                            const auto& rec = recommendations[i];
                            if (rec.HasMember("dish_name") && rec.HasMember("reason")) {
                                std::cout << "  " << (i+1) << ". " << rec["dish_name"].GetString() 
                                         << " - " << rec["reason"].GetString() << std::endl;
                            }
                        }
                    }
                }
            }
            
            return true;
        } else {
            std::cout << "❌ 智能推荐请求失败" << std::endl;
            if (res) {
                std::cout << "状态码: " << res->status << std::endl;
                std::cout << "错误信息: " << res->body << std::endl;
            }
            return false;
        }
    }
};

void printUsage(const char* program_name) {
    std::cout << "用法: " << program_name << " [选项]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -d <设备>     摄像头设备路径 (默认: /dev/video0)" << std::endl;
    std::cout << "  -s <服务器>   服务器地址 (默认: http://localhost:8080)" << std::endl;
    std::cout << "  -t <桌号>     餐桌号码 (默认: T001)" << std::endl;
    std::cout << "  -u <用户ID>   用户ID (默认: test_user)" << std::endl;
    std::cout << "  -h            显示帮助信息" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string device = "/dev/video0";
    std::string server_url = "http://localhost:8080";
    std::string table_number = "T001";
    std::string user_id = "test_user";
    
    // 解析命令行参数
    int opt;
    while ((opt = getopt(argc, argv, "d:s:t:u:h")) != -1) {
        switch (opt) {
            case 'd':
                device = optarg;
                break;
            case 's':
                server_url = optarg;
                break;
            case 't':
                table_number = optarg;
                break;
            case 'u':
                user_id = optarg;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    
    std::cout << "🎥 智能餐厅摄像头推荐测试程序" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "摄像头设备: " << device << std::endl;
    std::cout << "服务器地址: " << server_url << std::endl;
    std::cout << "餐桌号码: " << table_number << std::endl;
    std::cout << "用户ID: " << user_id << std::endl;
    std::cout << "=================================" << std::endl;
    
    // 初始化推荐客户端
    RecommendationClient client(server_url);
    
    // 测试服务器连接
    std::cout << "\n1. 测试服务器连接..." << std::endl;
    if (!client.testConnection()) {
        std::cerr << "无法连接到服务器，请确保服务器正在运行" << std::endl;
        return 1;
    }
    
    // 获取推荐菜品列表
    std::cout << "\n2. 获取推荐菜品列表..." << std::endl;
    client.getRecommendedDishes();
    
    // 初始化摄像头
    std::cout << "\n3. 初始化摄像头..." << std::endl;
    CameraCapture camera;
    if (!camera.initialize(device.c_str())) {
        std::cerr << "摄像头初始化失败" << std::endl;
        return 1;
    }
    
    if (!camera.startStreaming()) {
        std::cerr << "启动视频流失败" << std::endl;
        return 1;
    }
    
    // 等待用户输入
    std::cout << "\n4. 摄像头就绪，按回车键拍照进行智能推荐..." << std::endl;
    std::cout << "   (按 'q' 退出程序)" << std::endl;
    
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "q" || input == "quit") {
            break;
        }
        
        std::cout << "\n📸 正在拍照..." << std::endl;
        
        // 捕获一帧
        std::vector<unsigned char> frame_data;
        if (!camera.captureFrame(frame_data)) {
            std::cerr << "拍照失败" << std::endl;
            continue;
        }
        
        std::cout << "✅ 拍照成功，图片大小: " << frame_data.size() << " 字节" << std::endl;
        
        // 保存图片到文件（可选）
        std::string filename = "capture_" + std::to_string(std::time(nullptr)) + ".jpg";
        std::ofstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(frame_data.data()), frame_data.size());
            file.close();
            std::cout << "📁 图片已保存到: " << filename << std::endl;
        }
        
        // 转换为Base64
        std::cout << "🔄 正在编码图片..." << std::endl;
        std::string image_base64 = base64_encode(frame_data.data(), frame_data.size());
        std::cout << "✅ 图片编码完成，Base64长度: " << image_base64.length() << std::endl;
        
        // 发送推荐请求
        std::cout << "\n🤖 正在发送智能推荐请求..." << std::endl;
        if (client.sendRecommendationRequest(image_base64, table_number, user_id)) {
            std::cout << "✅ 智能推荐完成！" << std::endl;
        } else {
            std::cout << "❌ 智能推荐失败" << std::endl;
        }
        
        std::cout << "\n按回车键继续拍照，或输入 'q' 退出..." << std::endl;
    }
    
    std::cout << "\n👋 程序结束" << std::endl;
    return 0;
}
