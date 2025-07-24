#pragma once

#include "common.h"
#include <map>
#include <optional>
#include <functional>
#include <mutex>

namespace udp2docker {

// 配置项类型
enum class ConfigType {
    STRING,
    INTEGER,
    BOOLEAN,
    DOUBLE,
    LIST
};

// 配置项结构
struct ConfigItem {
    ConfigType type;
    string_t value;
    string_t description;
    bool required = false;
    
    ConfigItem() = default;
    ConfigItem(ConfigType t, const string_t& v, const string_t& desc = "", bool req = false)
        : type(t), value(v), description(desc), required(req) {}
    
    // 类型转换方法
    string_t as_string() const { return value; }
    int as_int() const;
    bool as_bool() const;
    double as_double() const;
    std::vector<string_t> as_list() const;
};

// 配置变更回调
using ConfigChangeCallback = std::function<void(const string_t& key, const ConfigItem& old_value, const ConfigItem& new_value)>;

/**
 * @brief 配置管理器类，负责应用程序配置的读取、存储和管理
 * 
 * 该类提供了完整的配置管理功能：
 * - 从文件或环境变量加载配置
 * - 配置的持久化存储
 * - 配置验证和类型转换
 * - 配置变更通知
 * - 支持多种配置文件格式（JSON、INI、YAML）
 */
class ConfigManager {
public:
    /**
     * @brief 构造函数
     * @param config_file 配置文件路径（可选）
     */
    explicit ConfigManager(const string_t& config_file = "");
    
    /**
     * @brief 析构函数
     */
    ~ConfigManager();
    
    /**
     * @brief 加载配置文件
     * @param config_file 配置文件路径
     * @return 加载结果
     */
    ErrorCode load_config(const string_t& config_file);
    
    /**
     * @brief 保存配置到文件
     * @param config_file 配置文件路径（可选，默认使用构造时的路径）
     * @return 保存结果
     */
    ErrorCode save_config(const string_t& config_file = "");
    
    /**
     * @brief 从环境变量加载配置
     * @param prefix 环境变量前缀（如"UDP2DOCKER_"）
     */
    void load_from_environment(const string_t& prefix = "UDP2DOCKER_");
    
    /**
     * @brief 设置配置项
     * @param key 配置键
     * @param item 配置项
     */
    void set(const string_t& key, const ConfigItem& item);
    
    /**
     * @brief 设置字符串配置
     * @param key 配置键
     * @param value 配置值
     * @param description 描述
     */
    void set_string(const string_t& key, const string_t& value, const string_t& description = "");
    
    /**
     * @brief 设置整数配置
     * @param key 配置键
     * @param value 配置值
     * @param description 描述
     */
    void set_int(const string_t& key, int value, const string_t& description = "");
    
    /**
     * @brief 设置布尔配置
     * @param key 配置键
     * @param value 配置值
     * @param description 描述
     */
    void set_bool(const string_t& key, bool value, const string_t& description = "");
    
    /**
     * @brief 设置浮点数配置
     * @param key 配置键
     * @param value 配置值
     * @param description 描述
     */
    void set_double(const string_t& key, double value, const string_t& description = "");
    
    /**
     * @brief 获取配置项
     * @param key 配置键
     * @return 配置项，不存在返回空
     */
    std::optional<ConfigItem> get(const string_t& key) const;
    
    /**
     * @brief 获取字符串配置
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    string_t get_string(const string_t& key, const string_t& default_value = "") const;
    
    /**
     * @brief 获取整数配置
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    int get_int(const string_t& key, int default_value = 0) const;
    
    /**
     * @brief 获取布尔配置
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    bool get_bool(const string_t& key, bool default_value = false) const;
    
    /**
     * @brief 获取浮点数配置
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    double get_double(const string_t& key, double default_value = 0.0) const;
    
    /**
     * @brief 检查配置项是否存在
     * @param key 配置键
     * @return 存在返回true
     */
    bool has(const string_t& key) const;
    
    /**
     * @brief 删除配置项
     * @param key 配置键
     * @return 删除结果
     */
    bool remove(const string_t& key);
    
    /**
     * @brief 获取所有配置键
     * @return 配置键列表
     */
    std::vector<string_t> get_all_keys() const;
    
    /**
     * @brief 获取配置数量
     * @return 配置项数量
     */
    size_t size() const;
    
    /**
     * @brief 清空所有配置
     */
    void clear();
    
    /**
     * @brief 验证配置
     * @return 验证结果，成功返回SUCCESS
     */
    ErrorCode validate() const;
    
    /**
     * @brief 注册配置变更回调
     * @param callback 回调函数
     */
    void register_change_callback(ConfigChangeCallback callback);
    
    /**
     * @brief 取消注册配置变更回调
     */
    void unregister_change_callback();
    
    /**
     * @brief 设置默认配置
     */
    void set_defaults();
    
    /**
     * @brief 获取配置文件路径
     * @return 配置文件路径
     */
    const string_t& get_config_file() const;
    
    /**
     * @brief 重新加载配置文件
     * @return 重新加载结果
     */
    ErrorCode reload();
    
    /**
     * @brief 导出配置为字符串
     * @param format 导出格式（json, ini, yaml）
     * @return 配置字符串
     */
    string_t export_config(const string_t& format = "json") const;
    
    /**
     * @brief 从字符串导入配置
     * @param config_str 配置字符串
     * @param format 配置格式
     * @return 导入结果
     */
    ErrorCode import_config(const string_t& config_str, const string_t& format = "json");

private:
    string_t config_file_;
    std::map<string_t, ConfigItem> config_items_;
    mutable std::mutex config_mutex_;
    ConfigChangeCallback change_callback_;
    
    // 私有方法
    ErrorCode load_json_config(const string_t& file_path);
    ErrorCode load_ini_config(const string_t& file_path);
    ErrorCode save_json_config(const string_t& file_path) const;
    ErrorCode save_ini_config(const string_t& file_path) const;
    
    string_t get_file_extension(const string_t& file_path) const;
    void notify_change(const string_t& key, const ConfigItem& old_value, const ConfigItem& new_value);
    
    // JSON解析辅助方法
    std::optional<std::map<string_t, ConfigItem>> parse_json(const string_t& json_str) const;
    string_t config_to_json() const;
    
    // INI解析辅助方法
    std::optional<std::map<string_t, ConfigItem>> parse_ini(const string_t& ini_str) const;
    string_t config_to_ini() const;
};

// 配置管理器单例
class ConfigManagerSingleton {
public:
    static ConfigManager& instance();
    static void initialize(const string_t& config_file = "");
    static void destroy();
    
private:
    static std::unique_ptr<ConfigManager> instance_;
    static std::mutex instance_mutex_;
};

// 便捷宏定义
#define CONFIG() ConfigManagerSingleton::instance()
#define CONFIG_GET_STRING(key, default_val) CONFIG().get_string(key, default_val)
#define CONFIG_GET_INT(key, default_val) CONFIG().get_int(key, default_val)
#define CONFIG_GET_BOOL(key, default_val) CONFIG().get_bool(key, default_val)
#define CONFIG_GET_DOUBLE(key, default_val) CONFIG().get_double(key, default_val)

} // namespace udp2docker 