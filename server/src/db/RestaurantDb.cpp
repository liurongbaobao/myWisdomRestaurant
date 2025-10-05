#include "db/RestaurantDb.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <random>
#include <iomanip>
#include <loguru.hpp>

namespace WisdomRestaurant {

RestaurantDb::RestaurantDb() : db_(nullptr), initialized_(false) {
}

RestaurantDb::~RestaurantDb() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool RestaurantDb::initialize(const std::string& db_path) {
    try {
        int rc = sqlite3_open(db_path.c_str(), &db_);
        if (rc != SQLITE_OK) {
            LOG_F(ERROR, "无法打开数据库: %s", sqlite3_errmsg(db_));
            return false;
        }

        // 启用外键约束
        executeSQL("PRAGMA foreign_keys = ON;");
        
        // 创建表结构
        if (!createTables()) {
            LOG_F(ERROR, "创建表结构失败");
            return false;
        }

        initialized_ = true;
        LOG_F(INFO, "SQLite数据库初始化成功: %s", db_path.c_str());
        return true;
    } catch (const std::exception& e) {
        LOG_F(ERROR, "数据库初始化失败: %s", e.what());
        return false;
    }
}

bool RestaurantDb::createTables() {
    std::vector<std::string> create_table_sqls = {
        // 用户表
        R"(CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT UNIQUE NOT NULL,
            nickname TEXT,
            phone TEXT,
            email TEXT,
            avatar_url TEXT,
            gender TEXT,
            age_grades TEXT,
            body_type TEXT,
            taste_preference TEXT,
            dietary_restrictions TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",
        
        // 餐桌表
        R"(CREATE TABLE IF NOT EXISTS tables (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            table_number TEXT UNIQUE NOT NULL,
            table_name TEXT,
            seat_count INTEGER DEFAULT 4,
            table_type TEXT DEFAULT 'normal',
            status TEXT DEFAULT 'available',
            location TEXT,
            qr_code TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",
        
        // 菜品表
        R"(CREATE TABLE IF NOT EXISTS dishes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            dish_code TEXT UNIQUE NOT NULL,
            dish_name TEXT NOT NULL,
            category_id INTEGER DEFAULT 1,
            price REAL NOT NULL,
            original_price REAL,
            description TEXT,
            ingredients TEXT,
            nutrition_info TEXT,
            taste_tags TEXT,
            allergen_info TEXT,
            cooking_time INTEGER DEFAULT 15,
            difficulty_level TEXT DEFAULT 'medium',
            image_url TEXT,
            images TEXT,
            is_recommended BOOLEAN DEFAULT 0,
            is_signature BOOLEAN DEFAULT 0,
            is_available BOOLEAN DEFAULT 1,
            stock_count INTEGER DEFAULT 100,
            sales_count INTEGER DEFAULT 0,
            rating REAL DEFAULT 0.0,
            rating_count INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",
        
        // 订单表
        R"(CREATE TABLE IF NOT EXISTS orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_no TEXT UNIQUE NOT NULL,
            table_id INTEGER,
            user_id TEXT,
            order_type TEXT DEFAULT 'dine_in',
            people_count INTEGER DEFAULT 1,
            total_amount REAL DEFAULT 0.0,
            discount_amount REAL DEFAULT 0.0,
            final_amount REAL DEFAULT 0.0,
            payment_method TEXT DEFAULT 'cash',
            payment_status TEXT DEFAULT 'pending',
            order_status TEXT DEFAULT 'pending',
            special_requirements TEXT,
            estimated_time INTEGER DEFAULT 30,
            actual_time INTEGER,
            order_time DATETIME,
            confirm_time DATETIME,
            complete_time DATETIME,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (table_id) REFERENCES tables(id)
        ))",
        
