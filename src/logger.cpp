#include "udp2docker/logger.h"
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <map>
#include <memory>

// C++17 filesystem支持
#if __cplusplus >= 201703L
    #include <filesystem>
    namespace fs = std::filesystem;
#else
         // 如果编译器不支持C++17 filesystem，使用简化版本
     #include <sys/stat.h>
     #ifdef _WIN32
         #include <direct.h>
     #else
         #include <sys/stat.h>
         #include <unistd.h>
     #endif
    namespace fs {
        struct path {
            std::string str;
            path(const std::string& s) : str(s) {}
            path parent_path() const {
                size_t pos = str.find_last_of("/\\");
                return pos != std::string::npos ? path(str.substr(0, pos)) : path("");
            }
            std::string string() const { return str; }
        };
        inline bool exists(const path& p) {
            struct stat st;
            return stat(p.string().c_str(), &st) == 0;
        }
                 inline bool create_directories(const path& p) {
#ifdef _WIN32
             return _mkdir(p.string().c_str()) == 0;
#else
             return mkdir(p.string().c_str(), 0755) == 0;
#endif
         }
        inline std::uintmax_t file_size(const path& p) {
            struct stat st;
            return stat(p.string().c_str(), &st) == 0 ? st.st_size : 0;
        }
        inline bool remove(const path& p) {
            return ::remove(p.string().c_str()) == 0;
        }
        inline bool rename(const path& from, const path& to) {
            return ::rename(from.string().c_str(), to.string().c_str()) == 0;
        }
    }
#endif

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace udp2docker {

// 全局辅助函数
void replace_all(string_t& str, const string_t& from, const string_t& to) {
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != string_t::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

// LogRecord 实现
LogRecord::LogRecord(LogLevel lvl, const string_t& msg, const string_t& logger,
                    const string_t& file, int line, const string_t& func)
    : level(lvl)
    , message(msg)
    , logger_name(logger)
    , file_name(file)
    , line_number(line)
    , function_name(func)
    , timestamp(std::chrono::system_clock::now())
    , thread_id(std::this_thread::get_id())
{
}

string_t LogRecord::format(const string_t& pattern) const {
    if (pattern.empty()) {
        // 默认格式
        std::stringstream ss;
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << " [" << ::udp2docker::level_to_string(level) << "]";
        if (!logger_name.empty()) {
            ss << " [" << logger_name << "]";
        }
        ss << " " << message;
        if (!file_name.empty()) {
            ss << " (" << ::udp2docker::extract_filename(file_name) << ":" << line_number << ")";
        }
        return ss.str();
    }
    
    // 使用指定格式
    string_t result = pattern;
    
    // 替换占位符
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream time_ss;
    time_ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    std::stringstream thread_ss;
    thread_ss << thread_id;
    
    // 执行替换（使用全局函数）
    ::udp2docker::replace_all(result, "%d", time_ss.str());
    ::udp2docker::replace_all(result, "%l", ::udp2docker::level_to_string(level));
    ::udp2docker::replace_all(result, "%n", logger_name);
    ::udp2docker::replace_all(result, "%m", message);
    ::udp2docker::replace_all(result, "%f", ::udp2docker::extract_filename(file_name));
    ::udp2docker::replace_all(result, "%L", std::to_string(line_number));
    ::udp2docker::replace_all(result, "%F", function_name);
    ::udp2docker::replace_all(result, "%t", thread_ss.str());
    
    return result;
}

// 全局辅助函数：将日志级别转换为字符串
string_t level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::LOG_ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        case LogLevel::OFF: return "OFF";
        default: return "UNKNOWN";
    }
}

// 全局辅助函数：从路径中提取文件名
string_t extract_filename(const string_t& path) {
    size_t pos = path.find_last_of("/\\");
    return pos != string_t::npos ? path.substr(pos + 1) : path;
}



// Logger 实现
Logger::Logger(const string_t& name)
    : name_(name)
    , level_(LogLevel::INFO)
    , target_(LogTarget::CONSOLE)
    , pattern_("[%d] [%l] [%n] %m")
    , max_file_size_mb_(100)
    , max_files_(5)
    , async_enabled_(false)
    , should_stop_(false)
    , buffer_size_(1000)
{
}

Logger::~Logger() {
    disable_async();
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::set_level(LogLevel level) {
    level_ = level;
}

LogLevel Logger::get_level() const {
    return level_.load();
}

void Logger::set_target(LogTarget target) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    target_ = target;
}

