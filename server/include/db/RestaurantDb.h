#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <mutex>

namespace WisdomRestaurant {

// 数据库实体结构（与RestaurantDb.h相同）
struct User {
    int id;
    std::string user_id;
    std::string nickname;
    std::string phone;
    std::string email;
    std::string avatar_url;
    std::string gender;
    std::string age_grades;
    std::string body_type;
    std::string taste_preference;
    std::string dietary_restrictions;
    std::string created_at;
    std::string updated_at;
};

struct Table {
    int id;
    std::string table_number;
    std::string table_name;
    int seat_count;
    std::string table_type;
    std::string status;
    std::string location;
    std::string qr_code;
    std::string created_at;
    std::string updated_at;
};

struct Dish {
    int id;
    std::string dish_code;
    std::string dish_name;
    int category_id;
    double price;
    double original_price;
    std::string description;
    std::string ingredients;
    std::string nutrition_info;
    std::string taste_tags;
    std::string allergen_info;
    int cooking_time;
    std::string difficulty_level;
    std::string image_url;
    std::string images;
    bool is_recommended;
    bool is_signature;
    bool is_available;
    int stock_count;
    int sales_count;
    double rating;
    int rating_count;
    std::string created_at;
    std::string updated_at;
};

struct Order {
    int id;
    std::string order_no;
    int table_id;
    std::string user_id;
    std::string order_type;
    int people_count;
    double total_amount;
    double discount_amount;
    double final_amount;
    std::string payment_method;
    std::string payment_status;
    std::string order_status;
    std::string special_requirements;
    int estimated_time;
    int actual_time;
    std::string order_time;
    std::string confirm_time;
    std::string complete_time;
    std::string created_at;
    std::string updated_at;
};

struct OrderItem {
    int id;
    int order_id;
    int dish_id;
    std::string dish_name;
    double dish_price;
    int quantity;
    double subtotal;
    std::string special_requirements;
    std::string item_status;
    std::string created_at;
    std::string updated_at;
};

struct AiRecommendation {
    int id;
    std::string session_id;
    int table_id;
    std::string user_id;
    std::string image_base64;
    std::string vision_result;
    std::string recommendation_result;
    std::string season;
    std::string meal_time;
    int people_count;
    std::string customer_portraits;
    std::string recommended_dishes;
    bool is_accepted;
    int feedback_score;
    std::string feedback_comment;
    int processing_time;
    std::string created_at;
    std::string updated_at;
};

struct ClientHeartbeat {
    int id;
    int table_id;
    std::string client_id;
    double temperature;
    double light_intensity;
    double humidity;
    double noise_level;
    int battery_level;
    int signal_strength;
    std::string device_status;
    std::string last_heartbeat;
    std::string created_at;
    std::string updated_at;
};

struct ServiceCall {
    int id;
    std::string call_id;
    int table_id;
    std::string client_id;
    std::string user_id;
    std::string call_type;
    std::string priority;
    std::string description;
    std::string call_status;
    std::string assigned_staff_id;
    std::string call_time;
    std::string response_time;
    std::string complete_time;
    int response_duration;
    int service_duration;
    int customer_rating;
    std::string customer_feedback;
    std::string created_at;
    std::string updated_at;
};

class RestaurantDb {
public:
    RestaurantDb();
    ~RestaurantDb();

    // 初始化数据库连接
    bool initialize(const std::string& db_path = "wisdom_restaurant.db");

    // 用户相关操作
    std::optional<User> getUserById(const std::string& user_id);
    bool createUser(const User& user);
    bool updateUser(const User& user);
    bool deleteUser(const std::string& user_id);

    // 餐桌相关操作
    std::optional<Table> getTableByNumber(const std::string& table_number);
    std::optional<Table> getTableById(int table_id);
    std::vector<Table> getAllTables();
    bool updateTableStatus(int table_id, const std::string& status);

    // 菜品相关操作
    std::vector<Dish> getAllDishes();
    std::vector<Dish> getDishesByCategory(int category_id);
    std::vector<Dish> getRecommendedDishes();
    std::vector<Dish> getSignatureDishes();
    std::optional<Dish> getDishById(int dish_id);
    std::optional<Dish> getDishByCode(const std::string& dish_code);
    bool updateDishStock(int dish_id, int stock_count);

    // 订单相关操作
    std::string createOrder(const Order& order);
    bool addOrderItem(const OrderItem& item);
    std::optional<Order> getOrderByNo(const std::string& order_no);
    std::vector<OrderItem> getOrderItems(int order_id);
    bool updateOrderStatus(const std::string& order_no, const std::string& status);
    bool updateOrderPaymentStatus(const std::string& order_no, const std::string& payment_status);

    // AI推荐相关操作
    bool saveAiRecommendation(const AiRecommendation& recommendation);
    std::optional<AiRecommendation> getAiRecommendation(const std::string& session_id);
    bool updateAiRecommendationFeedback(const std::string& session_id, int score, const std::string& comment);

    // 心跳相关操作
    bool updateClientHeartbeat(const ClientHeartbeat& heartbeat);
    std::optional<ClientHeartbeat> getClientHeartbeat(int table_id, const std::string& client_id);
    std::vector<ClientHeartbeat> getActiveClients();
    bool cleanupInactiveClients(int timeout_seconds);

    // 服务呼叫相关操作
    std::string createServiceCall(const ServiceCall& call);
    std::vector<ServiceCall> getPendingServiceCalls();
    std::vector<ServiceCall> getServiceCallsByTable(int table_id);
    bool updateServiceCallStatus(const std::string& call_id, const std::string& status);
    bool assignServiceCall(const std::string& call_id, const std::string& staff_id);
    bool completeServiceCall(const std::string& call_id, int rating, const std::string& feedback);

    // 统计相关操作
    int getTodayOrderCount();
    double getTodayRevenue();
    std::vector<std::pair<std::string, int>> getPopularDishes(int limit = 10);
    std::vector<std::pair<std::string, double>> getTableUtilization();

    // 生成唯一ID（公共方法）
    std::string generateSessionId();

private:
    // 生成唯一ID（私有方法）
    std::string generateOrderNo();
    std::string generateCallId();

    // 数据库操作辅助方法
    bool executeSQL(const std::string& sql);
    bool executeSQLWithParams(const std::string& sql, const std::vector<std::string>& params);
    std::vector<std::vector<std::string>> executeQuery(const std::string& sql);
    std::vector<std::vector<std::string>> executeQueryWithParams(const std::string& sql, const std::vector<std::string>& params);
    
    // 创建表结构
    bool createTables();
    
    // 插入示例数据
    void insertSampleData();
    
    // 数据库连接
    sqlite3* db_;
    std::mutex db_mutex_;
    bool initialized_;
};

} // namespace WisdomRestaurant
