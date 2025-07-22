#pragma once

#include "common.h"
#include <map>
#include <optional>

namespace udp2docker {

// 消息头结构
struct MessageHeader {
    uint32_t magic_number = 0x55AA55AA;    // 魔数，用于识别协议
    uint16_t version = 1;                  // 协议版本
    MessageType type = MessageType::DATA;   // 消息类型
    Priority priority = Priority::NORMAL;  // 消息优先级
    uint32_t sequence_id = 0;              // 序列号
    uint32_t timestamp = 0;                // 时间戳
    uint32_t payload_size = 0;             // 负载大小
    uint32_t checksum = 0;                 // 校验和
    
    // 序列化和反序列化
    buffer_t serialize() const;
    bool deserialize(const buffer_t& data);
    
    // 计算头部大小
    static constexpr size_t header_size() { return 32; }
};

// 消息结构
struct Message {
    MessageHeader header;
    buffer_t payload;
    std::map<string_t, string_t> metadata;  // 元数据
    
    Message() = default;
    Message(MessageType type, const buffer_t& data, Priority priority = Priority::NORMAL);
    Message(MessageType type, const string_t& data, Priority priority = Priority::NORMAL);
    
    // 获取消息总大小
    size_t total_size() const;
    
    // 验证消息完整性
    bool is_valid() const;
};

/**
 * @brief 消息协议类，负责消息的序列化和反序列化
 * 
 * 该类提供了完整的消息协议处理功能：
 * - 消息序列化和反序列化
 * - 消息完整性校验
 * - 协议版本管理
 * - 压缩和加密支持（可选）
 */
class MessageProtocol {
public:
    /**
     * @brief 构造函数
     */
    MessageProtocol();
    
    /**
     * @brief 析构函数
     */
    ~MessageProtocol() = default;
    
    /**
     * @brief 序列化消息
     * @param message 要序列化的消息
     * @return 序列化后的字节流，失败返回空
     */
    std::optional<buffer_t> serialize(const Message& message);
    
    /**
     * @brief 反序列化消息
     * @param data 字节流数据
     * @return 反序列化后的消息，失败返回空
     */
    std::optional<Message> deserialize(const buffer_t& data);
    
    /**
     * @brief 创建心跳消息
     * @return 心跳消息
     */
    Message create_heartbeat();
    
    /**
     * @brief 创建数据消息
     * @param payload 负载数据
     * @param priority 消息优先级
     * @return 数据消息
     */
    Message create_data_message(const buffer_t& payload, Priority priority = Priority::NORMAL);
    
    /**
     * @brief 创建字符串数据消息
     * @param data 字符串数据
     * @param priority 消息优先级
     * @return 数据消息
     */
    Message create_string_message(const string_t& data, Priority priority = Priority::NORMAL);
    
    /**
     * @brief 创建控制消息
     * @param command 控制命令
     * @param priority 消息优先级
     * @return 控制消息
     */
    Message create_control_message(const string_t& command, Priority priority = Priority::HIGH);
    
    /**
     * @brief 创建响应消息
     * @param response_to_seq 响应的序列号
     * @param response_data 响应数据
     * @return 响应消息
     */
    Message create_response_message(uint32_t response_to_seq, const buffer_t& response_data);
    
    /**
     * @brief 创建错误消息
     * @param error_code 错误代码
     * @param error_message 错误描述
     * @return 错误消息
     */
    Message create_error_message(ErrorCode error_code, const string_t& error_message);
    
    /**
     * @brief 验证消息完整性
     * @param data 消息数据
     * @return 验证结果
     */
    bool validate_message(const buffer_t& data);
    
    /**
     * @brief 设置是否启用压缩
     * @param enable 是否启用
     */
    void set_compression_enabled(bool enable);
    
    /**
     * @brief 设置是否启用加密
     * @param enable 是否启用
     * @param key 加密密钥（可选）
     */
    void set_encryption_enabled(bool enable, const string_t& key = "");
    
    /**
     * @brief 获取下一个序列号
     * @return 序列号
     */
    uint32_t get_next_sequence_id();
    
    /**
     * @brief 重置序列号
     */
    void reset_sequence_id();
    
    /**
     * @brief 设置协议版本
     * @param version 版本号
     */
    void set_protocol_version(uint16_t version);
    
    /**
     * @brief 获取协议版本
     * @return 版本号
     */
    uint16_t get_protocol_version() const;
    
    /**
     * @brief 获取支持的最大消息大小
     * @return 最大消息大小
     */
    size_t get_max_message_size() const;

private:
    uint32_t sequence_counter_;
    uint16_t protocol_version_;
    bool compression_enabled_;
    bool encryption_enabled_;
    string_t encryption_key_;
    size_t max_message_size_;
    
    // 私有方法
    uint32_t calculate_checksum(const buffer_t& data);
    uint32_t get_timestamp();
    buffer_t compress_data(const buffer_t& data);
    buffer_t decompress_data(const buffer_t& data);
    buffer_t encrypt_data(const buffer_t& data);
    buffer_t decrypt_data(const buffer_t& data);
    
    // 字节序转换
    template<typename T>
    buffer_t to_bytes(T value);
    
    template<typename T>
    T from_bytes(const buffer_t& data, size_t offset = 0);
};

// 消息构建器类
class MessageBuilder {
public:
    MessageBuilder() = default;
    
    MessageBuilder& set_type(MessageType type);
    MessageBuilder& set_priority(Priority priority);
    MessageBuilder& set_payload(const buffer_t& payload);
    MessageBuilder& set_payload(const string_t& payload);
    MessageBuilder& add_metadata(const string_t& key, const string_t& value);
    MessageBuilder& set_sequence_id(uint32_t seq_id);
    
    Message build();

private:
    Message message_;
};

// 辅助函数
string_t message_type_to_string(MessageType type);
string_t priority_to_string(Priority priority);
string_t error_code_to_string(ErrorCode error);

} // namespace udp2docker 