        // 订单项表
        R"(CREATE TABLE IF NOT EXISTS order_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER,
            dish_id INTEGER,
            dish_name TEXT,
            dish_price REAL,
            quantity INTEGER DEFAULT 1,
            subtotal REAL,
            special_requirements TEXT,
            item_status TEXT DEFAULT 'pending',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (order_id) REFERENCES orders(id),
            FOREIGN KEY (dish_id) REFERENCES dishes(id)
        ))",
        
        // AI推荐表
        R"(CREATE TABLE IF NOT EXISTS ai_recommendations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT UNIQUE NOT NULL,
            table_id INTEGER,
            user_id TEXT,
            image_base64 TEXT,
            vision_result TEXT,
            recommendation_result TEXT,
            season TEXT,
            meal_time TEXT,
            people_count INTEGER,
            customer_portraits TEXT,
            recommended_dishes TEXT,
            is_accepted BOOLEAN DEFAULT 0,
            feedback_score INTEGER,
            feedback_comment TEXT,
            processing_time INTEGER,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (table_id) REFERENCES tables(id)
        ))",
        
        // 客户端心跳表
        R"(CREATE TABLE IF NOT EXISTS client_heartbeats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            table_id INTEGER,
            client_id TEXT,
            temperature REAL,
            light_intensity REAL,
            humidity REAL,
            noise_level REAL,
            battery_level INTEGER,
            signal_strength INTEGER,
            device_status TEXT,
            last_heartbeat DATETIME DEFAULT CURRENT_TIMESTAMP,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (table_id) REFERENCES tables(id),
            UNIQUE(table_id, client_id)
        ))",
        
        // 服务呼叫表
        R"(CREATE TABLE IF NOT EXISTS service_calls (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            call_id TEXT UNIQUE NOT NULL,
            table_id INTEGER,
            client_id TEXT,
            user_id TEXT,
            call_type TEXT,
            priority TEXT DEFAULT 'normal',
            description TEXT,
            call_status TEXT DEFAULT 'pending',
            assigned_staff_id TEXT,
            call_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            response_time DATETIME,
            complete_time DATETIME,
            response_duration INTEGER,
            service_duration INTEGER,
            customer_rating INTEGER,
            customer_feedback TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (table_id) REFERENCES tables(id)
        ))"
    };

    for (const auto& sql : create_table_sqls) {
        if (!executeSQL(sql)) {
            return false;
        }
    }

    // 插入一些示例数据
    insertSampleData();
    return true;
}

void RestaurantDb::insertSampleData() {
    // 插入示例餐桌
    executeSQL(R"(INSERT OR IGNORE INTO tables (table_number, table_name, seat_count, table_type, status, location) VALUES 
        ('T001', '1号桌', 4, 'normal', 'available', '大厅'),
        ('T002', '2号桌', 6, 'vip', 'available', '包厢'),
        ('T003', '3号桌', 2, 'normal', 'occupied', '窗边'),
        ('T004', '4号桌', 8, 'family', 'available', '大厅'),
        ('T005', '5号桌', 4, 'normal', 'available', '大厅')
    )");

    // 插入示例菜品
    executeSQL(R"(INSERT OR IGNORE INTO dishes (dish_code, dish_name, category_id, price, original_price, description, ingredients, taste_tags, is_recommended, is_signature) VALUES 
        ('D001', '宫保鸡丁', 1, 28.0, 32.0, '经典川菜，麻辣鲜香', '鸡肉、花生、干辣椒', '麻辣,香辣', 1, 1),
        ('D002', '红烧肉', 1, 35.0, 38.0, '传统家常菜，肥而不腻', '五花肉、冰糖、生抽', '甜咸,软糯', 1, 0),
        ('D003', '清蒸鲈鱼', 2, 48.0, 52.0, '新鲜鲈鱼，清淡鲜美', '鲈鱼、葱、姜', '清淡,鲜美', 1, 1),
        ('D004', '麻婆豆腐', 1, 18.0, 22.0, '四川名菜，麻辣嫩滑', '豆腐、肉末、豆瓣酱', '麻辣,嫩滑', 0, 0),
        ('D005', '糖醋里脊', 1, 32.0, 36.0, '酸甜可口，外酥内嫩', '里脊肉、番茄酱、糖', '酸甜,酥脆', 1, 0)
    )");
}

