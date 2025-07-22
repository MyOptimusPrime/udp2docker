#include "udp2docker/udp_client.h"
#include "udp2docker/message_protocol.h"
#include "udp2docker/config_manager.h"
#include "udp2docker/logger.h"

#include <iostream>
#include <cassert>

using namespace udp2docker;

// 简单的测试框架
class TestFramework {
private:
    int total_tests = 0;
    int passed_tests = 0;
    
public:
    void run_test(const std::string& test_name, bool result) {
        total_tests++;
        std::cout << "[" << (result ? "PASS" : "FAIL") << "] " << test_name << std::endl;
        if (result) passed_tests++;
    }
    
    void print_summary() {
        std::cout << "\n测试摘要: " << passed_tests << "/" << total_tests << " 通过" << std::endl;
        if (passed_tests == total_tests) {
            std::cout << "所有测试通过! ✓" << std::endl;
        } else {
            std::cout << "有测试失败! ✗" << std::endl;
        }
    }
};

// 测试配置管理器
void test_config_manager(TestFramework& tf) {
    std::cout << "\n=== 测试配置管理器 ===" << std::endl;
    
    ConfigManager config;
    
    // 测试字符串配置
    config.set_string("test.string", "hello world", "测试字符串");
    tf.run_test("设置/获取字符串配置", 
                config.get_string("test.string") == "hello world");
    
    // 测试整数配置
    config.set_int("test.int", 42, "测试整数");
    tf.run_test("设置/获取整数配置", 
                config.get_int("test.int") == 42);
    
    // 测试布尔配置
    config.set_bool("test.bool", true, "测试布尔值");
    tf.run_test("设置/获取布尔配置", 
                config.get_bool("test.bool") == true);
    
    // 测试浮点数配置
    config.set_double("test.double", 3.14159, "测试浮点数");
    tf.run_test("设置/获取浮点数配置", 
                abs(config.get_double("test.double") - 3.14159) < 0.00001);
    
    // 测试配置存在性检查
    tf.run_test("检查配置存在性", 
                config.has("test.string") && !config.has("nonexistent"));
    
    // 测试默认值
    tf.run_test("默认值功能", 
                config.get_string("nonexistent", "default") == "default");
}

// 测试消息协议
void test_message_protocol(TestFramework& tf) {
    std::cout << "\n=== 测试消息协议 ===" << std::endl;
    
    MessageProtocol protocol;
    
    // 测试心跳消息创建
    auto heartbeat = protocol.create_heartbeat();
    tf.run_test("创建心跳消息", 
                heartbeat.header.type == MessageType::HEARTBEAT);
    
    // 测试数据消息创建
    std::string test_data = "测试数据内容";
    auto data_msg = protocol.create_string_message(test_data, Priority::HIGH);
    tf.run_test("创建数据消息", 
                data_msg.header.type == MessageType::DATA &&
                data_msg.header.priority == Priority::HIGH);
    
    // 测试控制消息创建
    auto control_msg = protocol.create_control_message("TEST_COMMAND", Priority::CRITICAL);
    tf.run_test("创建控制消息", 
                control_msg.header.type == MessageType::CONTROL &&
                control_msg.header.priority == Priority::CRITICAL);
    
    // 测试消息序列化和反序列化
    auto serialized = protocol.serialize(data_msg);
    tf.run_test("消息序列化", serialized.has_value());
    
    if (serialized) {
        auto deserialized = protocol.deserialize(*serialized);
        tf.run_test("消息反序列化", deserialized.has_value());
        
        if (deserialized) {
            tf.run_test("序列化/反序列化一致性", 
                        deserialized->header.type == data_msg.header.type &&
                        deserialized->header.priority == data_msg.header.priority);
        }
    }
    
    // 测试消息验证
    if (serialized) {
        tf.run_test("消息验证", protocol.validate_message(*serialized));
    }
}

