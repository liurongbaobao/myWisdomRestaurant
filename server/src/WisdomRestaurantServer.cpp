#include "httplib.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "loguru.hpp"
#include "ai/AiService.h"
#include "db/RestaurantDb.h"
#include "api/RecommendationController.h"

#include <iostream>
#include <memory>
#include <signal.h>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace WisdomRestaurant;

// 全局服务器实例，用于信号处理
std::unique_ptr<httplib::Server> g_server;

// 信号处理函数
void signalHandler(int signal) {
    LOG_F(INFO, "收到信号 %d，正在关闭服务器...", signal);
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

// 打印启动信息
void printStartupInfo() {
    std::cout << R"(
    ╔══════════════════════════════════════════════════════════════╗
    ║                    智能餐厅服务端系统                          ║
    ║                  Wisdom Restaurant Server                    ║
    ╠══════════════════════════════════════════════════════════════╣
    ║  版本: 2.0.0                                                 ║
    ║  技术栈: C++, httplib, SQLite3, JSON, cURL, loguru          ║
    ║  功能: 智能菜品推荐, 通信协议, 数据库管理                      ║
    ╚══════════════════════════════════════════════════════════════╝
    )" << std::endl;
}

// 设置CORS头
void setCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    res.set_header("Access-Control-Allow-Credentials", "true");
}

// 处理OPTIONS请求
void handleOptions(const httplib::Request& req, httplib::Response& res) {
    (void)req; // 抑制未使用参数警告
    setCorsHeaders(res);
    res.status = 200;
}

// 构建JSON响应
std::string buildJsonResponse(int code, const std::string& message, const std::string& data = "{}") {
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();
    
    doc.AddMember("code", code, alloc);
    doc.AddMember("message", rapidjson::Value(message.c_str(), alloc), alloc);
    
    rapidjson::Document data_doc;
    data_doc.Parse(data.c_str());
    doc.AddMember("data", data_doc, alloc);
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    return buffer.GetString();
}