bool RestaurantDb::executeSQL(const std::string& sql) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    char* errMsg = 0;
    int rc = sqlite3_exec(db_, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        LOG_F(ERROR, "SQL执行失败: %s", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::string RestaurantDb::generateSessionId() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    
    std::ostringstream oss;
    oss << "AI" << std::put_time(&tm, "%Y%m%d%H%M%S") << dis(gen);
    return oss.str();
}

std::string RestaurantDb::generateOrderNo() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::ostringstream oss;
    oss << "ORD" << std::put_time(&tm, "%Y%m%d%H%M%S") << dis(gen);
    return oss.str();
}

std::string RestaurantDb::generateCallId() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100, 999);
    
    std::ostringstream oss;
    oss << "CALL" << std::put_time(&tm, "%Y%m%d%H%M%S") << dis(gen);
    return oss.str();
}

// 用户相关操作
std::optional<User> RestaurantDb::getUserById(const std::string& user_id) {
    if (!initialized_) return std::nullopt;

    std::string sql = "SELECT * FROM users WHERE user_id = ?";
    auto result = executeQueryWithParams(sql, {user_id});
    
    if (!result.empty()) {
        User user;
        user.id = std::stoi(result[0][0]);
        user.user_id = result[0][1];
        user.nickname = result[0][2];
        user.phone = result[0][3];
        user.email = result[0][4];
        user.avatar_url = result[0][5];
        user.gender = result[0][6];
        user.age_grades = result[0][7];
        user.body_type = result[0][8];
        user.taste_preference = result[0][9];
        user.dietary_restrictions = result[0][10];
        user.created_at = result[0][11];
        user.updated_at = result[0][12];
        return user;
    }
    return std::nullopt;
}

bool RestaurantDb::createUser(const User& user) {
    if (!initialized_) return false;

    std::string sql = R"(INSERT INTO users (user_id, nickname, phone, email, avatar_url, gender, 
                         age_grades, body_type, taste_preference, dietary_restrictions)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?))";
    
    std::vector<std::string> params = {
        user.user_id, user.nickname, user.phone, user.email, user.avatar_url,
        user.gender, user.age_grades, user.body_type, user.taste_preference, user.dietary_restrictions
    };
    
    return executeSQLWithParams(sql, params);
}

// 餐桌相关操作
std::optional<Table> RestaurantDb::getTableByNumber(const std::string& table_number) {
    if (!initialized_) return std::nullopt;

    std::string sql = "SELECT * FROM tables WHERE table_number = ?";
    auto result = executeQueryWithParams(sql, {table_number});
    
    if (!result.empty()) {
        Table table;
        table.id = std::stoi(result[0][0]);
        table.table_number = result[0][1];
        table.table_name = result[0][2];
        table.seat_count = std::stoi(result[0][3]);
        table.table_type = result[0][4];
        table.status = result[0][5];
        table.location = result[0][6];
        table.qr_code = result[0][7];
        table.created_at = result[0][8];
        table.updated_at = result[0][9];
        return table;
    }
    return std::nullopt;
}

std::vector<Table> RestaurantDb::getAllTables() {
    std::vector<Table> tables;
    if (!initialized_) return tables;

    std::string sql = "SELECT * FROM tables ORDER BY table_number";
    auto result = executeQuery(sql);
    
    for (const auto& row : result) {
        Table table;
        table.id = std::stoi(row[0]);
        table.table_number = row[1];
        table.table_name = row[2];
        table.seat_count = std::stoi(row[3]);
        table.table_type = row[4];
        table.status = row[5];
        table.location = row[6];
        table.qr_code = row[7];
        table.created_at = row[8];
        table.updated_at = row[9];
        tables.push_back(table);
    }
    return tables;
}

bool RestaurantDb::updateTableStatus(int table_id, const std::string& status) {
    if (!initialized_) return false;

    std::string sql = "UPDATE tables SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    return executeSQLWithParams(sql, {status, std::to_string(table_id)});
}

