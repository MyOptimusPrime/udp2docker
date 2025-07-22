#include "udp2docker/udp_client.h"
#include "udp2docker/message_protocol.h"
#include "udp2docker/config_manager.h"
#include "udp2docker/logger.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace udp2docker;

// 演示基本的UDP发送功能
void demo_basic_send() {
    std::cout << "\n=== 基本UDP发送示例 ===" << std::endl;
    
    // 配置UDP客户端
    UdpConfig config;
    config.server_host = "127.0.0.1";  // 本地Docker环境
    config.server_port = 8888;         // Docker容器端口
    config.timeout_ms = 5000;
    
    // 创建UDP客户端
    UdpClient client(config);
    
    // 初始化客户端
    if (client.initialize() != ErrorCode::SUCCESS) {
        LOG_ERROR("无法初始化UDP客户端");
        return;
    }
    
    LOG_INFO("UDP客户端初始化成功");
    
    // 发送字符串消息
    std::string message = "Hello Docker Container!";
    auto result = client.send_string(message);
    
    if (result == ErrorCode::SUCCESS) {
        LOG_INFO("消息发送成功: " + message);
    } else {
        LOG_ERROR("消息发送失败: " + error_code_to_string(result));
    }
    
    // 显示统计信息
    auto stats = client.get_statistics();
    std::cout << "发送统计:" << std::endl;
    std::cout << "  - 发送数据包: " << stats.packets_sent << std::endl;
    std::cout << "  - 发送字节数: " << stats.bytes_sent << std::endl;
    std::cout << "  - 发送错误: " << stats.send_errors << std::endl;
    
    client.close();
}

// 演示消息协议功能
void demo_message_protocol() {
    std::cout << "\n=== 消息协议示例 ===" << std::endl;
    
    MessageProtocol protocol;
    
    // 创建不同类型的消息
    auto data_msg = protocol.create_string_message("这是一条数据消息", Priority::NORMAL);
    auto control_msg = protocol.create_control_message("START_TASK", Priority::HIGH);
    auto heartbeat_msg = protocol.create_heartbeat();
    
    std::cout << "创建的消息:" << std::endl;
    std::cout << "  - 数据消息: " << data_msg.total_size() << " bytes" << std::endl;
    std::cout << "  - 控制消息: " << control_msg.total_size() << " bytes" << std::endl;
    std::cout << "  - 心跳消息: " << heartbeat_msg.total_size() << " bytes" << std::endl;
    
    // 序列化消息
    auto serialized = protocol.serialize(data_msg);
    if (serialized) {
        std::cout << "消息序列化成功: " << serialized->size() << " bytes" << std::endl;
        
        // 反序列化消息
        auto deserialized = protocol.deserialize(*serialized);
        if (deserialized) {
            std::cout << "消息反序列化成功" << std::endl;
            std::cout << "  - 类型: " << message_type_to_string(deserialized->header.type) << std::endl;
            std::cout << "  - 优先级: " << priority_to_string(deserialized->header.priority) << std::endl;
            std::cout << "  - 序列号: " << deserialized->header.sequence_id << std::endl;
        }
    }
}

// 演示异步接收功能
void demo_async_receive() {
    std::cout << "\n=== 异步接收示例 ===" << std::endl;
    
    UdpConfig config;
    config.server_host = "0.0.0.0";  // 监听所有接口
    config.server_port = 9999;       // 接收端口
    
    UdpClient client(config);
    
    if (client.initialize() != ErrorCode::SUCCESS) {
        LOG_ERROR("无法初始化UDP客户端");
        return;
    }
    
    // 设置接收回调
    MessageCallback msg_callback = [](const buffer_t& data, const string_t& from_host, int from_port) {
        std::string message(data.begin(), data.end());
        LOG_INFO("收到消息: " + message + " 来自 " + from_host + ":" + std::to_string(from_port));
    };
    
    ErrorCallback error_callback = [](ErrorCode error, const string_t& message) {
        LOG_WARN("接收错误: " + error_code_to_string(error) + " - " + message);
    };
    
    // 启动异步接收
    if (client.start_receive_async(msg_callback, error_callback) == ErrorCode::SUCCESS) {
        LOG_INFO("异步接收已启动，端口: " + std::to_string(config.server_port));
        LOG_INFO("等待接收消息（10秒）...");
        
        // 等待接收消息
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        client.stop_receive_async();
        LOG_INFO("停止异步接收");
    }
    
    client.close();
}

