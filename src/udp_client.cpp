#include "udp2docker/udp_client.h"
#include "udp2docker/logger.h"
#include <sstream>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif

namespace udp2docker {

UdpClient::UdpClient(const UdpConfig& config)
    : config_(config)
#ifdef _WIN32
    , socket_(INVALID_SOCKET)
#else
    , socket_(-1)
#endif
    , is_initialized_(false)
    , is_receiving_(false)
    , should_stop_(false)
{
    LOG_DEBUG("UdpClient created with server: " + config_.server_host + ":" + std::to_string(config_.server_port));
    stats_.last_activity = std::chrono::system_clock::now();
}

UdpClient::~UdpClient() {
    close();
    LOG_DEBUG("UdpClient destroyed");
}

UdpClient::UdpClient(UdpClient&& other) noexcept
    : config_(std::move(other.config_))
    , socket_(other.socket_)
    , is_initialized_(other.is_initialized_.load())
    , is_receiving_(other.is_receiving_.load())
    , should_stop_(other.should_stop_.load())
    , stats_(std::move(other.stats_))
    , receive_thread_(std::move(other.receive_thread_))
    , keep_alive_thread_(std::move(other.keep_alive_thread_))
    , message_callback_(std::move(other.message_callback_))
    , error_callback_(std::move(other.error_callback_))
{
#ifdef _WIN32
    other.socket_ = INVALID_SOCKET;
#else
    other.socket_ = -1;
#endif
    other.is_initialized_ = false;
    other.is_receiving_ = false;
}

UdpClient& UdpClient::operator=(UdpClient&& other) noexcept {
    if (this != &other) {
        close();
        
        config_ = std::move(other.config_);
        socket_ = other.socket_;
        is_initialized_ = other.is_initialized_.load();
        is_receiving_ = other.is_receiving_.load();
        should_stop_ = other.should_stop_.load();
        stats_ = std::move(other.stats_);
        receive_thread_ = std::move(other.receive_thread_);
        keep_alive_thread_ = std::move(other.keep_alive_thread_);
        message_callback_ = std::move(other.message_callback_);
        error_callback_ = std::move(other.error_callback_);
        
#ifdef _WIN32
        other.socket_ = INVALID_SOCKET;
#else
        other.socket_ = -1;
#endif
        other.is_initialized_ = false;
        other.is_receiving_ = false;
    }
    return *this;
}

ErrorCode UdpClient::initialize() {
    if (is_initialized_) {
        LOG_WARN("UdpClient already initialized");
        return ErrorCode::SUCCESS;
    }
    
    LOG_INFO("Initializing UdpClient...");
    
    auto result = init_socket();
    if (result == ErrorCode::SUCCESS) {
        is_initialized_ = true;
        LOG_INFO("UdpClient initialized successfully");
    } else {
        LOG_ERROR("Failed to initialize UdpClient: " + std::to_string(static_cast<int>(result)));
    }
    
    return result;
}

void UdpClient::close() {
    if (!is_initialized_) {
        return;
    }
    
    LOG_INFO("Closing UdpClient...");
    
    should_stop_ = true;
    stop_receive_async();
    
    // 等待线程结束
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    
    if (keep_alive_thread_.joinable()) {
        keep_alive_thread_.join();
    }
    
    cleanup_socket();
    is_initialized_ = false;
    
    LOG_INFO("UdpClient closed");
}

bool UdpClient::is_connected() const {
    return is_initialized_;
}

ErrorCode UdpClient::send(const buffer_t& data, const string_t& target_host, int target_port) {
    if (!is_initialized_) {
        LOG_ERROR("UdpClient not initialized");
        return ErrorCode::SOCKET_INIT_FAILED;
    }
    
    if (data.empty()) {
        LOG_ERROR("Cannot send empty data");
        return ErrorCode::INVALID_PARAMETER;
    }
    
    string_t host = target_host.empty() ? config_.server_host : target_host;
    int port = target_port == 0 ? config_.server_port : target_port;
    
    LOG_DEBUG("Sending " + std::to_string(data.size()) + " bytes to " + host + ":" + std::to_string(port));
    
    auto addr = create_address(host, port);
    
    int result = sendto(socket_, reinterpret_cast<const char*>(data.data()), 
                       static_cast<int>(data.size()), 0,
                       reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    
    if (result == SOCKET_ERROR || result < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        LOG_ERROR("Send failed with error: " + std::to_string(error));
#else
        LOG_ERROR("Send failed with error: " + std::string(strerror(errno)));
#endif
        update_stats_error(true);
        return ErrorCode::SOCKET_SEND_FAILED;
    }
    
    update_stats_sent(data.size());
    LOG_DEBUG("Successfully sent " + std::to_string(result) + " bytes");
    
    return ErrorCode::SUCCESS;
}

ErrorCode UdpClient::send_string(const string_t& message, const string_t& target_host, int target_port) {
    buffer_t data(message.begin(), message.end());
    return send(data, target_host, target_port);
}

void UdpClient::send_async(const buffer_t& data, std::function<void(ErrorCode)> callback,
                          const string_t& target_host, int target_port) {
    std::thread([this, data, callback, target_host, target_port]() {
        auto result = send(data, target_host, target_port);
        if (callback) {
            callback(result);
        }
    }).detach();
}

Result<size_t> UdpClient::receive(buffer_t& buffer, string_t& from_host, int& from_port) {
    if (!is_initialized_) {
        LOG_ERROR("UdpClient not initialized");
        return Result<size_t>(ErrorCode::SOCKET_INIT_FAILED);
    }
    
    sockaddr_in from_addr{};
    socklen_t addr_len = sizeof(from_addr);
    
    buffer.resize(MAX_BUFFER_SIZE);
    
    int result = recvfrom(socket_, reinterpret_cast<char*>(buffer.data()), 
                         static_cast<int>(buffer.size()), 0,
                         reinterpret_cast<sockaddr*>(&from_addr), &addr_len);
    
    if (result == SOCKET_ERROR || result < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            return Result<size_t>(ErrorCode::TIMEOUT);
        }
        LOG_ERROR("Receive failed with error: " + std::to_string(error));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return Result<size_t>(ErrorCode::TIMEOUT);
        }
        LOG_ERROR("Receive failed with error: " + std::string(strerror(errno)));
#endif
        update_stats_error(false);
        return Result<size_t>(ErrorCode::SOCKET_RECEIVE_FAILED);
    }
    
