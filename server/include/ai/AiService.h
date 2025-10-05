#pragma once

#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace WisdomRestaurant {

// 客户画像结构
struct CustomerPortrait {
    std::string age_grades;    // 年龄段：儿童、青年、中年、老年
    std::string gender;        // 性别：man、woman
    std::string body_type;     // 体型：瘦、标准、胖
};

// 视觉识别结果
struct VisionResult {
    int people_num;
    std::vector<CustomerPortrait> customer_portrait;
    bool success;
    std::string error_message;
};

// 菜品推荐结构
struct DishRecommendation {
    std::string dish_name;     // 菜品名称
    std::string reason;        // 推荐理由
    std::string taste_level;   // 口味等级（辣度、咸度、甜度）
    std::string nutrition_advice; // 营养建议
};

// 推荐结果
struct RecommendationResult {
    std::vector<DishRecommendation> recommendations;
    std::string taste_suggestion;  // 整体口味建议
    std::string nutrition_advice;  // 营养搭配建议
    bool success;
    std::string error_message;
};

class AiService {
public:
    AiService();
    ~AiService();

    // 初始化AI服务
    bool initialize();

    // 第一阶段：视觉理解 - 分析图片获取客户画像
    VisionResult analyzeCustomerImage(const std::string& image_base64);

    // 第二阶段：智能推荐 - 基于客户画像推荐菜品
    RecommendationResult recommendDishes(const VisionResult& vision_result, 
                                       const std::string& season = "春季",
                                       const std::string& meal_time = "午餐");

private:
    // 调用大模型API的通用方法
    std::string callLLMAPI(const std::string& prompt, const std::string& image_base64 = "");
    
    // 调用纯文本大模型API
    std::string callTextLLMAPI(const std::string& prompt);

    // 解析视觉识别结果
    VisionResult parseVisionResult(const std::string& response);

    // 解析推荐结果
    RecommendationResult parseRecommendationResult(const std::string& response);

    // 构建视觉识别提示词
    std::string buildVisionPrompt();

    // 构建推荐提示词
    std::string buildRecommendationPrompt(const VisionResult& vision_result,
                                        const std::string& season,
                                        const std::string& meal_time);

    // CURL写回调函数
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

private:
    std::string api_key_;
    std::string vision_model_;     // 视觉模型名称
    std::string text_model_;       // 文本模型名称
    std::string api_endpoint_;     // API端点
    bool initialized_;
};

} // namespace WisdomRestaurant