void Logger::set_file_output(const string_t& file_path, size_t max_size_mb, size_t max_files) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    
    file_path_ = file_path;
    max_file_size_mb_ = max_size_mb;
    max_files_ = max_files;
    
    // 创建目录（如果不存在）
    try {
        fs::path path(file_path_);
        fs::create_directories(path.parent_path());
        
        file_stream_.open(file_path_, std::ios::app);
        if (!file_stream_.is_open()) {
            // 如果无法打开文件，回退到控制台输出
            target_ = LogTarget::CONSOLE;
        }
    } catch (...) {
        target_ = LogTarget::CONSOLE;
    }
}

void Logger::set_pattern(const string_t& pattern) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    pattern_ = pattern;
}

void Logger::enable_async(size_t buffer_size) {
    if (async_enabled_) {
        return;
    }
    
    buffer_size_ = buffer_size;
    async_enabled_ = true;
    should_stop_ = false;
    
    async_thread_ = std::thread([this]() { async_worker(); });
}

void Logger::disable_async() {
    if (!async_enabled_) {
        return;
    }
    
    should_stop_ = true;
    queue_condition_.notify_all();
    
    if (async_thread_.joinable()) {
        async_thread_.join();
    }
    
    async_enabled_ = false;
}

void Logger::log(LogLevel level, const string_t& message,
                const string_t& file, int line, const string_t& function) {
    if (!is_enabled(level)) {
        return;
    }
    
    LogRecord record(level, message, name_, file, line, function);
    
    if (async_enabled_) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (log_queue_.size() < buffer_size_) {
            log_queue_.push(record);
            queue_condition_.notify_one();
        }
        // 如果队列满了，丢弃消息（防止内存泄露）
    } else {
        write_log(record);
    }
}

void Logger::trace(const string_t& message, const string_t& file, int line, const string_t& function) {
    log(LogLevel::TRACE, message, file, line, function);
}

void Logger::debug(const string_t& message, const string_t& file, int line, const string_t& function) {
    log(LogLevel::DEBUG, message, file, line, function);
}

void Logger::info(const string_t& message, const string_t& file, int line, const string_t& function) {
    log(LogLevel::INFO, message, file, line, function);
}

void Logger::warn(const string_t& message, const string_t& file, int line, const string_t& function) {
    log(LogLevel::WARN, message, file, line, function);
}

void Logger::error(const string_t& message, const string_t& file, int line, const string_t& function) {
    log(LogLevel::LOG_ERROR, message, file, line, function);
}

void Logger::fatal(const string_t& message, const string_t& file, int line, const string_t& function) {
    log(LogLevel::FATAL, message, file, line, function);
}

bool Logger::is_enabled(LogLevel level) const {
    return level >= level_.load();
}

void Logger::flush() {
    if (async_enabled_) {
        // 等待异步队列清空
        std::unique_lock<std::mutex> lock(queue_mutex_);
        while (!log_queue_.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (file_stream_.is_open()) {
        file_stream_.flush();
    }
    std::cout.flush();
}

const string_t& Logger::get_name() const {
    return name_;
}

void Logger::rotate_files() {
    if (!should_rotate()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex_);
    perform_rotation();
}

// 私有方法实现
void Logger::write_log(const LogRecord& record) {
    string_t formatted_message = record.format(pattern_);
    
    if (target_ == LogTarget::CONSOLE || target_ == LogTarget::CONSOLE_AND_FILE) {
        console_output(formatted_message, record.level);
    }
    
    if (target_ == LogTarget::FILE || target_ == LogTarget::CONSOLE_AND_FILE) {
        file_output(formatted_message);
    }
}

void Logger::console_output(const string_t& formatted_message, LogLevel level) {
    // 在Windows控制台中使用颜色
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 默认白色
    
    switch (level) {
        case LogLevel::LOG_ERROR:
        case LogLevel::FATAL:
            color = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        case LogLevel::WARN:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case LogLevel::INFO:
            color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case LogLevel::DEBUG:
            color = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        default:
            break;
    }
    
    SetConsoleTextAttribute(hConsole, color);
    std::cout << formatted_message << std::endl;
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    // 在Unix系统中使用ANSI颜色代码
    const char* color_code = "";
    
    switch (level) {
        case LogLevel::LOG_ERROR:
        case LogLevel::FATAL:
            color_code = "\033[31m"; // 红色
            break;
        case LogLevel::WARN:
            color_code = "\033[33m"; // 黄色
            break;
        case LogLevel::INFO:
            color_code = "\033[32m"; // 绿色
            break;
        case LogLevel::DEBUG:
            color_code = "\033[36m"; // 青色
            break;
        default:
            color_code = "\033[0m";  // 默认
            break;
    }
    
    std::cout << color_code << formatted_message << "\033[0m" << std::endl;
#endif
}

void Logger::file_output(const string_t& formatted_message) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (!file_stream_.is_open() && !file_path_.empty()) {
        file_stream_.open(file_path_, std::ios::app);
    }
    
    if (file_stream_.is_open()) {
        file_stream_ << formatted_message << std::endl;
        file_stream_.flush();
        
        // 检查是否需要轮转
        if (should_rotate()) {
            perform_rotation();
        }
    }
}

void Logger::async_worker() {
    while (!should_stop_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        queue_condition_.wait(lock, [this]() {
            return !log_queue_.empty() || should_stop_;
        });
        
        while (!log_queue_.empty()) {
            LogRecord record = log_queue_.front();
            log_queue_.pop();
            lock.unlock();
            
            write_log(record);
            
            lock.lock();
        }
    }
}

string_t Logger::get_current_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}