    buffer.resize(result);
    from_host = inet_ntoa(from_addr.sin_addr);
    from_port = ntohs(from_addr.sin_port);
    
    update_stats_received(result);
    LOG_DEBUG("Received " + std::to_string(result) + " bytes from " + from_host + ":" + std::to_string(from_port));
    
    return Result<size_t>(static_cast<size_t>(result));
}

ErrorCode UdpClient::start_receive_async(MessageCallback message_callback, ErrorCallback error_callback) {
    if (!is_initialized_) {
        LOG_ERROR("UdpClient not initialized");
        return ErrorCode::SOCKET_INIT_FAILED;
    }
    
    if (is_receiving_) {
        LOG_WARN("Already receiving asynchronously");
        return ErrorCode::SUCCESS;
    }
    
    message_callback_ = message_callback;
    error_callback_ = error_callback;
    is_receiving_ = true;
    should_stop_ = false;
    
    receive_thread_ = std::thread([this]() { receive_loop(); });
    
    if (config_.enable_keep_alive) {
        keep_alive_thread_ = std::thread([this]() { keep_alive_loop(); });
    }
    
    LOG_INFO("Started asynchronous receiving");
    return ErrorCode::SUCCESS;
}

void UdpClient::stop_receive_async() {
    if (!is_receiving_) {
        return;
    }
    
    LOG_INFO("Stopping asynchronous receiving");
    should_stop_ = true;
    is_receiving_ = false;
    
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    
    if (keep_alive_thread_.joinable()) {
        keep_alive_thread_.join();
    }
    
    LOG_INFO("Stopped asynchronous receiving");
}

void UdpClient::set_timeout(int timeout_ms) {
    config_.timeout_ms = timeout_ms;
    
    if (is_initialized_) {
#ifdef _WIN32
        DWORD timeout = timeout_ms;
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    }
    
    LOG_DEBUG("Set timeout to " + std::to_string(timeout_ms) + " ms");
}