// 演示配置管理功能
void demo_config_management() {
    std::cout << "\n=== 配置管理示例 ===" << std::endl;
    
    // 创建配置管理器
    ConfigManager config("udp2docker.ini");
    
    // 设置一些配置项
    config.set_string("docker.host", "localhost", "Docker主机地址");
    config.set_int("docker.port", 2376, "Docker API端口");
    config.set_bool("docker.tls", true, "启用TLS连接");
    config.set_double("connection.timeout", 30.5, "连接超时时间（秒）");
    
    // 读取配置
    std::cout << "配置信息:" << std::endl;
    std::cout << "  - Docker主机: " << config.get_string("docker.host", "未设置") << std::endl;
    std::cout << "  - Docker端口: " << config.get_int("docker.port", 0) << std::endl;
    std::cout << "  - 启用TLS: " << (config.get_bool("docker.tls") ? "是" : "否") << std::endl;
    std::cout << "  - 连接超时: " << config.get_double("connection.timeout", 0.0) << " 秒" << std::endl;
    
    // 保存配置到文件
    if (config.save_config() == ErrorCode::SUCCESS) {
        LOG_INFO("配置已保存到文件");
    }
    
    // 导出配置为JSON格式
    std::cout << "\nJSON格式配置:" << std::endl;
    std::cout << config.export_config("json") << std::endl;
}

// 演示完整的UDP到Docker通信流程
void demo_complete_workflow() {
    std::cout << "\n=== 完整工作流程示例 ===" << std::endl;
    
    // 初始化配置
    ConfigManagerSingleton::initialize("udp2docker_demo.ini");
    auto& config = ConfigManagerSingleton::instance();
    
    // 初始化日志系统
    auto& logger = LoggerManager::get_logger("UDP2Docker");
    logger.set_level(LogLevel::DEBUG);
    logger.set_target(LogTarget::CONSOLE_AND_FILE);
    logger.set_file_output("logs/udp2docker_demo.log");
    
    LOG_INFO("开始完整工作流程演示");
    
    // 创建消息协议处理器
    MessageProtocol protocol;
    
    // 配置UDP客户端
    UdpConfig udp_config;
    udp_config.server_host = config.get_string("server.host", "127.0.0.1");
    udp_config.server_port = config.get_int("server.port", 8888);
    udp_config.timeout_ms = config.get_int("client.timeout_ms", 5000);
    udp_config.enable_keep_alive = config.get_bool("client.enable_keep_alive", true);
    
    UdpClient client(udp_config);
    
    if (client.initialize() != ErrorCode::SUCCESS) {
        LOG_ERROR("UDP客户端初始化失败");
        return;
    }
    
    // 发送多种类型的消息
    std::vector<std::string> commands = {
        "docker ps",
        "docker images", 
        "docker info",
        "docker version"
    };
    
    for (const auto& cmd : commands) {
        LOG_INFO("发送命令: " + cmd);
        
        // 创建控制消息
        auto message = protocol.create_control_message(cmd, Priority::HIGH);
        
        // 序列化消息
        auto serialized = protocol.serialize(message);
        if (serialized) {
            auto result = client.send(*serialized);
            if (result == ErrorCode::SUCCESS) {
                LOG_INFO("命令发送成功: " + cmd);
            } else {
                LOG_ERROR("命令发送失败: " + cmd + " - " + error_code_to_string(result));
            }
        } else {
            LOG_ERROR("消息序列化失败: " + cmd);
        }
        
        // 等待一段时间再发送下一个命令
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    // 显示最终统计信息
    auto stats = client.get_statistics();
    LOG_INFO("最终统计信息:");
    LOG_INFO("  发送数据包: " + std::to_string(stats.packets_sent));
    LOG_INFO("  发送字节数: " + std::to_string(stats.bytes_sent));
    LOG_INFO("  发送错误: " + std::to_string(stats.send_errors));
    
    client.close();
    LoggerManager::shutdown();
    ConfigManagerSingleton::destroy();
}

int main() {
    std::cout << "=== UDP2Docker 通信演示程序 ===" << std::endl;
    std::cout << "这个程序演示了从Windows宿主机向Docker环境发送UDP消息的功能" << std::endl;
    
    try {
        // 初始化全局日志系统
        LoggerManager::set_global_level(LogLevel::INFO);
        LoggerManager::set_global_pattern("[%d] [%l] %m");
        
        // 依次执行各个演示
        demo_basic_send();
        demo_message_protocol();
        demo_config_management();
        demo_complete_workflow();
        
        // 注意：异步接收演示需要有发送方，这里注释掉
        // demo_async_receive();
        
    } catch (const std::exception& e) {
        std::cerr << "程序异常: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== 演示程序结束 ===" << std::endl;
    std::cout << "提示: 在实际使用中，请确保Docker容器已启动并监听对应端口" << std::endl;
    
    return 0;
} 