#include "udp2docker/config_manager.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <mutex>

namespace udp2docker {

// ConfigItem 实现
int ConfigItem::as_int() const {
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

bool ConfigItem::as_bool() const {
    std::string lower_val = value;
    std::transform(lower_val.begin(), lower_val.end(), lower_val.begin(), ::tolower);
    return lower_val == "true" || lower_val == "1" || lower_val == "yes" || lower_val == "on";
}

double ConfigItem::as_double() const {
    try {
        return std::stod(value);
    } catch (...) {
        return 0.0;
    }
}

std::vector<string_t> ConfigItem::as_list() const {
    std::vector<string_t> result;
    std::stringstream ss(value);
    string_t item;
    
    while (std::getline(ss, item, ',')) {
        // 去除前后空格
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

// ConfigManager 实现
ConfigManager::ConfigManager(const string_t& config_file)
    : config_file_(config_file)
{
    set_defaults();
    
    if (!config_file_.empty()) {
        load_config(config_file_);
    }
}

ConfigManager::~ConfigManager() = default;

ErrorCode ConfigManager::load_config(const string_t& config_file) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    config_file_ = config_file;
    
    if (config_file_.empty()) {
        return ErrorCode::INVALID_PARAMETER;
    }
    
    std::ifstream file(config_file_);
    if (!file.is_open()) {
        return ErrorCode::SUCCESS; // 文件不存在不算错误，使用默认配置
    }
    
    string_t extension = get_file_extension(config_file_);
    
    if (extension == "json") {
        return load_json_config(config_file_);
    } else if (extension == "ini") {
        return load_ini_config(config_file_);
    } else {
        // 默认按INI格式处理
        return load_ini_config(config_file_);
    }
}

ErrorCode ConfigManager::save_config(const string_t& config_file) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    string_t file_path = config_file.empty() ? config_file_ : config_file;
    
    if (file_path.empty()) {
        return ErrorCode::INVALID_PARAMETER;
    }
    
    string_t extension = get_file_extension(file_path);
    
    if (extension == "json") {
        return save_json_config(file_path);
    } else {
        return save_ini_config(file_path);
    }
}

void ConfigManager::load_from_environment(const string_t& prefix) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    // 这里简化实现，实际项目中可以遍历环境变量
    const char* env_vars[] = {
        "SERVER_HOST", "SERVER_PORT", "TIMEOUT_MS", 
        "MAX_RETRIES", "ENABLE_KEEP_ALIVE", "LOG_LEVEL"
    };
    
    for (const char* var : env_vars) {
        string_t env_name = prefix + var;
        const char* env_value = std::getenv(env_name.c_str());
        
        if (env_value) {
            string_t key = var;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            std::replace(key.begin(), key.end(), '_', '.');
            
            ConfigItem item(ConfigType::STRING, env_value, "Environment variable");
            config_items_[key] = item;
        }
    }
}

void ConfigManager::set(const string_t& key, const ConfigItem& item) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto old_item = config_items_.find(key);
    ConfigItem old_value;
    
    if (old_item != config_items_.end()) {
        old_value = old_item->second;
    }
    
    config_items_[key] = item;
    
    if (change_callback_) {
        try {
            change_callback_(key, old_value, item);
        } catch (...) {
            // 忽略回调异常
        }
    }
}

void ConfigManager::set_string(const string_t& key, const string_t& value, const string_t& description) {
    set(key, ConfigItem(ConfigType::STRING, value, description));
}

void ConfigManager::set_int(const string_t& key, int value, const string_t& description) {
    set(key, ConfigItem(ConfigType::INTEGER, std::to_string(value), description));
}

void ConfigManager::set_bool(const string_t& key, bool value, const string_t& description) {
    set(key, ConfigItem(ConfigType::BOOLEAN, value ? "true" : "false", description));
}

void ConfigManager::set_double(const string_t& key, double value, const string_t& description) {
    set(key, ConfigItem(ConfigType::DOUBLE, std::to_string(value), description));
}

