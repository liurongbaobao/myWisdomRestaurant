#include "ai/AiService.h"
#include <iostream>
#include <cstdlib>
#include <sstream>

namespace WisdomRestaurant {

AiService::AiService() 
    : vision_model_("qwen3-vl-plus")
    , text_model_("qwen-plus")
    , api_endpoint_("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions")
    , initialized_(false) {
}

AiService::~AiService() {
    if (initialized_) {
        curl_global_cleanup();
    }
}

bool AiService::initialize() {
    // 从环境变量获取API密钥
    const char* apiKey = std::getenv("DASHSCOPE_API_KEY");
    if (!apiKey || !*apiKey) {
        std::cerr << "错误：未设置DASHSCOPE_API_KEY环境变量" << std::endl;
        return false;
    }
    api_key_ = apiKey;

    // 初始化CURL
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        std::cerr << "CURL初始化失败: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    initialized_ = true;
    std::cout << "AI服务初始化成功" << std::endl;
    return true;
}

VisionResult AiService::analyzeCustomerImage(const std::string& image_base64) {
    VisionResult result;
    result.success = false;

    if (!initialized_) {
        result.error_message = "AI服务未初始化";
        return result;
    }

    // 构建视觉识别提示词
    std::string prompt = buildVisionPrompt();
    
    // 调用视觉大模型
    std::string response = callLLMAPI(prompt, image_base64);
    
    if (response.empty() || response == "No response from AI") {
        result.error_message = "大模型调用失败";
        return result;
    }

    // 解析结果
    result = parseVisionResult(response);
    return result;
}

RecommendationResult AiService::recommendDishes(const VisionResult& vision_result, 
                                               const std::string& season,
                                               const std::string& meal_time) {
    RecommendationResult result;
    result.success = false;

    if (!initialized_) {
        result.error_message = "AI服务未初始化";
        return result;
    }

    if (!vision_result.success) {
        result.error_message = "客户画像分析失败，无法进行推荐";
        return result;
    }

    // 构建推荐提示词
    std::string prompt = buildRecommendationPrompt(vision_result, season, meal_time);
    
    // 调用文本大模型
    std::string response = callTextLLMAPI(prompt);
    
    if (response.empty() || response == "No response from AI") {
        result.error_message = "推荐服务调用失败";
        return result;
    }

    // 解析推荐结果
    result = parseRecommendationResult(response);
    return result;
}

std::string AiService::callLLMAPI(const std::string& prompt, const std::string& image_base64) {
    std::string response;
    CURL *curl = curl_easy_init();
    
    if (!curl) {
        std::cerr << "CURL初始化失败" << std::endl;
        return "CURL initialization failed";
    }

    // 构建请求体
    std::string body;
    {
        rapidjson::Document d;
        d.SetObject();
        auto &alloc = d.GetAllocator();
        
        d.AddMember("model", rapidjson::Value(vision_model_.c_str(), alloc), alloc);
        rapidjson::Value messages(rapidjson::kArrayType);
        
        {
            rapidjson::Value usr(rapidjson::kObjectType);
            usr.AddMember("role", rapidjson::Value("user", alloc), alloc);
            
            rapidjson::Value content(rapidjson::kArrayType);
            
            // 添加文本内容
            rapidjson::Value textPart(rapidjson::kObjectType);
            textPart.AddMember("type", rapidjson::Value("text", alloc), alloc);
            textPart.AddMember("text", rapidjson::Value(prompt.c_str(), alloc), alloc);
            content.PushBack(textPart, alloc);
            
            // 如果有图片，添加图片内容
            if (!image_base64.empty()) {
                rapidjson::Value imagePart(rapidjson::kObjectType);
                imagePart.AddMember("type", rapidjson::Value("image_url", alloc), alloc);
                rapidjson::Value imageUrl(rapidjson::kObjectType);
                std::string imageDataUrl = "data:image/png;base64," + image_base64;
                imageUrl.AddMember("url", rapidjson::Value(imageDataUrl.c_str(), alloc), alloc);
                imagePart.AddMember("image_url", imageUrl, alloc);
                content.PushBack(imagePart, alloc);
            }
            
            usr.AddMember("content", content, alloc);
            messages.PushBack(usr, alloc);
        }
        
        d.AddMember("messages", messages, alloc);
        d.AddMember("max_tokens", 2048, alloc);
        d.AddMember("temperature", 0.7, alloc);
        
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);
        body.assign(sb.GetString(), sb.GetSize());
    }

