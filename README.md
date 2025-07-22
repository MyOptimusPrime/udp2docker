# UDP2Docker - Windows到Docker的UDP通信库

一个专为Windows平台设计的C++库，用于通过UDP协议向Docker容器环境发送消息。该项目采用现代C++工程实践，具有良好的模块化设计，便于团队协作和二次开发。

## 🚀 项目特性

### 核心功能
- **UDP通信**: 支持可靠的UDP数据包发送和接收
- **消息协议**: 内置消息序列化/反序列化和完整性校验
- **异步通信**: 支持异步发送和接收，提高性能
- **配置管理**: 灵活的配置文件支持（JSON/INI格式）
- **日志系统**: 多级别、多输出目标的日志记录
- **错误处理**: 完善的错误码系统和异常处理

### 工程特性
- **跨平台兼容**: 主要针对Windows，同时兼容Linux
- **模块化设计**: 清晰的模块划分，便于维护和扩展
- **线程安全**: 所有核心组件都是线程安全的
- **现代C++**: 使用C++17标准，采用RAII和智能指针
- **完整文档**: 详细的代码注释和使用文档

## 📋 系统要求

### Windows环境
- Windows 7 或更高版本
- **选项1**: Visual Studio 2017 或更高版本（支持C++17）
- **选项2**: MSYS2 + MinGW-w64 工具链（推荐轻量级方案）
- CMake 3.15 或更高版本
- Git（用于下载源码）

> **注意**: 如果你使用MSYS2环境，请参考 [MSYS2_SETUP.md](MSYS2_SETUP.md) 获取详细配置指南。

### Linux环境（可选）
- GCC 7.0+ 或 Clang 5.0+
- CMake 3.15+
- 标准C++17库支持

## 🛠️ 快速开始

### 1. 克隆项目
```bash
git clone https://github.com/your-org/udp2docker.git
cd udp2docker
```

### 2. 构建项目

#### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

#### Windows (MinGW)
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

#### Linux
```bash
mkdir build
cd build
cmake ..
make -j4
```

### 3. 运行示例
```bash
# Windows
.\bin\udp2docker.exe

# Linux
./bin/udp2docker
```

### 4. 运行测试
```bash
# Windows
.\bin\udp2docker_test.exe

# Linux
./bin/udp2docker_test
```

## 💻 使用示例

### 基本UDP发送
```cpp
#include "udp2docker/udp_client.h"
#include "udp2docker/logger.h"

using namespace udp2docker;

int main() {
    // 配置UDP客户端
    UdpConfig config;
    config.server_host = "127.0.0.1";
    config.server_port = 8888;
    config.timeout_ms = 5000;
    
    // 创建客户端
    UdpClient client(config);
    
    // 初始化
    if (client.initialize() != ErrorCode::SUCCESS) {
        LOG_ERROR("客户端初始化失败");
        return -1;
    }
    
    // 发送消息
    std::string message = "Hello Docker!";
    auto result = client.send_string(message);
    
    if (result == ErrorCode::SUCCESS) {
        LOG_INFO("消息发送成功");
    } else {
        LOG_ERROR("消息发送失败");
    }
    
    client.close();
    return 0;
}
```

### 使用消息协议
```cpp
#include "udp2docker/message_protocol.h"
#include "udp2docker/udp_client.h"

using namespace udp2docker;

int main() {
    MessageProtocol protocol;
    UdpClient client;
    
    client.initialize();
    
    // 创建控制消息
    auto message = protocol.create_control_message("docker ps", Priority::HIGH);
    
    // 序列化并发送
    auto serialized = protocol.serialize(message);
    if (serialized) {
        client.send(*serialized);
    }
    
    return 0;
}
```

### 异步接收消息
```cpp
#include "udp2docker/udp_client.h"

using namespace udp2docker;

int main() {
    UdpConfig config;
    config.server_host = "0.0.0.0";  // 监听所有接口
    config.server_port = 9999;
    
    UdpClient client(config);
    client.initialize();
    
    // 设置回调函数
    MessageCallback callback = [](const buffer_t& data, const string_t& from_host, int from_port) {
        std::string message(data.begin(), data.end());
        std::cout << "收到消息: " << message << " 来自 " << from_host << std::endl;
    };
    
    // 启动异步接收
    client.start_receive_async(callback);
    
    // 等待消息...
    std::this_thread::sleep_for(std::chrono::seconds(60));
    
    client.stop_receive_async();
    return 0;
}
```

## 🏗️ 项目结构

```
udp2docker/
├── CMakeLists.txt          # 构建配置
├── README.md               # 项目说明
├── include/                # 头文件目录
│   └── udp2docker/
│       ├── common.h        # 公共定义
│       ├── udp_client.h    # UDP客户端
│       ├── message_protocol.h # 消息协议
│       ├── config_manager.h   # 配置管理
│       └── logger.h        # 日志系统
├── src/                    # 源文件目录
│   ├── udp_client.cpp
│   ├── message_protocol.cpp
│   ├── config_manager.cpp
│   └── logger.cpp
├── examples/               # 示例代码
│   └── main.cpp
├── tests/                  # 测试代码
│   └── test_main.cpp
├── config/                 # 配置文件
│   └── default.ini
├── scripts/                # 构建脚本
├── docker/                 # Docker相关文件
└── docs/                   # 文档目录
```