std::optional<ConfigItem> ConfigManager::get(const string_t& key) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = config_items_.find(key);
    if (it != config_items_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

string_t ConfigManager::get_string(const string_t& key, const string_t& default_value) const {
    auto item = get(key);
    return item ? item->as_string() : default_value;
}

int ConfigManager::get_int(const string_t& key, int default_value) const {
    auto item = get(key);
    return item ? item->as_int() : default_value;
}

bool ConfigManager::get_bool(const string_t& key, bool default_value) const {
    auto item = get(key);
    return item ? item->as_bool() : default_value;
}

double ConfigManager::get_double(const string_t& key, double default_value) const {
    auto item = get(key);
    return item ? item->as_double() : default_value;
}

bool ConfigManager::has(const string_t& key) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_items_.find(key) != config_items_.end();
}

bool ConfigManager::remove(const string_t& key) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_items_.erase(key) > 0;
}

std::vector<string_t> ConfigManager::get_all_keys() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::vector<string_t> keys;
    keys.reserve(config_items_.size());
    
    for (const auto& pair : config_items_) {
        keys.push_back(pair.first);
    }
    
    return keys;
}

size_t ConfigManager::size() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_items_.size();
}

void ConfigManager::clear() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_items_.clear();
}

ErrorCode ConfigManager::validate() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    for (const auto& pair : config_items_) {
        if (pair.second.required && pair.second.value.empty()) {
            return ErrorCode::INVALID_PARAMETER;
        }
    }
    
    return ErrorCode::SUCCESS;
}

void ConfigManager::register_change_callback(ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    change_callback_ = callback;
}

void ConfigManager::unregister_change_callback() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    change_callback_ = nullptr;
}

void ConfigManager::set_defaults() {
    set_string("server.host", DEFAULT_HOST, "UDP server host address");
    set_int("server.port", DEFAULT_PORT, "UDP server port");
    set_int("client.timeout_ms", DEFAULT_TIMEOUT_MS, "Client timeout in milliseconds");
    set_int("client.max_retries", 3, "Maximum number of retries");
    set_bool("client.enable_keep_alive", true, "Enable keep-alive heartbeat");
    set_int("client.keep_alive_interval_ms", 30000, "Keep-alive interval in milliseconds");
    set_string("log.level", "INFO", "Log level (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)");
    set_string("log.file", "udp2docker.log", "Log file path");
    set_bool("log.console", true, "Enable console logging");
}

const string_t& ConfigManager::get_config_file() const {
    return config_file_;
}

ErrorCode ConfigManager::reload() {
    return load_config(config_file_);
}

string_t ConfigManager::export_config(const string_t& format) const {
    if (format == "json") {
        return config_to_json();
    } else {
        return config_to_ini();
    }
}

ErrorCode ConfigManager::import_config(const string_t& config_str, const string_t& format) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    try {
        if (format == "json") {
            auto parsed = parse_json(config_str);
            if (parsed) {
                config_items_ = *parsed;
                return ErrorCode::SUCCESS;
            }
        } else {
            auto parsed = parse_ini(config_str);
            if (parsed) {
                config_items_ = *parsed;
                return ErrorCode::SUCCESS;
            }
        }
        
        return ErrorCode::PROTOCOL_ERROR;
    } catch (...) {
        return ErrorCode::PROTOCOL_ERROR;
    }
}

// 私有方法实现
ErrorCode ConfigManager::load_json_config(const string_t& file_path) {
    // 简化的JSON加载实现
    // 实际项目中应该使用成熟的JSON库如nlohmann/json
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return ErrorCode::SOCKET_RECEIVE_FAILED;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    auto parsed = parse_json(content);
    if (parsed) {
        for (const auto& pair : *parsed) {
            config_items_[pair.first] = pair.second;
        }
        return ErrorCode::SUCCESS;
    }
    
    return ErrorCode::PROTOCOL_ERROR;
}

ErrorCode ConfigManager::load_ini_config(const string_t& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return ErrorCode::SOCKET_RECEIVE_FAILED;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    auto parsed = parse_ini(content);
    if (parsed) {
        for (const auto& pair : *parsed) {
            config_items_[pair.first] = pair.second;
        }
        return ErrorCode::SUCCESS;
    }
    
    return ErrorCode::PROTOCOL_ERROR;
}

ErrorCode ConfigManager::save_json_config(const string_t& file_path) const {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return ErrorCode::SOCKET_SEND_FAILED;
    }
    
    file << config_to_json();
    return ErrorCode::SUCCESS;
}

