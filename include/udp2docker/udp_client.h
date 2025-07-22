#pragma once

#include "common.h"
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace udp2docker {

// UDP连接配置结构
struct UdpConfig {
    string_t server_host = DEFAULT_HOST;
    int server_port = DEFAULT_PORT;
    int timeout_ms = DEFAULT_TIMEOUT_MS;
    size_t max_retries = 3;
    bool enable_keep_alive = true;
    int keep_alive_interval_ms = 30000;
};

// 消息回调函数类型
using MessageCallback = std::function<void(const buffer_t&, const string_t& from_host, int from_port)>;
using ErrorCallback = std::function<void(ErrorCode error_code, const string_t& error_message)>;

/**
 * @brief UDP客户端类，负责UDP通信的管理
 * 
 * 该类提供了完整的UDP客户端功能，包括：
 * - 发送和接收UDP数据包
 * - 异步通信支持
 * - 连接管理和错误处理
 * - 心跳保持连接
 */
class UdpClient {
public:
    /**
     * @brief 构造函数
     * @param config UDP连接配置
     */
    explicit UdpClient(const UdpConfig& config = UdpConfig{});
    
    /**
     * @brief 析构函数
     */
    ~UdpClient();
    
    // 禁用拷贝构造和赋值
    UdpClient(const UdpClient&) = delete;
    UdpClient& operator=(const UdpClient&) = delete;
    
    // 支持移动构造和赋值
    UdpClient(UdpClient&& other) noexcept;
    UdpClient& operator=(UdpClient&& other) noexcept;
    
    /**
     * @brief 初始化UDP客户端
     * @return 成功返回SUCCESS，失败返回对应错误码
     */
    ErrorCode initialize();
    
    /**
     * @brief 关闭UDP客户端
     */
    void close();
    
    /**
     * @brief 检查客户端是否已连接
     * @return 连接状态
     */
    bool is_connected() const;
    
    /**
     * @brief 同步发送数据
     * @param data 要发送的数据
     * @param target_host 目标主机（可选，默认使用配置中的主机）
     * @param target_port 目标端口（可选，默认使用配置中的端口）
     * @return 发送结果
     */
    ErrorCode send(const buffer_t& data, 
                   const string_t& target_host = "", 
                   int target_port = 0);
    
    /**
     * @brief 同步发送字符串数据
     * @param message 要发送的字符串
     * @param target_host 目标主机
     * @param target_port 目标端口
     * @return 发送结果
     */
    ErrorCode send_string(const string_t& message,
                         const string_t& target_host = "",
                         int target_port = 0);
    
    /**
     * @brief 异步发送数据
     * @param data 要发送的数据
     * @param callback 发送完成回调
     * @param target_host 目标主机
     * @param target_port 目标端口
     */
    void send_async(const buffer_t& data,
                    std::function<void(ErrorCode)> callback,
                    const string_t& target_host = "",
                    int target_port = 0);
    
    /**
     * @brief 同步接收数据
     * @param buffer 接收数据的缓冲区
     * @param from_host 发送方主机地址（输出参数）
     * @param from_port 发送方端口（输出参数）
     * @return 接收结果，成功返回接收到的字节数
     */
    Result<size_t> receive(buffer_t& buffer, string_t& from_host, int& from_port);
    
    /**
     * @brief 启动异步接收模式
     * @param message_callback 消息接收回调
     * @param error_callback 错误处理回调
     * @return 启动结果
     */
    ErrorCode start_receive_async(MessageCallback message_callback,
                                 ErrorCallback error_callback = nullptr);
    
    /**
     * @brief 停止异步接收
     */
    void stop_receive_async();
    
    /**
     * @brief 设置超时时间
     * @param timeout_ms 超时时间（毫秒）
     */
    void set_timeout(int timeout_ms);
    
    /**
     * @brief 获取当前配置
     * @return UDP配置
     */
    const UdpConfig& get_config() const;
    
    /**
     * @brief 更新配置
     * @param config 新的配置
     * @return 更新结果
     */
    ErrorCode update_config(const UdpConfig& config);
    
    /**
     * @brief 获取统计信息
     */
    struct Statistics {
        size_t packets_sent = 0;
        size_t packets_received = 0;
        size_t bytes_sent = 0;
        size_t bytes_received = 0;
        size_t send_errors = 0;
        size_t receive_errors = 0;
        time_point_t last_activity;
    };
    
    Statistics get_statistics() const;
    
    /**
     * @brief 重置统计信息
     */
    void reset_statistics();

private:
    // 私有成员变量
    UdpConfig config_;
    
#ifdef _WIN32
    SOCKET socket_;
    WSADATA wsa_data_;
#else
    int socket_;
#endif
    
    std::atomic<bool> is_initialized_;
    std::atomic<bool> is_receiving_;
    std::atomic<bool> should_stop_;
    
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    std::thread receive_thread_;
    std::thread keep_alive_thread_;
    
    MessageCallback message_callback_;
    ErrorCallback error_callback_;
    
    // 私有方法
    ErrorCode init_socket();
    void cleanup_socket();
    void receive_loop();
    void keep_alive_loop();
    void update_stats_sent(size_t bytes);
    void update_stats_received(size_t bytes);
    void update_stats_error(bool is_send_error);
    sockaddr_in create_address(const string_t& host, int port);
};

} // namespace udp2docker 