## ⚙️ 配置说明

项目支持通过配置文件进行灵活配置。默认配置文件位于 `config/default.ini`：

```ini
[server]
host = 127.0.0.1
port = 8888

[client]  
timeout_ms = 5000
max_retries = 3
enable_keep_alive = true

[log]
level = INFO
file = logs/udp2docker.log
console = true
```

### 配置项说明

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| server.host | UDP服务器地址 | 127.0.0.1 |
| server.port | UDP服务器端口 | 8888 |
| client.timeout_ms | 客户端超时时间(ms) | 5000 |
| client.max_retries | 最大重试次数 | 3 |
| log.level | 日志级别 | INFO |
| log.file | 日志文件路径 | logs/udp2docker.log |

## 🐋 Docker集成

### 创建Docker服务器
项目包含了一个简单的Docker服务器示例，用于接收UDP消息：

```bash
# 构建Docker镜像
cd docker
docker build -t udp2docker-server .

# 运行容器
docker run -p 8888:8888/udp udp2docker-server
```

### Docker Compose
```yaml
version: '3.8'
services:
  udp-server:
    build: ./docker
    ports:
      - "8888:8888/udp"
    volumes:
      - ./logs:/app/logs
```

## 🧪 测试

### 单元测试
```bash
# 运行所有测试
./bin/udp2docker_test

# 带详细输出的测试
./bin/udp2docker_test --verbose
```

### 集成测试
1. 启动Docker容器服务器：
   ```bash
   docker run -p 8888:8888/udp udp2docker-server
   ```

2. 运行客户端示例：
   ```bash
   ./bin/udp2docker
   ```

## 📚 API参考

### UdpClient 类
主要的UDP通信客户端类。

#### 构造函数
```cpp
explicit UdpClient(const UdpConfig& config = UdpConfig{});
```

#### 主要方法
- `ErrorCode initialize()` - 初始化客户端
- `void close()` - 关闭连接
- `bool is_connected() const` - 检查连接状态
- `ErrorCode send(const buffer_t& data, ...)` - 发送数据
- `ErrorCode send_string(const string_t& message, ...)` - 发送字符串
- `Result<size_t> receive(...)` - 接收数据
- `ErrorCode start_receive_async(...)` - 启动异步接收

### MessageProtocol 类
消息协议处理类。

#### 主要方法
- `Message create_heartbeat()` - 创建心跳消息
- `Message create_data_message(...)` - 创建数据消息
- `Message create_control_message(...)` - 创建控制消息
- `std::optional<buffer_t> serialize(const Message&)` - 序列化消息
- `std::optional<Message> deserialize(const buffer_t&)` - 反序列化消息

### ConfigManager 类
配置管理类。

#### 主要方法
- `ErrorCode load_config(const string_t& file)` - 加载配置文件
- `ErrorCode save_config(...)` - 保存配置
- `string_t get_string(const string_t& key, ...)` - 获取字符串配置
- `int get_int(const string_t& key, ...)` - 获取整数配置
- `bool get_bool(const string_t& key, ...)` - 获取布尔配置

## 🔧 高级特性

### 消息压缩和加密
```cpp
MessageProtocol protocol;

// 启用压缩
protocol.set_compression_enabled(true);

// 启用加密
protocol.set_encryption_enabled(true, "your-encryption-key");
```

### 自定义日志格式
```cpp
Logger logger("MyApp");

// 设置自定义格式
logger.set_pattern("[%d] [%l] [%F:%L] %m");

// 启用异步日志
logger.enable_async(1000);
```

### 配置变更回调
```cpp
ConfigManager config;

// 注册变更回调
config.register_change_callback([](const string_t& key, const ConfigItem& old_val, const ConfigItem& new_val) {
    std::cout << "配置项 " << key << " 从 " << old_val.value << " 变更为 " << new_val.value << std::endl;
});
```

## 🤝 贡献指南

我们欢迎所有形式的贡献！请遵循以下步骤：

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 代码规范
- 使用C++17标准
- 遵循Google C++代码风格
- 添加适当的单元测试
- 更新相关文档

## 📝 版本历史

- **v1.0.0** - 初始版本
  - 基本UDP通信功能
  - 消息协议支持
  - 配置管理系统
  - 日志系统

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙋 常见问题

### Q: 为什么选择UDP而不是TCP？
A: UDP具有较低的延迟和较少的网络开销，适合向Docker容器发送控制命令和状态信息。

### Q: 如何处理UDP数据包丢失？
A: 库内置了重试机制和确认机制。你也可以通过配置调整重试次数和超时时间。

### Q: 支持IPv6吗？
A: 当前版本主要支持IPv4，IPv6支持在后续版本中添加。

### Q: 如何调试网络问题？
A: 启用DEBUG日志级别，查看详细的网络交互日志。

## 📞 技术支持

- 📧 Email: support@your-org.com
- 🐛 Issues: [GitHub Issues](https://github.com/your-org/udp2docker/issues)
- 📖 Wiki: [项目Wiki](https://github.com/your-org/udp2docker/wiki)
- 💬 讨论: [GitHub Discussions](https://github.com/your-org/udp2docker/discussions)

---

**Made with ❤️ by [Your Organization]** 