ErrorCode ConfigManager::save_ini_config(const string_t& file_path) const {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return ErrorCode::SOCKET_SEND_FAILED;
    }
    
    file << config_to_ini();
    return ErrorCode::SUCCESS;
}

string_t ConfigManager::get_file_extension(const string_t& file_path) const {
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != string_t::npos) {
        string_t ext = file_path.substr(dot_pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

void ConfigManager::notify_change(const string_t& key, const ConfigItem& old_value, const ConfigItem& new_value) {
    if (change_callback_) {
        try {
            change_callback_(key, old_value, new_value);
        } catch (...) {
            // 忽略回调异常
        }
    }
}

std::optional<std::map<string_t, ConfigItem>> ConfigManager::parse_json(const string_t& json_str) const {
    // 简化的JSON解析实现
    // 实际项目中应该使用专业的JSON解析库
    
    std::map<string_t, ConfigItem> result;
    
    // 这里只是一个基础实现示例
    std::stringstream ss(json_str);
    string_t line;
    
    while (std::getline(ss, line)) {
        // 去除空格和特殊字符
        line.erase(std::remove_if(line.begin(), line.end(), 
                  [](char c) { return c == '{' || c == '}' || c == '"' || c == ',' || c == ' ' || c == '\t'; }), 
                  line.end());
        
        if (line.empty()) continue;
        
        size_t colon_pos = line.find(':');
        if (colon_pos != string_t::npos) {
            string_t key = line.substr(0, colon_pos);
            string_t value = line.substr(colon_pos + 1);
            
            result[key] = ConfigItem(ConfigType::STRING, value, "From JSON file");
        }
    }
    
    return result;
}

string_t ConfigManager::config_to_json() const {
    std::stringstream ss;
    ss << "{\n";
    
    bool first = true;
    for (const auto& pair : config_items_) {
        if (!first) ss << ",\n";
        ss << "  \"" << pair.first << "\": \"" << pair.second.value << "\"";
        first = false;
    }
    
    ss << "\n}\n";
    return ss.str();
}

std::optional<std::map<string_t, ConfigItem>> ConfigManager::parse_ini(const string_t& ini_str) const {
    std::map<string_t, ConfigItem> result;
    std::stringstream ss(ini_str);
    string_t line;
    string_t current_section;
    
    while (std::getline(ss, line)) {
        // 去除前后空格
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // 跳过空行和注释
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // 处理节（section）
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // 处理键值对
        size_t equal_pos = line.find('=');
        if (equal_pos != string_t::npos) {
            string_t key = line.substr(0, equal_pos);
            string_t value = line.substr(equal_pos + 1);
            
            // 去除前后空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 组合节名和键名
            if (!current_section.empty()) {
                key = current_section + "." + key;
            }
            
            result[key] = ConfigItem(ConfigType::STRING, value, "From INI file");
        }
    }
    
    return result;
}

string_t ConfigManager::config_to_ini() const {
    std::stringstream ss;
    std::map<string_t, std::map<string_t, ConfigItem>> sections;
    
    // 按节分组
    for (const auto& pair : config_items_) {
        size_t dot_pos = pair.first.find('.');
        if (dot_pos != string_t::npos) {
            string_t section = pair.first.substr(0, dot_pos);
            string_t key = pair.first.substr(dot_pos + 1);
            sections[section][key] = pair.second;
        } else {
            sections[""][pair.first] = pair.second;
        }
    }
    
    // 输出各节
    for (const auto& section : sections) {
        if (!section.first.empty()) {
            ss << "[" << section.first << "]\n";
        }
        
        for (const auto& item : section.second) {
            if (!item.second.description.empty()) {
                ss << "# " << item.second.description << "\n";
            }
            ss << item.first << " = " << item.second.value << "\n";
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

// ConfigManagerSingleton 实现
std::unique_ptr<ConfigManager> ConfigManagerSingleton::instance_;
std::mutex ConfigManagerSingleton::instance_mutex_;

ConfigManager& ConfigManagerSingleton::instance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        instance_ = std::make_unique<ConfigManager>();
    }
    return *instance_;
}

void ConfigManagerSingleton::initialize(const string_t& config_file) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_ = std::make_unique<ConfigManager>(config_file);
}

void ConfigManagerSingleton::destroy() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_.reset();
}

} // namespace udp2docker 