bool Logger::should_rotate() const {
    if (file_path_.empty() || !file_stream_.is_open()) {
        return false;
    }
    
    try {
        fs::path path(file_path_);
        if (fs::exists(path)) {
            auto file_size = fs::file_size(path);
            return file_size > (max_file_size_mb_ * 1024 * 1024);
        }
    } catch (...) {
        return false;
    }
    
    return false;
}

void Logger::perform_rotation() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    
    try {
        // 删除最旧的日志文件
        string_t oldest_file = get_rotated_filename(max_files_ - 1);
        if (fs::exists(oldest_file)) {
            fs::remove(oldest_file);
        }
        
        // 重命名现有的日志文件
        for (int i = max_files_ - 2; i >= 0; --i) {
            string_t old_name = (i == 0) ? file_path_ : get_rotated_filename(i);
            string_t new_name = get_rotated_filename(i + 1);
            
            if (fs::exists(old_name)) {
                fs::rename(old_name, new_name);
            }
        }
        
        // 重新打开日志文件
        file_stream_.open(file_path_, std::ios::app);
    } catch (...) {
        // 轮转失败，继续使用原文件
        if (!file_stream_.is_open()) {
            file_stream_.open(file_path_, std::ios::app);
        }
    }
}

string_t Logger::get_rotated_filename(size_t index) const {
    fs::path path(file_path_);
    string_t stem = path.string();  // 简化版本中使用完整路径
    string_t extension = "";
    string_t directory = "";
    
    // 手动解析路径
    size_t dot_pos = stem.find_last_of('.');
    size_t slash_pos = stem.find_last_of("/\\");
    
    if (dot_pos != string_t::npos && dot_pos > slash_pos) {
        extension = stem.substr(dot_pos);
        stem = stem.substr(0, dot_pos);
    }
    
    if (slash_pos != string_t::npos) {
        directory = stem.substr(0, slash_pos);
        stem = stem.substr(slash_pos + 1);
    }
    
    if (directory.empty()) {
        return stem + "." + std::to_string(index) + extension;
    } else {
        return directory + "/" + stem + "." + std::to_string(index) + extension;
    }
}

// LoggerManager 实现
std::map<string_t, std::unique_ptr<Logger>> LoggerManager::loggers_;
std::mutex LoggerManager::manager_mutex_;
LogLevel LoggerManager::global_level_ = LogLevel::INFO;
string_t LoggerManager::global_pattern_ = "[%d] [%l] [%n] %m";

Logger& LoggerManager::get_logger(const string_t& name) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = loggers_.find(name);
    if (it != loggers_.end()) {
        return *it->second;
    }
    
    // 创建新的日志器
    auto logger = std::make_unique<Logger>(name);
    logger->set_level(global_level_);
    logger->set_pattern(global_pattern_);
    
    Logger& logger_ref = *logger;
    loggers_[name] = std::move(logger);
    
    return logger_ref;
}

void LoggerManager::set_global_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    global_level_ = level;
    for (auto& pair : loggers_) {
        pair.second->set_level(level);
    }
}

void LoggerManager::set_global_pattern(const string_t& pattern) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    global_pattern_ = pattern;
    for (auto& pair : loggers_) {
        pair.second->set_pattern(pattern);
    }
}

void LoggerManager::shutdown() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    for (auto& pair : loggers_) {
        pair.second->flush();
        pair.second->disable_async();
    }
    
    loggers_.clear();
}

} // namespace udp2docker 