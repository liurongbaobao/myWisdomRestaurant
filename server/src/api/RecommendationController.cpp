#include "api/RecommendationController.h"
#include "loguru.hpp"
#include <iostream>
#include <chrono>
#include <ctime>

namespace WisdomRestaurant {

RecommendationController::RecommendationController(std::shared_ptr<AiService> ai_service, 
                                                 std::shared_ptr<RestaurantDb> db)
    : ai_service_(ai_service), db_(db) {
}

RecommendationController::~RecommendationController() {
}

void RecommendationController::handleRecommendation(const httplib::Request& request, httplib::Response& response) {
    setCorsHeaders(response);

    try {
        // 解析请求参数
        std::string image_base64, table_number, user_id, season, meal_time;
        if (!parseRecommendationRequest(request.body, image_base64, table_number, 
                                       user_id, season, meal_time)) {
            response.status = 400;
            response.set_content(buildErrorResponse("请求参数解析失败", 400), "application/json; charset=utf-8");
            return;
        }

        // 验证请求参数
        if (!validateRequest(image_base64, table_number)) {
            response.status = 400;
            response.set_content(buildErrorResponse("请求参数验证失败：图片或桌号不能为空", 400), "application/json; charset=utf-8");
            return;
        }

        // 获取餐桌信息
        auto table = db_->getTableByNumber(table_number);
        if (!table) {
            response.status = 404;
            response.set_content(buildErrorResponse("餐桌不存在", 404), "application/json; charset=utf-8");
            return;
        }

        // 如果季节或用餐时间为空，使用当前时间推断
        if (season.empty()) {
            season = getCurrentSeason();
        }
        if (meal_time.empty()) {
            meal_time = getCurrentMealTime();
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // 第一阶段：视觉识别
        LOG_F(INFO, "开始视觉识别...");
        VisionResult vision_result = ai_service_->analyzeCustomerImage(image_base64);
        
        if (!vision_result.success) {
            response.status = 500;
            response.set_content(buildErrorResponse("视觉识别失败：" + vision_result.error_message, 500), "application/json; charset=utf-8");
            return;
        }

        LOG_F(INFO, "视觉识别成功，识别到 %d 人", vision_result.people_num);

        // 第二阶段：智能推荐
        LOG_F(INFO, "开始智能推荐...");
        RecommendationResult recommendation_result = ai_service_->recommendDishes(vision_result, season, meal_time);
        
        if (!recommendation_result.success) {
            response.status = 500;
            response.set_content(buildErrorResponse("智能推荐失败：" + recommendation_result.error_message, 500), "application/json; charset=utf-8");
            return;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        LOG_F(INFO, "智能推荐成功，推荐了 %zu 道菜品", recommendation_result.recommendations.size());

        // 生成会话ID
        std::string session_id = db_->generateSessionId();

        // 保存推荐记录到数据库
        AiRecommendation ai_recommendation;
        ai_recommendation.session_id = session_id;
        ai_recommendation.table_id = table->id;
        ai_recommendation.user_id = user_id;
        ai_recommendation.image_base64 = image_base64;
        ai_recommendation.season = season;
        ai_recommendation.meal_time = meal_time;
        ai_recommendation.people_count = vision_result.people_num;
        ai_recommendation.processing_time = static_cast<int>(processing_time);

        // 将结果序列化为JSON字符串
        rapidjson::Document vision_doc;
        vision_doc.SetObject();
        auto& alloc = vision_doc.GetAllocator();
        
        vision_doc.AddMember("success", vision_result.success, alloc);
        vision_doc.AddMember("people_num", vision_result.people_num, alloc);
        // 将customer_portrait向量序列化为JSON字符串
        rapidjson::Value customer_portrait_array(rapidjson::kArrayType);
        for (const auto& portrait : vision_result.customer_portrait) {
            rapidjson::Value portrait_obj(rapidjson::kObjectType);
            portrait_obj.AddMember("age_grades", rapidjson::Value(portrait.age_grades.c_str(), alloc), alloc);
            portrait_obj.AddMember("gender", rapidjson::Value(portrait.gender.c_str(), alloc), alloc);
            portrait_obj.AddMember("body_type", rapidjson::Value(portrait.body_type.c_str(), alloc), alloc);
            customer_portrait_array.PushBack(portrait_obj, alloc);
        }
        vision_doc.AddMember("customer_portrait", customer_portrait_array, alloc);
        vision_doc.AddMember("error_message", rapidjson::Value(vision_result.error_message.c_str(), alloc), alloc);

        rapidjson::StringBuffer vision_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> vision_writer(vision_buffer);
        vision_doc.Accept(vision_writer);
        ai_recommendation.vision_result = vision_buffer.GetString();

        rapidjson::Document rec_doc;
        rec_doc.SetObject();
        rec_doc.AddMember("success", recommendation_result.success, alloc);
        rec_doc.AddMember("recommendations", rapidjson::Value(rapidjson::kArrayType), alloc);
        for (const auto& rec : recommendation_result.recommendations) {
            rapidjson::Value rec_obj(rapidjson::kObjectType);
            rec_obj.AddMember("dish_name", rapidjson::Value(rec.dish_name.c_str(), alloc), alloc);
            rec_obj.AddMember("reason", rapidjson::Value(rec.reason.c_str(), alloc), alloc);
            rec_obj.AddMember("confidence", 0.8, alloc); // 暂时使用固定值
            rec_doc["recommendations"].PushBack(rec_obj, alloc);
        }
        rec_doc.AddMember("error_message", rapidjson::Value(recommendation_result.error_message.c_str(), alloc), alloc);

        rapidjson::StringBuffer rec_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> rec_writer(rec_buffer);
        rec_doc.Accept(rec_writer);
        ai_recommendation.recommendation_result = rec_buffer.GetString();

        // 保存到数据库
        if (!db_->saveAiRecommendation(ai_recommendation)) {
            LOG_F(WARNING, "保存AI推荐记录失败");
        }

        // 构建响应数据
        rapidjson::Document response_doc;
        response_doc.SetObject();
        response_doc.AddMember("session_id", rapidjson::Value(session_id.c_str(), alloc), alloc);
        response_doc.AddMember("table_number", rapidjson::Value(table_number.c_str(), alloc), alloc);
        response_doc.AddMember("people_count", vision_result.people_num, alloc);
        response_doc.AddMember("season", rapidjson::Value(season.c_str(), alloc), alloc);
        response_doc.AddMember("meal_time", rapidjson::Value(meal_time.c_str(), alloc), alloc);
        response_doc.AddMember("processing_time", static_cast<int>(processing_time), alloc);
        response_doc.AddMember("recommendations", rec_doc["recommendations"], alloc);

        rapidjson::StringBuffer response_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> response_writer(response_buffer);
        response_doc.Accept(response_writer);

        response.status = 200;
        response.set_content(buildSuccessResponse("推荐成功", response_buffer.GetString()), "application/json; charset=utf-8");

    } catch (const std::exception& e) {
        LOG_F(ERROR, "处理推荐请求时发生异常: %s", e.what());
        response.status = 500;
        response.set_content(buildErrorResponse("服务器内部错误", 500), "application/json; charset=utf-8");
    }
}

void RecommendationController::handleGetRecommendationHistory(const httplib::Request& request, httplib::Response& response) {
    setCorsHeaders(response);
    
    try {
        // 获取查询参数
        std::string table_number = request.get_param_value("table_number");
        std::string user_id = request.get_param_value("user_id");
        int limit = 10;
        
        if (request.has_param("limit")) {
            limit = std::stoi(request.get_param_value("limit"));
        }

        // 这里应该实现获取推荐历史的逻辑
        // 暂时返回空结果
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        doc.AddMember("recommendations", rapidjson::Value(rapidjson::kArrayType), alloc);
        doc.AddMember("total", 0, alloc);
        doc.AddMember("limit", limit, alloc);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        response.status = 200;
        response.set_content(buildSuccessResponse("获取推荐历史成功", buffer.GetString()), "application/json; charset=utf-8");

    } catch (const std::exception& e) {
        LOG_F(ERROR, "获取推荐历史时发生异常: %s", e.what());
        response.status = 500;
        response.set_content(buildErrorResponse("服务器内部错误", 500), "application/json; charset=utf-8");
    }
}

void RecommendationController::handleRecommendationFeedback(const httplib::Request& request, httplib::Response& response) {
    setCorsHeaders(response);
    
    try {
        rapidjson::Document doc;
        doc.Parse(request.body.c_str());
        
        if (!doc.IsObject() || !doc.HasMember("session_id") || !doc.HasMember("score")) {
            response.status = 400;
            response.set_content(buildErrorResponse("请求参数不完整", 400), "application/json; charset=utf-8");
            return;
        }

        std::string session_id = doc["session_id"].GetString();
        int score = doc["score"].GetInt();
        std::string comment = doc.HasMember("comment") ? doc["comment"].GetString() : "";

        // 更新推荐反馈
        if (db_->updateAiRecommendationFeedback(session_id, score, comment)) {
            response.status = 200;
            response.set_content(buildSuccessResponse("反馈提交成功", "{}"), "application/json; charset=utf-8");
        } else {
            response.status = 500;
            response.set_content(buildErrorResponse("反馈提交失败", 500), "application/json; charset=utf-8");
        }

    } catch (const std::exception& e) {
        LOG_F(ERROR, "处理推荐反馈时发生异常: %s", e.what());
        response.status = 500;
        response.set_content(buildErrorResponse("服务器内部错误", 500), "application/json; charset=utf-8");
    }
}

void RecommendationController::handleGetRecommendedDishes(const httplib::Request& request, httplib::Response& response) {
    (void)request; // 抑制未使用参数警告
    setCorsHeaders(response);

    try {
        // 获取推荐菜品
        auto dishes = db_->getRecommendedDishes();

        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        rapidjson::Value dishes_array(rapidjson::kArrayType);
        for (const auto& dish : dishes) {
            rapidjson::Value dish_obj(rapidjson::kObjectType);
            dish_obj.AddMember("id", dish.id, alloc);
            dish_obj.AddMember("dish_code", rapidjson::Value(dish.dish_code.c_str(), alloc), alloc);
            dish_obj.AddMember("dish_name", rapidjson::Value(dish.dish_name.c_str(), alloc), alloc);
            dish_obj.AddMember("price", dish.price, alloc);
            dish_obj.AddMember("description", rapidjson::Value(dish.description.c_str(), alloc), alloc);
            dish_obj.AddMember("taste_tags", rapidjson::Value(dish.taste_tags.c_str(), alloc), alloc);
            dish_obj.AddMember("image_url", rapidjson::Value(dish.image_url.c_str(), alloc), alloc);
            dish_obj.AddMember("is_signature", dish.is_signature, alloc);
            dish_obj.AddMember("rating", dish.rating, alloc);
            dish_obj.AddMember("sales_count", dish.sales_count, alloc);
            dishes_array.PushBack(dish_obj, alloc);
        }
        
        doc.AddMember("dishes", dishes_array, alloc);
        doc.AddMember("total", static_cast<int>(dishes.size()), alloc);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        response.status = 200;
        response.set_content(buildSuccessResponse("获取推荐菜品成功", buffer.GetString()), "application/json; charset=utf-8");

    } catch (const std::exception& e) {
        LOG_F(ERROR, "获取推荐菜品时发生异常: %s", e.what());
        response.status = 500;
        response.set_content(buildErrorResponse("服务器内部错误", 500), "application/json; charset=utf-8");
    }
}

bool RecommendationController::parseRecommendationRequest(const std::string& body, std::string& image_base64, 
                                                           std::string& table_number, std::string& user_id, 
                                                           std::string& season, std::string& meal_time) {
    try {
        rapidjson::Document doc;
        doc.Parse(body.c_str());

        if (!doc.IsObject()) {
            return false;
        }

        if (doc.HasMember("image_base64") && doc["image_base64"].IsString()) {
            image_base64 = doc["image_base64"].GetString();
        }
        
        if (doc.HasMember("table_number") && doc["table_number"].IsString()) {
            table_number = doc["table_number"].GetString();
        }
        
        if (doc.HasMember("user_id") && doc["user_id"].IsString()) {
            user_id = doc["user_id"].GetString();
        }
        
        if (doc.HasMember("season") && doc["season"].IsString()) {
            season = doc["season"].GetString();
        }
        
        if (doc.HasMember("meal_time") && doc["meal_time"].IsString()) {
            meal_time = doc["meal_time"].GetString();
        }

        return true;
    } catch (const std::exception& e) {
        LOG_F(ERROR, "解析推荐请求参数失败: %s", e.what());
        return false;
    }
}

bool RecommendationController::validateRequest(const std::string& image_base64, const std::string& table_number) {
    return !image_base64.empty() && !table_number.empty();
}

std::string RecommendationController::buildErrorResponse(const std::string& message, int code) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("code", code, alloc);
    doc.AddMember("message", rapidjson::Value(message.c_str(), alloc), alloc);
    doc.AddMember("data", rapidjson::Value(rapidjson::kObjectType), alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

std::string RecommendationController::buildSuccessResponse(const std::string& message, const std::string& data) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("code", 200, alloc);
    doc.AddMember("message", rapidjson::Value(message.c_str(), alloc), alloc);
    
        rapidjson::Document data_doc;
        data_doc.Parse(data.c_str());
            doc.AddMember("data", data_doc, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

std::string RecommendationController::getCurrentSeason() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    int month = tm.tm_mon + 1; // tm_mon is 0-based
    if (month >= 3 && month <= 5) return "春季";
    if (month >= 6 && month <= 8) return "夏季";
    if (month >= 9 && month <= 11) return "秋季";
        return "冬季";
}

std::string RecommendationController::getCurrentMealTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    int hour = tm.tm_hour;
    if (hour >= 6 && hour < 10) return "早餐";
    if (hour >= 10 && hour < 14) return "午餐";
    if (hour >= 14 && hour < 17) return "下午茶";
    if (hour >= 17 && hour < 21) return "晚餐";
        return "夜宵";
    }

void RecommendationController::setCorsHeaders(httplib::Response& response) {
    response.set_header("Access-Control-Allow-Origin", "*");
    response.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    response.set_header("Access-Control-Allow-Credentials", "true");
}

} // namespace WisdomRestaurant
