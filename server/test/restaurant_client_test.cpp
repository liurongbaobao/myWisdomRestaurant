#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include "base64_utils.h"

// HTTP响应结构
struct HttpResponse {
    std::string data;
    long status_code;
    
    HttpResponse() : status_code(0) {}
};

// 摄像头捕获类
class CameraCapture {
public:
    CameraCapture() = default;
    
    // 捕获图片并返回文件路径
    std::string captureImage(const std::string& filename = "") {
        std::string output_file = filename.empty() ? generateFilename() : filename;
        
        std::cout << "正在捕获图像..." << std::endl;
        
        // 调用摄像头程序捕获图片
        std::string command = "./camera_test single " + output_file;
        int result = system(command.c_str());
        
        if (result == 0) {
            std::cout << "图像捕获成功: " << output_file << std::endl;
            return output_file;
        } else {
            std::cerr << "图像捕获失败" << std::endl;
            return "";
        }
    }
    
private:
    std::string generateFilename() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "test_capture_%Y%m%d_%H%M%S.jpg", &tm);
        return std::string(buffer);
    }
};

// Base64编码工具类（使用我们的实现）
class Base64Encoder {
public:
    static std::string encodeFile(const std::string& filepath) {
        std::string result = Base64Utils::encodeFile(filepath);
        if (result.empty()) {
            std::cerr << "无法编码文件: " << filepath << std::endl;
        }
        return result;
    }
};

// HTTP客户端类
class HttpClient {
public:
    HttpClient() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    
    ~HttpClient() {
        curl_global_cleanup();
    }
    
    // 发送POST请求
    HttpResponse post(const std::string& url, const std::string& json_data) {
        CURL* curl = curl_easy_init();
        HttpResponse response;
        
        if (!curl) {
            std::cerr << "初始化CURL失败" << std::endl;
            return response;
        }
        
        // 设置请求头
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
        headers = curl_slist_append(headers, "Accept: application/json");
        
        // 配置CURL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        // 执行请求
        CURLcode res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
        } else {
            std::cerr << "HTTP请求失败: " << curl_easy_strerror(res) << std::endl;
        }
        
        // 清理
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return response;
    }
    
private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
        size_t total_size = size * nmemb;
        data->append(static_cast<char*>(contents), total_size);
        return total_size;
    }
};

// 智慧餐厅客户端测试类
class RestaurantClientTest {
public:
    RestaurantClientTest(const std::string& server_url) 
        : server_url_(server_url), camera_(), http_client_() {}
    
    // 执行完整的测试流程
    bool runTest() {
        std::cout << "\n=== 智慧餐厅客户端测试 ===" << std::endl;
        std::cout << "服务器地址: " << server_url_ << std::endl;
        std::cout << std::endl;
        
        // 1. 测试服务器连接
        if (!testServerConnection()) {
            std::cerr << "服务器连接测试失败" << std::endl;
            return false;
        }
        
        // 2. 捕获图像
        std::string image_file = camera_.captureImage();
        if (image_file.empty()) {
            std::cerr << "图像捕获失败" << std::endl;
            return false;
        }
        
        // 3. 编码图像为Base64
        std::string base64_image = Base64Encoder::encodeFile(image_file);
        if (base64_image.empty()) {
            std::cerr << "图像编码失败" << std::endl;
            return false;
        }
        
        std::cout << "图像编码完成，大小: " << base64_image.length() << " 字符" << std::endl;
        
        // 4. 发送推荐请求
        if (!sendRecommendationRequest(base64_image)) {
            std::cerr << "推荐请求失败" << std::endl;
            return false;
        }
        
        std::cout << "\n=== 测试完成 ===" << std::endl;
        return true;
    }
    
private:
    std::string server_url_;
    CameraCapture camera_;
    HttpClient http_client_;
    
    // 测试服务器连接
    bool testServerConnection() {
        std::cout << "测试服务器连接..." << std::endl;
        
        std::string health_url = server_url_ + "/api/v1/health";
        HttpResponse response = http_client_.post(health_url, "{}");
        
        if (response.status_code == 200) {
            std::cout << "服务器连接正常" << std::endl;
            std::cout << "响应: " << response.data << std::endl;
            return true;
        } else {
            std::cerr << "服务器连接失败，状态码: " << response.status_code << std::endl;
            if (!response.data.empty()) {
                std::cerr << "响应内容: " << response.data << std::endl;
            }
            return false;
        }
    }
    