    // 设置HTTP头
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth = "Authorization: Bearer " + api_key_;
    headers = curl_slist_append(headers, auth.c_str());

    // 配置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, api_endpoint_.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // 执行请求
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        std::cerr << "CURL请求失败: " << curl_easy_strerror(res) << std::endl;
        response = "CURL request failed";
    }

    // 清理资源
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // 解析响应
    std::string answer;
    if (!response.empty() && response != "CURL request failed") {
        rapidjson::Document rd;
        rd.Parse(response.c_str());
        
        if (rd.IsObject() && rd.HasMember("choices") && rd["choices"].IsArray() && rd["choices"].Size() > 0) {
            const auto &c0 = rd["choices"][0];
            if (c0.IsObject() && c0.HasMember("message") && c0["message"].IsObject()) {
                const auto &msg = c0["message"];
                if (msg.HasMember("content") && msg["content"].IsString()) {
                    answer = msg["content"].GetString();
                }
            }
        }
    }

    if (answer.empty()) {
        answer = "No response from AI";
    }

    return answer;
}

std::string AiService::callTextLLMAPI(const std::string& prompt) {
    std::string response;
    CURL *curl = curl_easy_init();
    
    if (!curl) {
        std::cerr << "CURL初始化失败" << std::endl;
        return "CURL initialization failed";
    }

    // 构建请求体
    std::string body;
    {
        rapidjson::Document d;
        d.SetObject();
        auto &alloc = d.GetAllocator();
        
        d.AddMember("model", rapidjson::Value(text_model_.c_str(), alloc), alloc);
        rapidjson::Value messages(rapidjson::kArrayType);
        
        {
            rapidjson::Value usr(rapidjson::kObjectType);
            usr.AddMember("role", rapidjson::Value("user", alloc), alloc);
            usr.AddMember("content", rapidjson::Value(prompt.c_str(), alloc), alloc);
            messages.PushBack(usr, alloc);
        }
        
        d.AddMember("messages", messages, alloc);
        d.AddMember("max_tokens", 2048, alloc);
        d.AddMember("temperature", 0.8, alloc);
        
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);
        body.assign(sb.GetString(), sb.GetSize());
    }

    // 设置HTTP头
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth = "Authorization: Bearer " + api_key_;
    headers = curl_slist_append(headers, auth.c_str());

    // 配置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, api_endpoint_.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // 执行请求
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        std::cerr << "CURL请求失败: " << curl_easy_strerror(res) << std::endl;
        response = "CURL request failed";
    }

    // 清理资源
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // 解析响应
    std::string answer;
    if (!response.empty() && response != "CURL request failed") {
        rapidjson::Document rd;
        rd.Parse(response.c_str());
        
        if (rd.IsObject() && rd.HasMember("choices") && rd["choices"].IsArray() && rd["choices"].Size() > 0) {
            const auto &c0 = rd["choices"][0];
            if (c0.IsObject() && c0.HasMember("message") && c0["message"].IsObject()) {
                const auto &msg = c0["message"];
                if (msg.HasMember("content") && msg["content"].IsString()) {
                    answer = msg["content"].GetString();
                }
            }
        }
    }

    if (answer.empty()) {
        answer = "No response from AI";
    }

    return answer;
}

