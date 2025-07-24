#include "udp2docker/udp_client.h"
#include "udp2docker/message_protocol.h"
#include "udp2docker/config_manager.h"
#include "udp2docker/logger.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace udp2docker;

// Demonstrate basic UDP sending functionality
void demo_basic_send() {
    std::cout << "\n=== Basic UDP Send Example ===" << std::endl;
    
    // Configure UDP client
    UdpConfig config;
    config.server_host = "127.0.0.1";  // Local Docker environment
    config.server_port = 8888;         // Docker container port
    config.timeout_ms = 5000;
    
    // Create UDP client
    UdpClient client(config);
    
    // Initialize client
    if (client.initialize() != ErrorCode::SUCCESS) {
        LOG_ERROR("Unable to initialize UDP client");
        return;
    }
    
    LOG_INFO("UDP client initialized successfully");
    
    // Send string message
    std::string message = "Hello Docker Container!";
    auto result = client.send_string(message);
    
    if (result == ErrorCode::SUCCESS) {
        LOG_INFO("Message sent successfully: " + message);
    } else {
        LOG_ERROR("Message send failed: " + error_code_to_string(result));
    }
    
    // Display statistics
    auto stats = client.get_statistics();
    std::cout << "Send Statistics:" << std::endl;
    std::cout << "  - Packets sent: " << stats.packets_sent << std::endl;
    std::cout << "  - Bytes sent: " << stats.bytes_sent << std::endl;
    std::cout << "  - Send errors: " << stats.send_errors << std::endl;
    
    client.close();
}

// Demonstrate message protocol functionality
void demo_message_protocol() {
    std::cout << "\n=== Message Protocol Example ===" << std::endl;
    
    MessageProtocol protocol;
    
    // Create different types of messages
    auto data_msg = protocol.create_string_message("This is a data message", Priority::NORMAL);
    auto control_msg = protocol.create_control_message("START_TASK", Priority::HIGH);
    auto heartbeat_msg = protocol.create_heartbeat();
    
    std::cout << "Created messages:" << std::endl;
    std::cout << "  - Data message: " << data_msg.total_size() << " bytes" << std::endl;
    std::cout << "  - Control message: " << control_msg.total_size() << " bytes" << std::endl;
    std::cout << "  - Heartbeat message: " << heartbeat_msg.total_size() << " bytes" << std::endl;
    
    // Serialize message
    auto serialized = protocol.serialize(data_msg);
    if (serialized) {
        std::cout << "Message serialization successful: " << serialized->size() << " bytes" << std::endl;
        
        // Deserialize message
        auto deserialized = protocol.deserialize(*serialized);
        if (deserialized) {
            std::cout << "Message deserialization successful" << std::endl;
            std::cout << "  - Type: " << message_type_to_string(deserialized->header.type) << std::endl;
            std::cout << "  - Priority: " << priority_to_string(deserialized->header.priority) << std::endl;
            std::cout << "  - Sequence ID: " << deserialized->header.sequence_id << std::endl;
        }
    }
}

// Demonstrate async receive functionality
void demo_async_receive() {
    std::cout << "\n=== Async Receive Example ===" << std::endl;
    
    UdpConfig config;
    config.server_host = "0.0.0.0";  // Listen on all interfaces
    config.server_port = 9999;       // Receive port
    
    UdpClient client(config);
    
    if (client.initialize() != ErrorCode::SUCCESS) {
        LOG_ERROR("Unable to initialize UDP client");
        return;
    }
    
    // Set receive callbacks
    MessageCallback msg_callback = [](const buffer_t& data, const string_t& from_host, int from_port) {
        std::string message(data.begin(), data.end());
        LOG_INFO("Received message: " + message + " from " + from_host + ":" + std::to_string(from_port));
    };
    
    ErrorCallback error_callback = [](ErrorCode error, const string_t& message) {
        LOG_WARN("Receive error: " + error_code_to_string(error) + " - " + message);
    };
    
    // Start async receive
    if (client.start_receive_async(msg_callback, error_callback) == ErrorCode::SUCCESS) {
        LOG_INFO("Async receive started, port: " + std::to_string(config.server_port));
        LOG_INFO("Waiting for messages (10 seconds)...");
        
        // Wait for messages
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        client.stop_receive_async();
        LOG_INFO("Stopped async receive");
    }
    
    client.close();
}