// 菜品相关操作
std::vector<Dish> RestaurantDb::getAllDishes() {
    std::vector<Dish> dishes;
    if (!initialized_) return dishes;

    std::string sql = "SELECT * FROM dishes WHERE is_available = 1 ORDER BY category_id, dish_name";
    auto result = executeQuery(sql);
    
    for (const auto& row : result) {
        Dish dish;
        dish.id = std::stoi(row[0]);
        dish.dish_code = row[1];
        dish.dish_name = row[2];
        dish.category_id = std::stoi(row[3]);
        dish.price = std::stod(row[4]);
        dish.original_price = std::stod(row[5]);
        dish.description = row[6];
        dish.ingredients = row[7];
        dish.nutrition_info = row[8];
        dish.taste_tags = row[9];
        dish.allergen_info = row[10];
        dish.cooking_time = std::stoi(row[11]);
        dish.difficulty_level = row[12];
        dish.image_url = row[13];
        dish.images = row[14];
        dish.is_recommended = (row[15] == "1");
        dish.is_signature = (row[16] == "1");
        dish.is_available = (row[17] == "1");
        dish.stock_count = std::stoi(row[18]);
        dish.sales_count = std::stoi(row[19]);
        dish.rating = std::stod(row[20]);
        dish.rating_count = std::stoi(row[21]);
        dish.created_at = row[22];
        dish.updated_at = row[23];
        dishes.push_back(dish);
    }
    return dishes;
}

std::vector<Dish> RestaurantDb::getRecommendedDishes() {
    std::vector<Dish> dishes;
    if (!initialized_) return dishes;

    std::string sql = "SELECT * FROM dishes WHERE is_recommended = 1 AND is_available = 1 ORDER BY sales_count DESC";
    auto result = executeQuery(sql);
    
    for (const auto& row : result) {
        Dish dish;
        dish.id = std::stoi(row[0]);
        dish.dish_code = row[1];
        dish.dish_name = row[2];
        dish.category_id = std::stoi(row[3]);
        dish.price = std::stod(row[4]);
        dish.original_price = std::stod(row[5]);
        dish.description = row[6];
        dish.ingredients = row[7];
        dish.nutrition_info = row[8];
        dish.taste_tags = row[9];
        dish.allergen_info = row[10];
        dish.cooking_time = std::stoi(row[11]);
        dish.difficulty_level = row[12];
        dish.image_url = row[13];
        dish.images = row[14];
        dish.is_recommended = (row[15] == "1");
        dish.is_signature = (row[16] == "1");
        dish.is_available = (row[17] == "1");
        dish.stock_count = std::stoi(row[18]);
        dish.sales_count = std::stoi(row[19]);
        dish.rating = std::stod(row[20]);
        dish.rating_count = std::stoi(row[21]);
        dish.created_at = row[22];
        dish.updated_at = row[23];
        dishes.push_back(dish);
    }
    return dishes;
}

// AI推荐相关操作
bool RestaurantDb::saveAiRecommendation(const AiRecommendation& recommendation) {
    if (!initialized_) return false;

    std::string sql = R"(INSERT INTO ai_recommendations (session_id, table_id, user_id, image_base64,
                          vision_result, recommendation_result, season, meal_time,
                          people_count, customer_portraits, recommended_dishes, processing_time)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))";
    
    std::vector<std::string> params = {
        recommendation.session_id, std::to_string(recommendation.table_id), recommendation.user_id, 
        recommendation.image_base64, recommendation.vision_result, recommendation.recommendation_result,
        recommendation.season, recommendation.meal_time, std::to_string(recommendation.people_count),
        recommendation.customer_portraits, recommendation.recommended_dishes, std::to_string(recommendation.processing_time)
    };
    
    return executeSQLWithParams(sql, params);
}

// 心跳相关操作
bool RestaurantDb::updateClientHeartbeat(const ClientHeartbeat& heartbeat) {
    if (!initialized_) return false;

    std::string sql = R"(INSERT OR REPLACE INTO client_heartbeats 
        (table_id, client_id, temperature, light_intensity, humidity, noise_level, 
         battery_level, signal_strength, device_status, last_heartbeat, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP))";
    
    std::vector<std::string> params = {
        std::to_string(heartbeat.table_id), heartbeat.client_id, std::to_string(heartbeat.temperature),
        std::to_string(heartbeat.light_intensity), std::to_string(heartbeat.humidity), std::to_string(heartbeat.noise_level),
        std::to_string(heartbeat.battery_level), std::to_string(heartbeat.signal_strength), heartbeat.device_status
    };
    
    return executeSQLWithParams(sql, params);
}

