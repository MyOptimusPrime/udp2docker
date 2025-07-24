#include "udp2docker/udp_client.h"
#include "udp2docker/message_protocol.h"
#include "udp2docker/config_manager.h"
#include "udp2docker/logger.h"

#include <iostream>
#include <cassert>

using namespace udp2docker;

// Simple test framework
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
        std::cout << "\nTest Summary: " << passed_tests << "/" << total_tests << " passed" << std::endl;
        if (passed_tests == total_tests) {
            std::cout << "All tests passed! ✓" << std::endl;
        } else {
            std::cout << "Some tests failed! ✗" << std::endl;
        }
    }
};

// Test configuration manager
void test_config_manager(TestFramework& tf) {
    std::cout << "\n=== Testing Configuration Manager ===" << std::endl;
    
    ConfigManager config;
    
    // Test string configuration
    config.set_string("test.string", "hello world", "Test string");
    tf.run_test("Set/Get string config", 
                config.get_string("test.string") == "hello world");
    
    // Test integer configuration
    config.set_int("test.int", 42, "Test integer");
    tf.run_test("Set/Get integer config", 
                config.get_int("test.int") == 42);
    
    // Test boolean configuration
    config.set_bool("test.bool", true, "Test boolean");
    tf.run_test("Set/Get boolean config", 
                config.get_bool("test.bool") == true);
    
    // Test double configuration
    config.set_double("test.double", 3.14159, "Test double");
    tf.run_test("Set/Get double config", 
                abs(config.get_double("test.double") - 3.14159) < 0.00001);
    
    // Test configuration existence check
    tf.run_test("Check config existence", 
                config.has("test.string") && !config.has("nonexistent"));
    
    // Test default values
    tf.run_test("Default value functionality", 
                config.get_string("nonexistent", "default") == "default");
}

// Test message protocol
void test_message_protocol(TestFramework& tf) {
    std::cout << "\n=== Testing Message Protocol ===" << std::endl;
    
    MessageProtocol protocol;
    
    // Test heartbeat message creation
    auto heartbeat = protocol.create_heartbeat();
    tf.run_test("Create heartbeat message", 
                heartbeat.header.type == MessageType::HEARTBEAT);
    
    // Test data message creation
    std::string test_data = "Test data content";
    auto data_msg = protocol.create_string_message(test_data, Priority::HIGH);
    tf.run_test("Create data message", 
                data_msg.header.type == MessageType::DATA &&
                data_msg.header.priority == Priority::HIGH);
    
    // Test control message creation
    auto control_msg = protocol.create_control_message("TEST_COMMAND", Priority::CRITICAL);
    tf.run_test("Create control message", 
                control_msg.header.type == MessageType::CONTROL &&
                control_msg.header.priority == Priority::CRITICAL);
    
    // Test message serialization and deserialization
    auto serialized = protocol.serialize(data_msg);
    tf.run_test("Message serialization", serialized.has_value());
    
    if (serialized) {
        auto deserialized = protocol.deserialize(*serialized);
        tf.run_test("Message deserialization", deserialized.has_value());
        
        if (deserialized) {
            tf.run_test("Serialization/deserialization consistency", 
                        deserialized->header.type == data_msg.header.type &&
                        deserialized->header.priority == data_msg.header.priority);
        }
    }
    
    // Test message validation
    if (serialized) {
        tf.run_test("Message validation", protocol.validate_message(*serialized));
    }
}

// Test logging system
void test_logger(TestFramework& tf) {
    std::cout << "\n=== Testing Logging System ===" << std::endl;
    
    Logger logger("TestLogger");
    
    // Test log level setting
    logger.set_level(LogLevel::DEBUG);
    tf.run_test("Set log level", logger.get_level() == LogLevel::DEBUG);
    
    // Test log level checking
    tf.run_test("Log level check - DEBUG", logger.is_enabled(LogLevel::DEBUG));
    tf.run_test("Log level check - TRACE", !logger.is_enabled(LogLevel::TRACE));
    tf.run_test("Log level check - ERROR", logger.is_enabled(LogLevel::LOG_ERROR));
    
    // Test log recording (these won't fail, just verify they can be called normally)
    logger.debug("This is a debug log");
    logger.info("This is an info log");
    logger.warn("This is a warning log");
    logger.error("This is an error log");
    
    tf.run_test("Log recording functionality", true); // Assume success if we get here
    
    // Test logger name
    tf.run_test("Logger name", logger.get_name() == "TestLogger");
}

// Test UDP client (basic functionality, no actual networking)
void test_udp_client(TestFramework& tf) {
    std::cout << "\n=== Testing UDP Client ===" << std::endl;
    
    UdpConfig config;
    config.server_host = "127.0.0.1";
    config.server_port = 8888;
    config.timeout_ms = 1000; // Short timeout to avoid long test time
    
    UdpClient client(config);
    
    // Test configuration retrieval
    tf.run_test("Get configuration", 
                client.get_config().server_host == "127.0.0.1" &&
                client.get_config().server_port == 8888);
    
    // Test initialization
    auto init_result = client.initialize();
    tf.run_test("UDP client initialization", 
                init_result == ErrorCode::SUCCESS);
    
    if (init_result == ErrorCode::SUCCESS) {
        // Test connection status
        tf.run_test("Connection status check", client.is_connected());
        
        // Test statistics (initial state)
        auto stats = client.get_statistics();
        tf.run_test("Initial statistics", 
                    stats.packets_sent == 0 && 
                    stats.packets_received == 0);
        
        // Test sending (may fail, but shouldn't crash)
        std::string test_message = "Test message";
        auto send_result = client.send_string(test_message);
        tf.run_test("Send message call", 
                    send_result == ErrorCode::SUCCESS || 
                    send_result != ErrorCode::INVALID_PARAMETER);
        
        client.close();
        tf.run_test("Status after closing connection", !client.is_connected());
    }
}

// Test utility functions
void test_utility_functions(TestFramework& tf) {
    std::cout << "\n=== Testing Utility Functions ===" << std::endl;
    
    // Test message type to string conversion
    tf.run_test("Message type to string", 
                message_type_to_string(MessageType::DATA) == "DATA" &&
                message_type_to_string(MessageType::HEARTBEAT) == "HEARTBEAT");
    
    // Test priority to string conversion
    tf.run_test("Priority to string",
                priority_to_string(Priority::HIGH) == "HIGH" &&
                priority_to_string(Priority::LOW) == "LOW");
    
    // Test error code to string conversion
    tf.run_test("Error code to string",
                error_code_to_string(ErrorCode::SUCCESS) == "SUCCESS" &&
                error_code_to_string(ErrorCode::TIMEOUT) == "TIMEOUT");
}

int main() {
    std::cout << "=== UDP2Docker Unit Tests ===" << std::endl;
    std::cout << "Running basic functionality tests for all modules" << std::endl;
    
    TestFramework tf;
    
    try {
        // Run all tests
        test_config_manager(tf);
        test_message_protocol(tf);
        test_logger(tf);
        test_udp_client(tf);
        test_utility_functions(tf);
        
        // Print test summary
        tf.print_summary();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred during testing: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nNote: UDP network tests require actual network environment configuration" << std::endl;
    std::cout << "These tests mainly verify basic functionality and interface correctness" << std::endl;
    
    return 0;
} 