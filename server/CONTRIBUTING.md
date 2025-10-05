# 贡献指南

感谢您对智能餐厅服务端系统的关注！我们欢迎所有形式的贡献。

## 🚀 快速开始

### 1. Fork 项目
点击项目页面右上角的 "Fork" 按钮，将项目复制到您的GitHub账户。

### 2. 克隆项目
```bash
git clone https://github.com/yourusername/myWisdomRestaurant.git
cd myWisdomRestaurant/server
```

### 3. 创建功能分支
```bash
git checkout -b feature/your-feature-name
```

### 4. 进行开发
- 编写代码
- 添加测试
- 更新文档

### 5. 提交更改
```bash
git add .
git commit -m "Add: 描述您的更改"
```

### 6. 推送分支
```bash
git push origin feature/your-feature-name
```

### 7. 创建 Pull Request
在GitHub上创建Pull Request，详细描述您的更改。

## 📋 贡献类型

### 🐛 Bug修复
- 修复现有功能的问题
- 改进错误处理
- 优化性能问题

### ✨ 新功能
- 添加新的API接口
- 实现新的业务逻辑
- 集成新的第三方服务

### 📚 文档改进
- 完善API文档
- 添加使用示例
- 改进README说明

### 🧪 测试
- 添加单元测试
- 编写集成测试
- 改进测试覆盖率

### 🎨 代码优化
- 重构代码结构
- 优化算法性能
- 改进代码可读性

## 🔧 开发环境设置

### 系统要求
- Linux (推荐Ubuntu 20.04+)
- GCC 7.0+ 或 Clang 5.0+
- CMake 3.16+
- Git

### 安装依赖
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake libcurl4-openssl-dev libssl-dev

# CentOS/RHEL
sudo yum install gcc-c++ cmake libcurl-devel openssl-devel
```

### 编译项目
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### 运行测试
```bash
cd test
make
./test_new_architecture.sh
```

## 📝 代码规范

### C++编码规范
- 使用C++17标准
- 4空格缩进，不使用Tab
- 函数和类名使用PascalCase
- 变量名使用camelCase
- 常量使用UPPER_CASE

### 命名规范
```cpp
// 类名
class RecommendationController {
public:
    // 公共方法
    void handleRecommendation();
    
private:
    // 私有成员变量
    std::shared_ptr<AiService> ai_service_;
};

// 函数名
void processImageData();
bool validateUserInput();
```

### 注释规范
```cpp
/**
 * @brief 处理智能推荐请求
 * @param request HTTP请求对象
 * @param response HTTP响应对象
 * @return 无返回值
 */
void RecommendationController::handleRecommendation(
    const httplib::Request& request, 
    httplib::Response& response) {
    
    // 设置CORS头
    setCorsHeaders(response);
    
    // 解析请求参数
    std::string image_base64 = request.get_param_value("image");
    
    // TODO: 实现推荐逻辑
}
```

### 错误处理
```cpp
try {
    // 可能抛出异常的操作
    auto result = processData();
    return result;
} catch (const std::exception& e) {
    LOG_F(ERROR, "处理数据失败: %s", e.what());
    return std::nullopt;
}
```

## 🧪 测试要求

### 单元测试
为每个新功能编写单元测试：

```cpp
#include <gtest/gtest.h>
#include "api/RecommendationController.h"

class RecommendationControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试初始化
    }
    
    void TearDown() override {
        // 测试清理
    }
};

TEST_F(RecommendationControllerTest, HandleRecommendationSuccess) {
    // 测试成功场景
    EXPECT_TRUE(true);
}

TEST_F(RecommendationControllerTest, HandleRecommendationFailure) {
    // 测试失败场景
    EXPECT_FALSE(false);
}
```

### 集成测试
确保新功能与现有系统兼容：

```bash
# 运行完整测试套件
./test_new_architecture.sh

# 运行特定测试
./camera_recommendation_test -t T001
```

## 📚 文档要求

### API文档
为新的API接口添加文档：

```markdown
#### 新接口名称
```http
POST /api/v1/new-endpoint
Content-Type: application/json
```

**请求参数**:
```json
{
  "param1": "value1",
  "param2": "value2"
}
```

**响应示例**:
```json
{
  "code": 200,
  "message": "成功",
  "data": {
    "result": "success"
  }
}
```
```

### 代码文档
为复杂的函数添加详细注释：

```cpp
/**
 * @brief 处理图像数据并生成推荐结果
 * 
 * 该函数接收Base64编码的图像数据，通过AI服务进行视觉识别，
 * 然后基于识别结果生成个性化的菜品推荐。
 * 
 * @param image_base64 Base64编码的图像数据
 * @param table_number 餐桌号码
 * @param user_id 用户ID
 * @param season 季节信息（可选）
 * @param meal_time 用餐时间（可选）
 * @return 推荐结果，包含推荐菜品列表和置信度
 * 
 * @throws std::invalid_argument 当图像数据为空时
 * @throws std::runtime_error 当AI服务调用失败时
 * 
 * @note 该函数是异步的，处理时间可能较长
 * @warning 图像数据大小不应超过10MB
 */
RecommendationResult processRecommendation(
    const std::string& image_base64,
    const std::string& table_number,
    const std::string& user_id,
    const std::string& season = "",
    const std::string& meal_time = "");
```

## 🔍 代码审查

### Pull Request 要求
- 标题清晰描述更改内容
- 详细描述更改原因和影响
- 包含相关的测试用例
- 更新相关文档

### 审查标准
- 代码符合项目规范
- 功能正确实现
- 性能影响可接受
- 安全性考虑充分
- 文档更新完整

### 审查流程
1. 自动检查（CI/CD）
2. 代码审查
3. 功能测试
4. 性能测试
5. 合并到主分支

## 🐛 报告问题

### Bug报告模板
```markdown
**Bug描述**
简要描述遇到的问题

**重现步骤**
1. 执行操作A
2. 执行操作B
3. 出现错误C

**预期行为**
描述期望的正确行为

**实际行为**
描述实际发生的行为

**环境信息**
- 操作系统: Ubuntu 20.04
- 编译器: GCC 9.4.0
- 版本: v2.0.0

**附加信息**
添加任何其他相关信息
```

### 功能请求模板
```markdown
**功能描述**
详细描述希望添加的功能

**使用场景**
描述该功能的使用场景

**实现建议**
如果有实现建议，请提供

**优先级**
高/中/低
```

## 📞 联系方式

- **项目维护者**: [Your Name]
- **邮箱**: [your.email@example.com]
- **GitHub**: [@yourusername](https://github.com/yourusername)
- **讨论区**: [GitHub Discussions](https://github.com/yourusername/myWisdomRestaurant/discussions)

## 📄 许可证

通过贡献代码，您同意您的贡献将在MIT许可证下发布。

## 🙏 致谢

感谢所有贡献者的努力！您的贡献让这个项目变得更好。

---

**再次感谢您的贡献！** 🎉