std::vector<ClientHeartbeat> RestaurantDb::getActiveClients() {
    std::vector<ClientHeartbeat> clients;
    if (!initialized_) return clients;

    std::string sql = R"(SELECT * FROM client_heartbeats 
        WHERE last_heartbeat > datetime('now', '-5 minutes') 
        ORDER BY last_heartbeat DESC)";
    auto result = executeQuery(sql);
    
    for (const auto& row : result) {
        ClientHeartbeat heartbeat;
        heartbeat.id = std::stoi(row[0]);
        heartbeat.table_id = std::stoi(row[1]);
        heartbeat.client_id = row[2];
        heartbeat.temperature = std::stod(row[3]);
        heartbeat.light_intensity = std::stod(row[4]);
        heartbeat.humidity = std::stod(row[5]);
        heartbeat.noise_level = std::stod(row[6]);
        heartbeat.battery_level = std::stoi(row[7]);
        heartbeat.signal_strength = std::stoi(row[8]);
        heartbeat.device_status = row[9];
        heartbeat.last_heartbeat = row[10];
        heartbeat.created_at = row[11];
        heartbeat.updated_at = row[12];
        clients.push_back(heartbeat);
    }
    return clients;
}

// 服务呼叫相关操作
std::string RestaurantDb::createServiceCall(const ServiceCall& call) {
    if (!initialized_) return "";

    std::string call_id = generateCallId();
    std::string sql = R"(INSERT INTO service_calls (call_id, table_id, client_id, user_id, call_type,
                         priority, description, call_status)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?))";
    
    std::vector<std::string> params = {
        call_id, std::to_string(call.table_id), call.client_id, call.user_id, 
        call.call_type, call.priority, call.description, call.call_status
    };
    
    if (executeSQLWithParams(sql, params)) {
        return call_id;
    }
    return "";
}

std::vector<ServiceCall> RestaurantDb::getPendingServiceCalls() {
    std::vector<ServiceCall> calls;
    if (!initialized_) return calls;

    std::string sql = "SELECT * FROM service_calls WHERE call_status = 'pending' ORDER BY call_time ASC";
    auto result = executeQuery(sql);
    
    for (const auto& row : result) {
        ServiceCall call;
        call.id = std::stoi(row[0]);
        call.call_id = row[1];
        call.table_id = std::stoi(row[2]);
        call.client_id = row[3];
        call.user_id = row[4];
        call.call_type = row[5];
        call.priority = row[6];
        call.description = row[7];
        call.call_status = row[8];
        call.assigned_staff_id = row[9];
        call.call_time = row[10];
        call.response_time = row[11];
        call.complete_time = row[12];
        call.response_duration = row[13].empty() ? 0 : std::stoi(row[13]);
        call.service_duration = row[14].empty() ? 0 : std::stoi(row[14]);
        call.customer_rating = row[15].empty() ? 0 : std::stoi(row[15]);
        call.customer_feedback = row[16];
        call.created_at = row[17];
        call.updated_at = row[18];
        calls.push_back(call);
    }
    return calls;
}

// 辅助方法实现
std::vector<std::vector<std::string>> RestaurantDb::executeQuery(const std::string& sql) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<std::vector<std::string>> result;
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOG_F(ERROR, "SQL准备失败: %s", sqlite3_errmsg(db_));
        return result;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::vector<std::string> row;
        int column_count = sqlite3_column_count(stmt);
        for (int i = 0; i < column_count; i++) {
            const char* value = (const char*)sqlite3_column_text(stmt, i);
            row.push_back(value ? value : "");
        }
        result.push_back(row);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

