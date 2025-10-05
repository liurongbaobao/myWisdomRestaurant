#pragma once

#include "httplib.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "ai/AiService.h"
#include "db/RestaurantDb.h"
#include <memory>
#include <string>

namespace WisdomRestaurant {

class RecommendationController {
public:
    RecommendationController(std::shared_ptr<AiService> ai_service, 
                           std::shared_ptr<RestaurantDb> db);
    ~RecommendationController();

    // 处理智能推荐请求
    void handleRecommendation(const httplib::Request& request, httplib::Response& response);
    
    // 处理获取推荐历史请求
    void handleGetRecommendationHistory(const httplib::Request& request, httplib::Response& response);
    
    // 处理推荐反馈请求
    void handleRecommendationFeedback(const httplib::Request& request, httplib::Response& response);
    
    // 处理获取推荐菜品请求
    void handleGetRecommendedDishes(const httplib::Request& request, httplib::Response& response);

private:
    // 解析推荐请求参数
    bool parseRecommendationRequest(const std::string& body, std::string& image_base64, 
                                   std::string& table_number, std::string& user_id, 
                                   std::string& season, std::string& meal_time);
    
    // 验证请求参数
    bool validateRequest(const std::string& image_base64, const std::string& table_number);
    
    // 构建错误响应
    std::string buildErrorResponse(const std::string& message, int code);
    
    // 构建成功响应
    std::string buildSuccessResponse(const std::string& message, const std::string& data);
    
    // 获取当前季节
    std::string getCurrentSeason();
    
    // 获取当前用餐时间
    std::string getCurrentMealTime();
    
    // 设置CORS头
    void setCorsHeaders(httplib::Response& response);

private:
    std::shared_ptr<AiService> ai_service_;
    std::shared_ptr<RestaurantDb> db_;
};

} // namespace WisdomRestaurant