// 配置路由
void setupRoutes(httplib::Server& server, 
                std::shared_ptr<RecommendationController> rec_controller) {
    
    LOG_F(INFO, "配置API路由...");

    // 健康检查接口
    server.Get("/api/v1/health", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "服务器运行正常", 
            R"({"status":"healthy","timestamp":)" + std::to_string(std::time(nullptr)) + "}"), 
            "application/json; charset=utf-8");
    });

    // 智能推荐相关路由
    server.Post("/api/v1/recommendation", [rec_controller](const httplib::Request& req, httplib::Response& res) {
        rec_controller->handleRecommendation(req, res);
        });

    server.Get("/api/v1/recommendation/history", [rec_controller](const httplib::Request& req, httplib::Response& res) {
        rec_controller->handleGetRecommendationHistory(req, res);
        });

    server.Post("/api/v1/recommendation/feedback", [rec_controller](const httplib::Request& req, httplib::Response& res) {
        rec_controller->handleRecommendationFeedback(req, res);
        });

    server.Get("/api/v1/dishes/recommended", [rec_controller](const httplib::Request& req, httplib::Response& res) {
        rec_controller->handleGetRecommendedDishes(req, res);
        });

    // 通信协议相关路由
    server.Post("/api/v1/heartbeat", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "心跳功能正在适配中"), "application/json; charset=utf-8");
    });

    server.Post("/api/v1/environment", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "环境数据功能正在适配中"), "application/json; charset=utf-8");
    });

    server.Post("/api/v1/service/call", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "服务呼叫功能正在适配中"), "application/json; charset=utf-8");
    });

    server.Get("/api/v1/service/calls", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "服务呼叫列表功能正在适配中"), "application/json; charset=utf-8");
    });

    server.Post("/api/v1/service/response", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "服务响应功能正在适配中"), "application/json; charset=utf-8");
    });

    server.Post("/api/v1/service/complete", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "服务完成功能正在适配中"), "application/json; charset=utf-8");
        });

    // 餐桌管理相关路由
    server.Get("/api/v1/tables/status", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "餐桌状态功能正在适配中"), "application/json; charset=utf-8");
    });

    server.Post("/api/v1/tables/status", [](const httplib::Request& req, httplib::Response& res) {
        (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "更新餐桌状态功能正在适配中"), "application/json; charset=utf-8");
    });

    server.Get("/api/v1/clients/active", [](const httplib::Request& req, httplib::Response& res) {
            (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        res.set_content(buildJsonResponse(200, "活跃客户端功能正在适配中"), "application/json; charset=utf-8");
        });

    // 根路径 - 显示API文档
    server.Get("/", [](const httplib::Request& req, httplib::Response& res) {
            (void)req; // 抑制未使用参数警告
        setCorsHeaders(res);
        std::string html = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="utf-8">
    <title>智能餐厅服务端 v2.0</title>
                    <style>
                        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
                        .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
                        h1 { color: #333; text-align: center; }
                        .api-list { margin-top: 30px; }
                        .api-item { margin: 10px 0; padding: 10px; background: #f8f9fa; border-radius: 5px; }
                        .method { font-weight: bold; color: #007bff; }
                        .path { color: #28a745; }
                        .desc { color: #666; margin-left: 10px; }
        .status { color: #ffc107; font-weight: bold; }
                    </style>
                </head>
                <body>
                    <div class="container">
        <h1>🍽️ 智能餐厅服务端系统 v2.0</h1>
                        <p>欢迎使用智能餐厅服务端API系统！</p>
        <p class="status">⚠️ 系统正在适配新架构 (httplib + SQLite3)</p>
                        
                        <div class="api-list">
                            <h3>📋 API接口列表</h3>
                            
            <h4>🔧 系统</h4>
            <div class="api-item">
                <span class="method">GET</span> <span class="path">/api/v1/health</span>
                <span class="desc">健康检查 ✅</span>
            </div>
            
            <h4>🤖 智能推荐 (适配中)</h4>
                            <div class="api-item">
                                <span class="method">POST</span> <span class="path">/api/v1/recommendation</span>
                <span class="desc">智能菜品推荐 🔄</span>
                            </div>
                            <div class="api-item">
                                <span class="method">GET</span> <span class="path">/api/v1/recommendation/history</span>
                <span class="desc">获取推荐历史 🔄</span>
                            </div>
                            <div class="api-item">
                                <span class="method">POST</span> <span class="path">/api/v1/recommendation/feedback</span>
                <span class="desc">推荐反馈 🔄</span>
                            </div>
                            <div class="api-item">
                                <span class="method">GET</span> <span class="path">/api/v1/dishes/recommended</span>
                <span class="desc">获取推荐菜品 🔄</span>
                            </div>
                            
            <h4>📡 通信协议 (适配中)</h4>
                            <div class="api-item">
                                <span class="method">POST</span> <span class="path">/api/v1/heartbeat</span>
                <span class="desc">客户端心跳 🔄</span>
                            </div>
                            <div class="api-item">
                                <span class="method">POST</span> <span class="path">/api/v1/environment</span>
                <span class="desc">环境数据上报 🔄</span>
                            </div>
                            <div class="api-item">
                                <span class="method">POST</span> <span class="path">/api/v1/service/call</span>
                <span class="desc">呼叫服务员 🔄</span>
                            </div>
                            <div class="api-item">
                                <span class="method">GET</span> <span class="path">/api/v1/service/calls</span>
                <span class="desc">获取服务呼叫列表 🔄</span>
                            </div>
                            
            <h4>🏪 餐桌管理 (适配中)</h4>
                            <div class="api-item">
                                <span class="method">GET</span> <span class="path">/api/v1/tables/status</span>
                <span class="desc">获取餐桌状态 🔄</span>
                            </div>
                            <div class="api-item">
                                <span class="method">POST</span> <span class="path">/api/v1/tables/status</span>
                <span class="desc">更新餐桌状态 🔄</span>
                            </div>
                            <div class="api-item">
                                <span class="method">GET</span> <span class="path">/api/v1/clients/active</span>
                <span class="desc">获取活跃客户端 🔄</span>
                            </div>
                        </div>
                        
                        <p style="text-align: center; margin-top: 30px; color: #666;">
                            服务器运行时间: <span id="uptime"></span>
                        </p>
                    </div>
                    
                    <script>
                        const startTime = Date.now();
                        function updateUptime() {
                            const uptime = Math.floor((Date.now() - startTime) / 1000);
                            const hours = Math.floor(uptime / 3600);
                            const minutes = Math.floor((uptime % 3600) / 60);
                            const seconds = uptime % 60;
                            document.getElementById('uptime').textContent = 
                                hours + '小时 ' + minutes + '分钟 ' + seconds + '秒';
                        }
                        setInterval(updateUptime, 1000);
                        updateUptime();
                    </script>
                </body>
                </html>
        )";
        res.set_content(html, "text/html; charset=utf-8");
        });

    // 处理OPTIONS请求
    server.Options(".*", handleOptions);

    LOG_F(INFO, "路由配置完成！");
}

int main(int argc, char* argv[]) {
    // 初始化日志系统
    loguru::init(argc, argv);
    
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 打印启动信息
    printStartupInfo();

    try {
        // 读取配置参数
        const char* port_env = std::getenv("SERVER_PORT");
        int port = port_env ? std::atoi(port_env) : 8080;

        const char* db_path_env = std::getenv("DB_PATH");
        std::string db_path = db_path_env ? db_path_env : "wisdom_restaurant.db";

        LOG_F(INFO, "服务器配置:");
        LOG_F(INFO, "  端口: %d", port);
        LOG_F(INFO, "  数据库: %s", db_path.c_str());

        // 初始化AI服务
        LOG_F(INFO, "初始化AI服务...");
        auto ai_service = std::make_shared<AiService>();
        if (!ai_service->initialize()) {
            LOG_F(ERROR, "AI服务初始化失败！");
            return 1;
        }

        // 初始化数据库
        LOG_F(INFO, "初始化SQLite数据库连接...");
        auto db = std::make_shared<RestaurantDb>();
        if (!db->initialize(db_path)) {
            LOG_F(ERROR, "数据库初始化失败！");
            return 1;
        }

        // 创建控制器
        LOG_F(INFO, "创建API控制器...");
        auto rec_controller = std::make_shared<RecommendationController>(ai_service, db);

        // 创建HTTP服务器
        LOG_F(INFO, "创建HTTP服务器...");
        g_server = std::make_unique<httplib::Server>();
        
        // 配置路由
        setupRoutes(*g_server, rec_controller);

        // 启动服务器
        LOG_F(INFO, "🚀 启动服务器...");
        LOG_F(INFO, "服务器地址: http://localhost:%d", port);
        LOG_F(INFO, "API文档: http://localhost:%d/", port);
        LOG_F(INFO, "健康检查: http://localhost:%d/api/v1/health", port);
        LOG_F(INFO, "按 Ctrl+C 停止服务器");

        // 启动服务器（阻塞）
        if (!g_server->listen("0.0.0.0", port)) {
            LOG_F(ERROR, "服务器启动失败！");
            return 1;
        }

    } catch (const std::exception& e) {
        LOG_F(ERROR, "服务器启动失败: %s", e.what());
        return 1;
    }

    return 0;
}