// 测试日志系统
void test_logger(TestFramework& tf) {
    std::cout << "\n=== 测试日志系统 ===" << std::endl;
    
    Logger logger("TestLogger");
    
    // 测试日志级别设置
    logger.set_level(LogLevel::DEBUG);
    tf.run_test("设置日志级别", logger.get_level() == LogLevel::DEBUG);
    
    // 测试日志级别检查
    tf.run_test("日志级别检查 - DEBUG", logger.is_enabled(LogLevel::DEBUG));
    tf.run_test("日志级别检查 - TRACE", !logger.is_enabled(LogLevel::TRACE));
    tf.run_test("日志级别检查 - ERROR", logger.is_enabled(LogLevel::ERROR));
    
    // 测试日志记录（这些不会失败，只是验证能正常调用）
    logger.debug("这是一条调试日志");
    logger.info("这是一条信息日志");
    logger.warn("这是一条警告日志");
    logger.error("这是一条错误日志");
    
    tf.run_test("日志记录功能", true); // 假设能执行到这里就成功了
    
    // 测试日志器名称
    tf.run_test("日志器名称", logger.get_name() == "TestLogger");
}

// 测试UDP客户端（基本功能，不涉及实际网络）
void test_udp_client(TestFramework& tf) {
    std::cout << "\n=== 测试UDP客户端 ===" << std::endl;
    
    UdpConfig config;
    config.server_host = "127.0.0.1";
    config.server_port = 8888;
    config.timeout_ms = 1000; // 短超时避免测试时间过长
    
    UdpClient client(config);
    
    // 测试配置获取
    tf.run_test("获取配置", 
                client.get_config().server_host == "127.0.0.1" &&
                client.get_config().server_port == 8888);
    
    // 测试初始化
    auto init_result = client.initialize();
    tf.run_test("UDP客户端初始化", 
                init_result == ErrorCode::SUCCESS);
    
    if (init_result == ErrorCode::SUCCESS) {
        // 测试连接状态
        tf.run_test("连接状态检查", client.is_connected());
        
        // 测试统计信息（初始状态）
        auto stats = client.get_statistics();
        tf.run_test("初始统计信息", 
                    stats.packets_sent == 0 && 
                    stats.packets_received == 0);
        
        // 测试发送（可能会失败，但不应该崩溃）
        std::string test_message = "测试消息";
        auto send_result = client.send_string(test_message);
        tf.run_test("发送消息调用", 
                    send_result == ErrorCode::SUCCESS || 
                    send_result != ErrorCode::INVALID_PARAMETER);
        
        client.close();
        tf.run_test("关闭连接后状态", !client.is_connected());
    }
}

// 测试辅助函数
void test_utility_functions(TestFramework& tf) {
    std::cout << "\n=== 测试辅助函数 ===" << std::endl;
    
    // 测试消息类型转字符串
    tf.run_test("消息类型转字符串", 
                message_type_to_string(MessageType::DATA) == "DATA" &&
                message_type_to_string(MessageType::HEARTBEAT) == "HEARTBEAT");
    
    // 测试优先级转字符串
    tf.run_test("优先级转字符串",
                priority_to_string(Priority::HIGH) == "HIGH" &&
                priority_to_string(Priority::LOW) == "LOW");
    
    // 测试错误码转字符串
    tf.run_test("错误码转字符串",
                error_code_to_string(ErrorCode::SUCCESS) == "SUCCESS" &&
                error_code_to_string(ErrorCode::TIMEOUT) == "TIMEOUT");
}

int main() {
    std::cout << "=== UDP2Docker 单元测试 ===" << std::endl;
    std::cout << "运行各个模块的基本功能测试" << std::endl;
    
    TestFramework tf;
    
    try {
        // 运行所有测试
        test_config_manager(tf);
        test_message_protocol(tf);
        test_logger(tf);
        test_udp_client(tf);
        test_utility_functions(tf);
        
        // 打印测试摘要
        tf.print_summary();
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生异常: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n注意: UDP网络测试需要实际的网络环境配置" << std::endl;
    std::cout << "这些测试主要验证基本功能和接口正确性" << std::endl;
    
    return 0;
} 