std::vector<std::vector<std::string>> RestaurantDb::executeQueryWithParams(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<std::vector<std::string>> result;
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOG_F(ERROR, "SQL准备失败: %s", sqlite3_errmsg(db_));
        return result;
    }
    
    // 绑定参数
    for (size_t i = 0; i < params.size(); i++) {
        sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_STATIC);
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::vector<std::string> row;
        int column_count = sqlite3_column_count(stmt);
        for (int i = 0; i < column_count; i++) {
            const char* value = (const char*)sqlite3_column_text(stmt, i);
            row.push_back(value ? value : "");
        }
        result.push_back(row);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

bool RestaurantDb::executeSQLWithParams(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOG_F(ERROR, "SQL准备失败: %s", sqlite3_errmsg(db_));
        return false;
    }
    
    // 绑定参数
    for (size_t i = 0; i < params.size(); i++) {
        sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_STATIC);
    }
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

// 其他方法的简化实现
std::optional<Table> RestaurantDb::getTableById(int table_id) {
    if (!initialized_) return std::nullopt;
    std::string sql = "SELECT * FROM tables WHERE id = ?";
    auto result = executeQueryWithParams(sql, {std::to_string(table_id)});
    if (!result.empty()) {
        Table table;
        table.id = std::stoi(result[0][0]);
        table.table_number = result[0][1];
        table.table_name = result[0][2];
        table.seat_count = std::stoi(result[0][3]);
        table.table_type = result[0][4];
        table.status = result[0][5];
        table.location = result[0][6];
        table.qr_code = result[0][7];
        table.created_at = result[0][8];
        table.updated_at = result[0][9];
        return table;
    }
    return std::nullopt;
}

std::vector<Dish> RestaurantDb::getDishesByCategory(int category_id) {
    std::vector<Dish> dishes;
    if (!initialized_) return dishes;
    std::string sql = "SELECT * FROM dishes WHERE category_id = ? AND is_available = 1 ORDER BY dish_name";
    auto result = executeQueryWithParams(sql, {std::to_string(category_id)});
    // 实现类似getAllDishes的逻辑
    return dishes;
}

std::vector<Dish> RestaurantDb::getSignatureDishes() {
    std::vector<Dish> dishes;
    if (!initialized_) return dishes;
    std::string sql = "SELECT * FROM dishes WHERE is_signature = 1 AND is_available = 1 ORDER BY sales_count DESC";
    auto result = executeQuery(sql);
    // 实现类似getAllDishes的逻辑
    return dishes;
}

std::optional<Dish> RestaurantDb::getDishById(int dish_id) {
    if (!initialized_) return std::nullopt;
    std::string sql = "SELECT * FROM dishes WHERE id = ?";
    auto result = executeQueryWithParams(sql, {std::to_string(dish_id)});
    // 实现类似getUserById的逻辑
    return std::nullopt;
}

std::optional<Dish> RestaurantDb::getDishByCode(const std::string& dish_code) {
    if (!initialized_) return std::nullopt;
    std::string sql = "SELECT * FROM dishes WHERE dish_code = ?";
    auto result = executeQueryWithParams(sql, {dish_code});
    // 实现类似getUserById的逻辑
    return std::nullopt;
}

bool RestaurantDb::updateDishStock(int dish_id, int stock_count) {
    if (!initialized_) return false;
    std::string sql = "UPDATE dishes SET stock_count = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    return executeSQLWithParams(sql, {std::to_string(stock_count), std::to_string(dish_id)});
}

std::string RestaurantDb::createOrder(const Order& order) {
    if (!initialized_) return "";
    std::string order_no = generateOrderNo();
    std::string sql = R"(INSERT INTO orders (order_no, table_id, user_id, order_type, people_count,
                          total_amount, discount_amount, final_amount, payment_method,
                          payment_status, order_status, special_requirements, estimated_time)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))";
    
    std::vector<std::string> params = {
        order_no, std::to_string(order.table_id), order.user_id, order.order_type, std::to_string(order.people_count),
        std::to_string(order.total_amount), std::to_string(order.discount_amount), std::to_string(order.final_amount),
        order.payment_method, order.payment_status, order.order_status, order.special_requirements, std::to_string(order.estimated_time)
    };
    
    if (executeSQLWithParams(sql, params)) {
        return order_no;
    }
    return "";
}

