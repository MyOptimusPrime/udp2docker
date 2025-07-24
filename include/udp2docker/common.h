#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace udp2docker {

// 类型定义
using byte = unsigned char;
using buffer_t = std::vector<byte>;
using string_t = std::string;
using time_point_t = std::chrono::system_clock::time_point;

// 常量定义
constexpr int DEFAULT_PORT = 8888;
constexpr size_t MAX_BUFFER_SIZE = 65536;
constexpr int DEFAULT_TIMEOUT_MS = 5000;
constexpr const char* DEFAULT_HOST = "127.0.0.1";

// 错误码定义
enum class ErrorCode {
    SUCCESS = 0,
    SOCKET_INIT_FAILED,
    SOCKET_CREATE_FAILED,
    SOCKET_BIND_FAILED,
    SOCKET_SEND_FAILED,
    SOCKET_RECEIVE_FAILED,
    INVALID_ADDRESS,
    TIMEOUT,
    INVALID_PARAMETER,
    PROTOCOL_ERROR
};

// 消息类型定义
enum class MessageType {
    HEARTBEAT = 1,
    DATA = 2,
    CONTROL = 3,
    RESPONSE = 4,
    MESSAGE_ERROR = 5
};

// 消息优先级
enum class Priority {
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    CRITICAL = 4
};

// 前置声明
class UdpClient;
class MessageProtocol;
class ConfigManager;
class Logger;

// 结果类模板
template<typename T>
class Result {
public:
    Result(T&& value) : value_(std::move(value)), error_code_(ErrorCode::SUCCESS) {}
    Result(ErrorCode error) : error_code_(error) {}
    
    bool is_success() const { return error_code_ == ErrorCode::SUCCESS; }
    const T& value() const { return value_; }
    ErrorCode error_code() const { return error_code_; }
    
private:
    T value_;
    ErrorCode error_code_;
};

} // namespace udp2docker 