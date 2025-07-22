#pragma once

#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace udp2docker {

// 日志级别
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5,
    OFF = 6
};

// 日志输出目标
enum class LogTarget {
    CONSOLE = 1,
    FILE = 2,
    CONSOLE_AND_FILE = 3
};

// 日志记录结构
struct LogRecord {
    LogLevel level;
    string_t message;
    string_t logger_name;
    string_t file_name;
    int line_number;
    string_t function_name;
    time_point_t timestamp;
    std::thread::id thread_id;
    
    LogRecord() = default;
    LogRecord(LogLevel lvl, const string_t& msg, const string_t& logger = "",
              const string_t& file = "", int line = 0, const string_t& func = "");
    
    string_t format(const string_t& pattern = "") const;
};

/**
 * @brief 日志系统类，提供多级别、多目标的日志记录功能
 * 
 * 该类提供了完整的日志记录功能：
 * - 多个日志级别（TRACE到FATAL）
 * - 多种输出目标（控制台、文件）
 * - 异步日志记录（高性能）
 * - 日志格式化和过滤
 * - 日志文件轮转
 * - 线程安全
 */
class Logger {
public:
    /**
     * @brief 构造函数
     * @param name 日志器名称
     */
    explicit Logger(const string_t& name = "udp2docker");
    
    /**
     * @brief 析构函数
     */
    ~Logger();
    
    // 禁用拷贝构造和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    /**
     * @brief 设置日志级别
     * @param level 日志级别
     */
    void set_level(LogLevel level);
    
    /**
     * @brief 获取日志级别
     * @return 当前日志级别
     */
    LogLevel get_level() const;
    
    /**
     * @brief 设置输出目标
     * @param target 输出目标
     */
    void set_target(LogTarget target);
    
    /**
     * @brief 设置日志文件路径
     * @param file_path 日志文件路径
     * @param max_size_mb 最大文件大小（MB）
     * @param max_files 最大文件数量
     */
    void set_file_output(const string_t& file_path, size_t max_size_mb = 100, size_t max_files = 5);
    
    /**
     * @brief 设置日志格式
     * @param pattern 格式模式字符串
     * 支持的占位符：
     * %d - 日期时间
     * %l - 日志级别
     * %n - 日志器名称
     * %m - 消息内容
     * %f - 文件名
     * %L - 行号
     * %F - 函数名
     * %t - 线程ID
     */
    void set_pattern(const string_t& pattern);
    
    /**
     * @brief 启用异步日志
     * @param buffer_size 缓冲区大小
     */
    void enable_async(size_t buffer_size = 1000);
    
    /**
     * @brief 禁用异步日志
     */
    void disable_async();
    
    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param message 消息内容
     * @param file 文件名
     * @param line 行号
     * @param function 函数名
     */
    void log(LogLevel level, const string_t& message,
             const string_t& file = "", int line = 0, const string_t& function = "");
    
    /**
     * @brief TRACE级别日志
     */
    void trace(const string_t& message, const string_t& file = "", int line = 0, const string_t& function = "");
    
    /**
     * @brief DEBUG级别日志
     */
    void debug(const string_t& message, const string_t& file = "", int line = 0, const string_t& function = "");
    
    /**
     * @brief INFO级别日志
     */
    void info(const string_t& message, const string_t& file = "", int line = 0, const string_t& function = "");
    
    /**
     * @brief WARN级别日志
     */
    void warn(const string_t& message, const string_t& file = "", int line = 0, const string_t& function = "");
    
    /**
     * @brief ERROR级别日志
     */
    void error(const string_t& message, const string_t& file = "", int line = 0, const string_t& function = "");
    
    /**
     * @brief FATAL级别日志
     */
    void fatal(const string_t& message, const string_t& file = "", int line = 0, const string_t& function = "");
    
    /**
     * @brief 检查指定级别是否启用
     * @param level 日志级别
     * @return 是否启用
     */
    bool is_enabled(LogLevel level) const;
    
    /**
     * @brief 刷新日志缓冲区
     */
    void flush();
    
    /**
     * @brief 获取日志器名称
     * @return 日志器名称
     */
    const string_t& get_name() const;
    
    /**
     * @brief 轮转日志文件
     */
    void rotate_files();

private:
    string_t name_;
    std::atomic<LogLevel> level_;
    LogTarget target_;
    string_t pattern_;
    string_t file_path_;
    size_t max_file_size_mb_;
    size_t max_files_;
    
    mutable std::mutex log_mutex_;
    std::ofstream file_stream_;
    
    // 异步日志相关
    std::atomic<bool> async_enabled_;
    std::atomic<bool> should_stop_;
    std::queue<LogRecord> log_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::thread async_thread_;
    size_t buffer_size_;
    
    // 私有方法
    void write_log(const LogRecord& record);
    void console_output(const string_t& formatted_message, LogLevel level);
    void file_output(const string_t& formatted_message);
    void async_worker();
    string_t get_current_timestamp() const;
    string_t level_to_string(LogLevel level) const;
    string_t extract_filename(const string_t& path) const;
    bool should_rotate() const;
    void perform_rotation();
    string_t get_rotated_filename(size_t index) const;
};

// 全局日志管理器
class LoggerManager {
public:
    static Logger& get_logger(const string_t& name = "default");
    static void set_global_level(LogLevel level);
    static void set_global_pattern(const string_t& pattern);
    static void shutdown();
    
private:
    static std::map<string_t, std::unique_ptr<Logger>> loggers_;
    static std::mutex manager_mutex_;
    static LogLevel global_level_;
    static string_t global_pattern_;
};

// 便捷宏定义
#define LOG_TRACE(msg) udp2docker::LoggerManager::get_logger().trace(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(msg) udp2docker::LoggerManager::get_logger().debug(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg) udp2docker::LoggerManager::get_logger().info(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(msg) udp2docker::LoggerManager::get_logger().warn(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg) udp2docker::LoggerManager::get_logger().error(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(msg) udp2docker::LoggerManager::get_logger().fatal(msg, __FILE__, __LINE__, __FUNCTION__)

// 带日志器名称的宏
#define LOGGER_TRACE(logger, msg) udp2docker::LoggerManager::get_logger(logger).trace(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOGGER_DEBUG(logger, msg) udp2docker::LoggerManager::get_logger(logger).debug(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOGGER_INFO(logger, msg) udp2docker::LoggerManager::get_logger(logger).info(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOGGER_WARN(logger, msg) udp2docker::LoggerManager::get_logger(logger).warn(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOGGER_ERROR(logger, msg) udp2docker::LoggerManager::get_logger(logger).error(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOGGER_FATAL(logger, msg) udp2docker::LoggerManager::get_logger(logger).fatal(msg, __FILE__, __LINE__, __FUNCTION__)

// 格式化日志宏
#define LOG_INFO_F(format, ...) do { \
    std::stringstream ss; \
    ss << format; \
    LOG_INFO(ss.str()); \
} while(0)

#define LOG_ERROR_F(format, ...) do { \
    std::stringstream ss; \
    ss << format; \
    LOG_ERROR(ss.str()); \
} while(0)

} // namespace udp2docker 