bool RestaurantDb::addOrderItem(const OrderItem& item) {
    if (!initialized_) return false;
    std::string sql = R"(INSERT INTO order_items (order_id, dish_id, dish_name, dish_price, quantity, subtotal, special_requirements, item_status)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?))";
    
    std::vector<std::string> params = {
        std::to_string(item.order_id), std::to_string(item.dish_id), item.dish_name, std::to_string(item.dish_price),
        std::to_string(item.quantity), std::to_string(item.subtotal), item.special_requirements, item.item_status
    };
    
    return executeSQLWithParams(sql, params);
}

std::optional<Order> RestaurantDb::getOrderByNo(const std::string& order_no) {
    if (!initialized_) return std::nullopt;
    std::string sql = "SELECT * FROM orders WHERE order_no = ?";
    auto result = executeQueryWithParams(sql, {order_no});
    // 实现类似getUserById的逻辑
    return std::nullopt;
}

std::vector<OrderItem> RestaurantDb::getOrderItems(int order_id) {
    std::vector<OrderItem> items;
    if (!initialized_) return items;
    std::string sql = "SELECT * FROM order_items WHERE order_id = ? ORDER BY id";
    auto result = executeQueryWithParams(sql, {std::to_string(order_id)});
    // 实现类似getAllTables的逻辑
    return items;
}

bool RestaurantDb::updateOrderStatus(const std::string& order_no, const std::string& status) {
    if (!initialized_) return false;
    std::string sql = "UPDATE orders SET order_status = ?, updated_at = CURRENT_TIMESTAMP WHERE order_no = ?";
    return executeSQLWithParams(sql, {status, order_no});
}

bool RestaurantDb::updateOrderPaymentStatus(const std::string& order_no, const std::string& payment_status) {
    if (!initialized_) return false;
    std::string sql = "UPDATE orders SET payment_status = ?, updated_at = CURRENT_TIMESTAMP WHERE order_no = ?";
    return executeSQLWithParams(sql, {payment_status, order_no});
}

std::optional<AiRecommendation> RestaurantDb::getAiRecommendation(const std::string& session_id) {
    if (!initialized_) return std::nullopt;
    std::string sql = "SELECT * FROM ai_recommendations WHERE session_id = ?";
    auto result = executeQueryWithParams(sql, {session_id});
    // 实现类似getUserById的逻辑
    return std::nullopt;
}

bool RestaurantDb::updateAiRecommendationFeedback(const std::string& session_id, int score, const std::string& comment) {
    if (!initialized_) return false;
    std::string sql = "UPDATE ai_recommendations SET feedback_score = ?, feedback_comment = ?, updated_at = CURRENT_TIMESTAMP WHERE session_id = ?";
    return executeSQLWithParams(sql, {std::to_string(score), comment, session_id});
}

std::optional<ClientHeartbeat> RestaurantDb::getClientHeartbeat(int table_id, const std::string& client_id) {
    if (!initialized_) return std::nullopt;
    std::string sql = "SELECT * FROM client_heartbeats WHERE table_id = ? AND client_id = ?";
    auto result = executeQueryWithParams(sql, {std::to_string(table_id), client_id});
    // 实现类似getUserById的逻辑
    return std::nullopt;
}

bool RestaurantDb::cleanupInactiveClients(int timeout_seconds) {
    if (!initialized_) return false;
    std::string sql = "DELETE FROM client_heartbeats WHERE last_heartbeat < datetime('now', '-' || ? || ' seconds')";
    return executeSQLWithParams(sql, {std::to_string(timeout_seconds)});
}

std::vector<ServiceCall> RestaurantDb::getServiceCallsByTable(int table_id) {
    std::vector<ServiceCall> calls;
    if (!initialized_) return calls;
    std::string sql = "SELECT * FROM service_calls WHERE table_id = ? ORDER BY call_time DESC LIMIT 10";
    auto result = executeQueryWithParams(sql, {std::to_string(table_id)});
    // 实现类似getPendingServiceCalls的逻辑
    return calls;
}

