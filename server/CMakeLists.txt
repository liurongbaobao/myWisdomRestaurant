cmake_minimum_required(VERSION 3.16)
project(WisdomRestaurantServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /utf-8")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/httplib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sqlite3)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/loguru)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/rapidjson)

# 查找cURL（用于AI服务）
find_package(CURL REQUIRED)

# 检查依赖是否找到
if(NOT CURL_FOUND)
    message(FATAL_ERROR "cURL library not found")
endif()

# 添加包含目录
include_directories(${CURL_INCLUDE_DIRS})

# 收集源文件
file(GLOB_RECURSE PROJECT_SOURCES
    "src/*.cpp"
)

# 收集所有源文件

# 添加sqlite3和loguru源文件
set(SQLITE3_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.c
)

set(LOGURU_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/loguru/loguru.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${SQLITE3_SOURCES}
    ${LOGURU_SOURCES}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    ${CURL_LIBRARIES}
)

# Windows特定设置
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ws2_32
        wldap32
        crypt32
        normaliz
    )
    
    # 设置运行时库
    if(MSVC)
        set_property(TARGET ${PROJECT_NAME} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()

# Linux特定设置
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME}
        pthread
        dl
    )
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 复制配置文件
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/sql/create_tables.sql
    DESTINATION sql
)

# 打印配置信息
message(STATUS "=== 智能餐厅服务端编译配置 ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "HTTP Library: httplib (header-only)")
message(STATUS "Database: SQLite3 (embedded)")
message(STATUS "Logging: loguru")
message(STATUS "JSON: RapidJSON")
message(STATUS "CURL Include: ${CURL_INCLUDE_DIRS}")
message(STATUS "CURL Libraries: ${CURL_LIBRARIES}")
message(STATUS "================================")