// Demonstrate configuration management functionality
void demo_config_management() {
    std::cout << "\n=== Configuration Management Example ===" << std::endl;
    
    // Create configuration manager
    ConfigManager config("udp2docker.ini");
    
    // Set some configuration items
    config.set_string("docker.host", "localhost", "Docker host address");
    config.set_int("docker.port", 2376, "Docker API port");
    config.set_bool("docker.tls", true, "Enable TLS connection");
    config.set_double("connection.timeout", 30.5, "Connection timeout (seconds)");
    
    // Read configuration
    std::cout << "Configuration info:" << std::endl;
    std::cout << "  - Docker host: " << config.get_string("docker.host", "Not set") << std::endl;
    std::cout << "  - Docker port: " << config.get_int("docker.port", 0) << std::endl;
    std::cout << "  - Enable TLS: " << (config.get_bool("docker.tls") ? "Yes" : "No") << std::endl;
    std::cout << "  - Connection timeout: " << config.get_double("connection.timeout", 0.0) << " seconds" << std::endl;
    
    // Save configuration to file
    if (config.save_config() == ErrorCode::SUCCESS) {
        LOG_INFO("Configuration saved to file");
    }
    
    // Export configuration in JSON format
    std::cout << "\nJSON format configuration:" << std::endl;
    std::cout << config.export_config("json") << std::endl;
}

// Demonstrate complete UDP to Docker communication workflow
void demo_complete_workflow() {
    std::cout << "\n=== Complete Workflow Example ===" << std::endl;
    
    // Initialize configuration
    ConfigManagerSingleton::initialize("udp2docker_demo.ini");
    auto& config = ConfigManagerSingleton::instance();
    
    // Initialize logging system
    auto& logger = LoggerManager::get_logger("UDP2Docker");
    logger.set_level(LogLevel::DEBUG);
    logger.set_target(LogTarget::CONSOLE_AND_FILE);
    logger.set_file_output("logs/udp2docker_demo.log");
    
    LOG_INFO("Starting complete workflow demonstration");
    
    // Create message protocol handler
    MessageProtocol protocol;
    
    // Configure UDP client
    UdpConfig udp_config;
    udp_config.server_host = config.get_string("server.host", "127.0.0.1");
    udp_config.server_port = config.get_int("server.port", 8888);
    udp_config.timeout_ms = config.get_int("client.timeout_ms", 5000);
    udp_config.enable_keep_alive = config.get_bool("client.enable_keep_alive", true);
    
    UdpClient client(udp_config);
    
    if (client.initialize() != ErrorCode::SUCCESS) {
        LOG_ERROR("UDP client initialization failed");
        return;
    }
    
    // Send multiple types of messages
    std::vector<std::string> commands = {
        "docker ps",
        "docker images", 
        "docker info",
        "docker version"
    };
    
    for (const auto& cmd : commands) {
        LOG_INFO("Sending command: " + cmd);
        
        // Create control message
        auto message = protocol.create_control_message(cmd, Priority::HIGH);
        
        // Serialize message
        auto serialized = protocol.serialize(message);
        if (serialized) {
            auto result = client.send(*serialized);
            if (result == ErrorCode::SUCCESS) {
                LOG_INFO("Command sent successfully: " + cmd);
            } else {
                LOG_ERROR("Command send failed: " + cmd + " - " + error_code_to_string(result));
            }
        } else {
            LOG_ERROR("Message serialization failed: " + cmd);
        }
        
        // Wait for a while before sending next command
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    // Display final statistics
    auto stats = client.get_statistics();
    LOG_INFO("Final statistics:");
    LOG_INFO("  Packets sent: " + std::to_string(stats.packets_sent));
    LOG_INFO("  Bytes sent: " + std::to_string(stats.bytes_sent));
    LOG_INFO("  Send errors: " + std::to_string(stats.send_errors));
    
    client.close();
    LoggerManager::shutdown();
    ConfigManagerSingleton::destroy();
}

int main() {
    std::cout << "=== UDP2Docker Communication Demo Program ===" << std::endl;
    std::cout << "This program demonstrates UDP messaging functionality from Windows host to Docker environment" << std::endl;
    
    try {
        // Initialize global logging system
        LoggerManager::set_global_level(LogLevel::INFO);
        LoggerManager::set_global_pattern("[%d] [%l] %m");
        
        // Execute demonstrations sequentially
        demo_basic_send();
        demo_message_protocol();
        demo_config_management();
        demo_complete_workflow();
        
        // Note: Async receive demo requires a sender, commented out here
        // demo_async_receive();
        
    } catch (const std::exception& e) {
        std::cerr << "Program exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Demo Program Finished ===" << std::endl;
    std::cout << "Note: In actual use, please ensure Docker container is started and listening on the corresponding port" << std::endl;
    
    return 0;
} 