bool RestaurantDb::updateServiceCallStatus(const std::string& call_id, const std::string& status) {
    if (!initialized_) return false;
    std::string sql = "UPDATE service_calls SET call_status = ?, updated_at = CURRENT_TIMESTAMP WHERE call_id = ?";
    return executeSQLWithParams(sql, {status, call_id});
}

bool RestaurantDb::assignServiceCall(const std::string& call_id, const std::string& staff_id) {
    if (!initialized_) return false;
    std::string sql = "UPDATE service_calls SET assigned_staff_id = ?, call_status = 'assigned', response_time = CURRENT_TIMESTAMP, updated_at = CURRENT_TIMESTAMP WHERE call_id = ?";
    return executeSQLWithParams(sql, {staff_id, call_id});
}

bool RestaurantDb::completeServiceCall(const std::string& call_id, int rating, const std::string& feedback) {
    if (!initialized_) return false;
    std::string sql = "UPDATE service_calls SET call_status = 'completed', complete_time = CURRENT_TIMESTAMP, customer_rating = ?, customer_feedback = ?, updated_at = CURRENT_TIMESTAMP WHERE call_id = ?";
    return executeSQLWithParams(sql, {std::to_string(rating), feedback, call_id});
}

bool RestaurantDb::updateUser(const User& user) {
    if (!initialized_) return false;
    std::string sql = R"(UPDATE users SET nickname = ?, phone = ?, email = ?, avatar_url = ?, 
                         gender = ?, age_grades = ?, body_type = ?, 
                         taste_preference = ?, dietary_restrictions = ?, updated_at = CURRENT_TIMESTAMP
        WHERE user_id = ?)";
    
    std::vector<std::string> params = {
        user.nickname, user.phone, user.email, user.avatar_url, user.gender,
        user.age_grades, user.body_type, user.taste_preference, user.dietary_restrictions, user.user_id
    };
    
    return executeSQLWithParams(sql, params);
}

bool RestaurantDb::deleteUser(const std::string& user_id) {
    if (!initialized_) return false;
    std::string sql = "DELETE FROM users WHERE user_id = ?";
    return executeSQLWithParams(sql, {user_id});
}

// 统计相关操作
int RestaurantDb::getTodayOrderCount() {
    if (!initialized_) return 0;
    std::string sql = "SELECT COUNT(*) FROM orders WHERE DATE(created_at) = DATE('now')";
    auto result = executeQuery(sql);
    return result.empty() ? 0 : std::stoi(result[0][0]);
}

double RestaurantDb::getTodayRevenue() {
    if (!initialized_) return 0.0;
    std::string sql = "SELECT SUM(final_amount) FROM orders WHERE DATE(created_at) = DATE('now') AND payment_status = 'paid'";
    auto result = executeQuery(sql);
    return result.empty() ? 0.0 : std::stod(result[0][0]);
}

std::vector<std::pair<std::string, int>> RestaurantDb::getPopularDishes(int limit) {
    std::vector<std::pair<std::string, int>> popular_dishes;
    if (!initialized_) return popular_dishes;
    std::string sql = "SELECT dish_name, sales_count FROM dishes ORDER BY sales_count DESC LIMIT ?";
    auto result = executeQueryWithParams(sql, {std::to_string(limit)});
    for (const auto& row : result) {
        popular_dishes.push_back({row[0], std::stoi(row[1])});
    }
    return popular_dishes;
}

std::vector<std::pair<std::string, double>> RestaurantDb::getTableUtilization() {
    std::vector<std::pair<std::string, double>> utilization;
    if (!initialized_) return utilization;
    std::string sql = R"(SELECT table_number, 
        CASE WHEN status = 'occupied' THEN 1.0 ELSE 0.0 END as utilization
        FROM tables ORDER BY table_number)";
    auto result = executeQuery(sql);
    for (const auto& row : result) {
        utilization.push_back({row[0], std::stod(row[1])});
    }
    return utilization;
}

} // namespace WisdomRestaurant