VisionResult AiService::parseVisionResult(const std::string& response) {
    VisionResult result;
    result.success = false;

    try {
        rapidjson::Document doc;
        doc.Parse(response.c_str());

        if (doc.HasParseError()) {
            result.error_message = "JSON解析失败";
            return result;
        }

        if (doc.HasMember("people_num") && doc["people_num"].IsString()) {
            result.people_num = std::stoi(doc["people_num"].GetString());
        }

        if (doc.HasMember("customer_portrait") && doc["customer_portrait"].IsArray()) {
            const auto& portraits = doc["customer_portrait"];
            for (rapidjson::SizeType i = 0; i < portraits.Size(); i++) {
                const auto& portrait = portraits[i];
                CustomerPortrait cp;
                
                if (portrait.HasMember("age_grades") && portrait["age_grades"].IsString()) {
                    cp.age_grades = portrait["age_grades"].GetString();
                }
                if (portrait.HasMember("gender") && portrait["gender"].IsString()) {
                    cp.gender = portrait["gender"].GetString();
                }
                if (portrait.HasMember("body_type") && portrait["body_type"].IsString()) {
                    cp.body_type = portrait["body_type"].GetString();
                }
                
                result.customer_portrait.push_back(cp);
            }
        }

        result.success = true;
    } catch (const std::exception& e) {
        result.error_message = "解析视觉识别结果时发生异常: " + std::string(e.what());
    }

    return result;
}

RecommendationResult AiService::parseRecommendationResult(const std::string& response) {
    RecommendationResult result;
    result.success = false;

    try {
        rapidjson::Document doc;
        doc.Parse(response.c_str());

        if (doc.HasParseError()) {
            result.error_message = "JSON解析失败";
            return result;
        }

        if (doc.IsArray()) {
            for (rapidjson::SizeType i = 0; i < doc.Size(); i++) {
                const auto& dish = doc[i];
                DishRecommendation dr;
                
                if (dish.HasMember("dish_name") && dish["dish_name"].IsString()) {
                    dr.dish_name = dish["dish_name"].GetString();
                }
                if (dish.HasMember("reason") && dish["reason"].IsString()) {
                    dr.reason = dish["reason"].GetString();
                }
                if (dish.HasMember("taste_level") && dish["taste_level"].IsString()) {
                    dr.taste_level = dish["taste_level"].GetString();
                }
                if (dish.HasMember("nutrition_advice") && dish["nutrition_advice"].IsString()) {
                    dr.nutrition_advice = dish["nutrition_advice"].GetString();
                }
                
                result.recommendations.push_back(dr);
            }
        }

        result.success = true;
    } catch (const std::exception& e) {
        result.error_message = "解析推荐结果时发生异常: " + std::string(e.what());
    }

    return result;
}

std::string AiService::buildVisionPrompt() {
    return R"(获取图像上的人数，性别，年龄，严格按照以下的 json 字符串返回
json 示例：{"people_num":"1","customer_portrait":[{"age_grades":"青年","gender":"man","body_type":"标准"}]}

请分析图片中的人物特征：
1. 统计人数
2. 识别每个人的性别（man/woman）
3. 判断年龄段（儿童/青年/中年/老年）
4. 评估体型（瘦/标准/胖）

请严格按照JSON格式返回结果。)";
}

std::string AiService::buildRecommendationPrompt(const VisionResult& vision_result,
                                               const std::string& season,
                                               const std::string& meal_time) {
    std::ostringstream oss;
    oss << "你是一个专业的餐厅营养师和美食顾问。\n";
    oss << "顾客信息：\n";
    
    if (!vision_result.customer_portrait.empty()) {
        const auto& customer = vision_result.customer_portrait[0];
        oss << "- 年龄：" << customer.age_grades << "\n";
        oss << "- 体型：" << customer.body_type << "\n";
        oss << "- 性别：" << customer.gender << "\n";
    }
    
    oss << "- 用餐人数：" << vision_result.people_num << "\n";
    oss << "- 当前季节：" << season << "\n";
    oss << "- 当前时间：" << meal_time << "\n\n";
    
    oss << "请根据以上信息：\n";
    oss << "1. 推荐3道最适合的招牌菜，并说明推荐理由\n";
    oss << "2. 建议合适的菜品口味（辣度、咸度、甜度）\n";
    oss << "3. 给出营养搭配建议\n\n";
    
    oss << "严格按照以下的 json 字符串返回\n";
    oss << R"(示例：[{"dish_name":"糖醋里脊","reason":"小孩爱吃甜的","taste_level":"微甜","nutrition_advice":"富含蛋白质"}])";
    
    return oss.str();
}

size_t AiService::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

} // namespace WisdomRestaurant