const UdpConfig& UdpClient::get_config() const {
    return config_;
}

ErrorCode UdpClient::update_config(const UdpConfig& config) {
    config_ = config;
    
    if (is_initialized_) {
        set_timeout(config_.timeout_ms);
    }
    
    LOG_INFO("Configuration updated");
    return ErrorCode::SUCCESS;
}

UdpClient::Statistics UdpClient::get_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void UdpClient::reset_statistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = Statistics{};
    stats_.last_activity = std::chrono::system_clock::now();
    LOG_INFO("Statistics reset");
}

// 私有方法实现
ErrorCode UdpClient::init_socket() {
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0) {
        LOG_ERROR("WSAStartup failed");
        return ErrorCode::SOCKET_INIT_FAILED;
    }
    
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) {
        LOG_ERROR("Socket creation failed with error: " + std::to_string(WSAGetLastError()));
        WSACleanup();
        return ErrorCode::SOCKET_CREATE_FAILED;
    }
#else
    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0) {
        LOG_ERROR("Socket creation failed: " + std::string(strerror(errno)));
        return ErrorCode::SOCKET_CREATE_FAILED;
    }
#endif
    
    set_timeout(config_.timeout_ms);
    
    LOG_DEBUG("Socket created successfully");
    return ErrorCode::SUCCESS;
}

void UdpClient::cleanup_socket() {
    if (socket_ != 
#ifdef _WIN32
        INVALID_SOCKET
#else
        -1
#endif
    ) {
#ifdef _WIN32
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        WSACleanup();
#else
        ::close(socket_);
        socket_ = -1;
#endif
        LOG_DEBUG("Socket cleaned up");
    }
}

void UdpClient::receive_loop() {
    LOG_INFO("Receive loop started");
    
    while (!should_stop_) {
        buffer_t buffer;
        string_t from_host;
        int from_port;
        
        auto result = receive(buffer, from_host, from_port);
        
        if (result.is_success()) {
            if (message_callback_) {
                try {
                    message_callback_(buffer, from_host, from_port);
                } catch (const std::exception& e) {
                    LOG_ERROR("Message callback exception: " + std::string(e.what()));
                }
            }
        } else if (result.error_code() != ErrorCode::TIMEOUT) {
            if (error_callback_) {
                try {
                    error_callback_(result.error_code(), "Receive error");
                } catch (const std::exception& e) {
                    LOG_ERROR("Error callback exception: " + std::string(e.what()));
                }
            }
        }
        
        // 短暂休眠以避免过度占用CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LOG_INFO("Receive loop stopped");
}

void UdpClient::keep_alive_loop() {
    LOG_INFO("Keep-alive loop started");
    
    while (!should_stop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.keep_alive_interval_ms));
        
        if (should_stop_) break;
        
        // 发送心跳包
        buffer_t heartbeat{'H', 'B'};
        auto result = send(heartbeat);
        
        if (result != ErrorCode::SUCCESS) {
            LOG_WARN("Keep-alive heartbeat send failed");
            if (error_callback_) {
                try {
                    error_callback_(result, "Keep-alive failed");
                } catch (const std::exception& e) {
                    LOG_ERROR("Error callback exception: " + std::string(e.what()));
                }
            }
        } else {
            LOG_DEBUG("Keep-alive heartbeat sent");
        }
    }
    
    LOG_INFO("Keep-alive loop stopped");
}

void UdpClient::update_stats_sent(size_t bytes) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.packets_sent++;
    stats_.bytes_sent += bytes;
    stats_.last_activity = std::chrono::system_clock::now();
}

void UdpClient::update_stats_received(size_t bytes) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.packets_received++;
    stats_.bytes_received += bytes;
    stats_.last_activity = std::chrono::system_clock::now();
}

void UdpClient::update_stats_error(bool is_send_error) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (is_send_error) {
        stats_.send_errors++;
    } else {
        stats_.receive_errors++;
    }
}

sockaddr_in UdpClient::create_address(const string_t& host, int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
#ifdef _WIN32
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        // 可能是域名，需要解析
        LOG_WARN("Host resolution not implemented for: " + host);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
#else
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        LOG_WARN("Invalid address: " + host);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    }
#endif
    
    return addr;
}

} // namespace udp2docker 