    // 发送菜品推荐请求
    bool sendRecommendationRequest(const std::string& base64_image) {
        std::cout << "发送菜品推荐请求..." << std::endl;
        
        // 构建请求JSON
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        
        // 添加图像数据
        rapidjson::Value image_value;
        image_value.SetString(base64_image.c_str(), base64_image.length(), allocator);
        doc.AddMember("image", image_value, allocator);
        
        // 添加餐桌号
        doc.AddMember("table_number", "TEST_001", allocator);
        
        // 添加用户信息（可选）
        rapidjson::Value user_info(rapidjson::kObjectType);
        user_info.AddMember("user_id", "test_user", allocator);
        user_info.AddMember("preferences", rapidjson::Value(rapidjson::kArrayType), allocator);
        doc.AddMember("user_info", user_info, allocator);
        
        // 转换为JSON字符串
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        
        std::string json_data = buffer.GetString();
        std::cout << "请求数据大小: " << json_data.length() << " 字节" << std::endl;
        
        // 发送请求
        std::string recommendation_url = server_url_ + "/api/v1/recommendation";
        HttpResponse response = http_client_.post(recommendation_url, json_data);
        
        // 处理响应
        if (response.status_code == 200) {
            std::cout << "推荐请求成功！" << std::endl;
            parseRecommendationResponse(response.data);
            return true;
        } else {
            std::cerr << "推荐请求失败，状态码: " << response.status_code << std::endl;
            if (!response.data.empty()) {
                std::cerr << "错误响应: " << response.data << std::endl;
            }
            return false;
        }
    }
    
    // 解析推荐响应
    void parseRecommendationResponse(const std::string& json_response) {
        std::cout << "\n=== 菜品推荐结果 ===" << std::endl;
        
        rapidjson::Document doc;
        if (doc.Parse(json_response.c_str()).HasParseError()) {
            std::cerr << "JSON解析失败" << std::endl;
            std::cout << "原始响应: " << json_response << std::endl;
            return;
        }
        
        // 美化输出JSON
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        
        std::cout << buffer.GetString() << std::endl;
        
        // 提取关键信息
        if (doc.HasMember("success") && doc["success"].GetBool()) {
            if (doc.HasMember("data")) {
                const auto& data = doc["data"];
                
                if (data.HasMember("vision_result")) {
                    std::cout << "\n--- 视觉识别结果 ---" << std::endl;
                    const auto& vision = data["vision_result"];
                    if (vision.HasMember("people_num")) {
                        std::cout << "识别人数: " << vision["people_num"].GetString() << std::endl;
                    }
                }
                
                if (data.HasMember("recommendations")) {
                    std::cout << "\n--- 推荐菜品 ---" << std::endl;
                    const auto& recommendations = data["recommendations"];
                    if (recommendations.IsArray()) {
                        for (rapidjson::SizeType i = 0; i < recommendations.Size(); i++) {
                            const auto& dish = recommendations[i];
                            if (dish.HasMember("dish_name") && dish.HasMember("reason")) {
                                std::cout << (i + 1) << ". " << dish["dish_name"].GetString() 
                                         << " - " << dish["reason"].GetString() << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }
};

// 显示帮助信息
void showHelp(const char* program_name) {
    std::cout << "智慧餐厅客户端测试程序" << std::endl;
    std::cout << std::endl;
    std::cout << "使用方法:" << std::endl;
    std::cout << "  " << program_name << " [server_url]" << std::endl;
    std::cout << std::endl;
    std::cout << "参数:" << std::endl;
    std::cout << "  server_url  服务器地址 (默认: http://localhost:8080)" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << std::endl;
    std::cout << "  " << program_name << " http://192.168.1.100:8080" << std::endl;
    std::cout << std::endl;
    std::cout << "功能:" << std::endl;
    std::cout << "  1. 测试服务器连接" << std::endl;
    std::cout << "  2. 捕获摄像头图像" << std::endl;
    std::cout << "  3. 上传图像并获取菜品推荐" << std::endl;
    std::cout << "  4. 显示推荐结果" << std::endl;
}

int main(int argc, char* argv[]) {
    // 处理命令行参数
    if (argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        showHelp(argv[0]);
        return 0;
    }
    
    std::string server_url = "http://localhost:8080";
    if (argc > 1) {
        server_url = argv[1];
    }
    
    // 检查摄像头测试程序是否存在
    if (system("test -f ./camera_test") != 0) {
        std::cerr << "错误: 找不到摄像头测试程序 './camera_test'" << std::endl;
        std::cerr << "请先编译摄像头程序: make camera_test" << std::endl;
        return 1;
    }
    
    // 创建并运行测试
    RestaurantClientTest test(server_url);
    
    bool success = test.runTest();
    
    return success ? 0 : 1;
}
