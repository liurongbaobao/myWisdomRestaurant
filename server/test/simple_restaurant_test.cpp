#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>

// 简单的Base64编码实现
class SimpleBase64 {
public:
    static std::string encode(const std::vector<unsigned char>& data) {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        int val = 0, valb = -6;
        
        for (size_t i = 0; i < data.size(); ++i) {
            val = (val << 8) + data[i];
            valb += 8;
            while (valb >= 0) {
                result.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        
        if (valb > -6) {
            result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }
        
        while (result.size() % 4) {
            result.push_back('=');
        }
        
        return result;
    }
    
    static std::string encodeFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << filepath << std::endl;
            return "";
        }
        
        std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)),
                                         std::istreambuf_iterator<char>());
        file.close();
        
        if (buffer.empty()) {
            std::cerr << "文件为空: " << filepath << std::endl;
            return "";
        }
        
        return encode(buffer);
    }
};

// HTTP响应结构
struct HttpResponse {
    std::string data;
    long status_code;
    
    HttpResponse() : status_code(0) {}
};

// HTTP回调函数
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    size_t total_size = size * nmemb;
    data->append(static_cast<char*>(contents), total_size);
    return total_size;
}

// 发送HTTP POST请求
HttpResponse sendPostRequest(const std::string& url, const std::string& json_data) {
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    
    // 添加协议支持和兼容性选项
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "WisdomRestaurant-TestClient/1.0");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);  // 启用详细输出用于调试
    
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

// 捕获图像
std::string captureImage() {
    std::cout << "正在捕获图像..." << std::endl;
    
    // 生成文件名
    std::string filename = "test_capture.jpg";
    
    // 调用简单的摄像头程序
    std::string command = "./simple_camera_test /dev/video0 " + filename;
    int result = system(command.c_str());
    
    if (result == 0) {
        std::cout << "图像捕获成功: " << filename << std::endl;
        return filename;
    } else {
        std::cerr << "图像捕获失败，尝试使用camera_test..." << std::endl;
        
        // 尝试使用C++版本
        command = "./camera_test single " + filename;
        result = system(command.c_str());
        
        if (result == 0) {
            std::cout << "图像捕获成功: " << filename << std::endl;
            return filename;
        } else {
            std::cerr << "所有摄像头程序都失败了" << std::endl;
            return "";
        }
    }
}

// 构建简单的JSON请求
std::string buildJsonRequest(const std::string& base64_image) {
    std::string json = "{";
    json += "\"image\":\"" + base64_image + "\",";
    json += "\"table_number\":\"TEST_001\",";
    json += "\"user_info\":{";
    json += "\"user_id\":\"test_user\",";
    json += "\"preferences\":[]";
    json += "}";
    json += "}";
    return json;
}

// 测试服务器健康状态
bool testServerHealth(const std::string& server_url) {
    std::cout << "测试服务器连接..." << std::endl;
    
    std::string health_url = server_url + "/api/v1/health";
    HttpResponse response = sendPostRequest(health_url, "{}");
    
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

// 发送推荐请求
bool sendRecommendationRequest(const std::string& server_url, const std::string& base64_image) {
    std::cout << "发送菜品推荐请求..." << std::endl;
    
    std::string json_data = buildJsonRequest(base64_image);
    std::cout << "请求数据大小: " << json_data.length() << " 字节" << std::endl;
    
    std::string recommendation_url = server_url + "/api/v1/recommendation";
    HttpResponse response = sendPostRequest(recommendation_url, json_data);
    
    if (response.status_code == 200) {
        std::cout << "推荐请求成功！" << std::endl;
        std::cout << "\n=== 服务器响应 ===" << std::endl;
        std::cout << response.data << std::endl;
        return true;
    } else {
        std::cerr << "推荐请求失败，状态码: " << response.status_code << std::endl;
        if (!response.data.empty()) {
            std::cerr << "错误响应: " << response.data << std::endl;
        }
        return false;
    }
}

// 显示帮助信息
void showHelp(const char* program_name) {
    std::cout << "智慧餐厅简单客户端测试程序" << std::endl;
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
    
    std::cout << "\n=== 智慧餐厅简单客户端测试 ===" << std::endl;
    std::cout << "服务器地址: " << server_url << std::endl;
    std::cout << std::endl;
    
    // 初始化CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    bool success = false;
    
    do {
        // 1. 测试服务器连接
        if (!testServerHealth(server_url)) {
            std::cerr << "服务器连接测试失败" << std::endl;
            break;
        }
        
        // 2. 捕获图像
        std::string image_file = captureImage();
        if (image_file.empty()) {
            std::cerr << "图像捕获失败" << std::endl;
            break;
        }
        
        // 3. 编码图像为Base64
        std::string base64_image = SimpleBase64::encodeFile(image_file);
        if (base64_image.empty()) {
            std::cerr << "图像编码失败" << std::endl;
            break;
        }
        
        std::cout << "图像编码完成，大小: " << base64_image.length() << " 字符" << std::endl;
        
        // 4. 发送推荐请求
        if (!sendRecommendationRequest(server_url, base64_image)) {
            std::cerr << "推荐请求失败" << std::endl;
            break;
        }
        
        success = true;
        
    } while (false);
    
    // 清理CURL
    curl_global_cleanup();
    
    std::cout << "\n=== 测试" << (success ? "完成" : "失败") << " ===" << std::endl;
    
    return success ? 0 : 1;
}
