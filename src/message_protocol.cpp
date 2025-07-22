#include "udp2docker/message_protocol.h"
#include "udp2docker/logger.h"
#include <cstring>
#include <chrono>
#include <algorithm>

namespace udp2docker {

// MessageHeader 实现
buffer_t MessageHeader::serialize() const {
    buffer_t buffer(header_size());
    size_t offset = 0;
    
    // 魔数
    std::memcpy(buffer.data() + offset, &magic_number, sizeof(magic_number));
    offset += sizeof(magic_number);
    
    // 版本
    std::memcpy(buffer.data() + offset, &version, sizeof(version));
    offset += sizeof(version);
    
    // 消息类型
    uint16_t type_val = static_cast<uint16_t>(type);
    std::memcpy(buffer.data() + offset, &type_val, sizeof(type_val));
    offset += sizeof(type_val);
    
    // 优先级
    uint16_t priority_val = static_cast<uint16_t>(priority);
    std::memcpy(buffer.data() + offset, &priority_val, sizeof(priority_val));
    offset += sizeof(priority_val);
    
    // 序列号
    std::memcpy(buffer.data() + offset, &sequence_id, sizeof(sequence_id));
    offset += sizeof(sequence_id);
    
    // 时间戳
    std::memcpy(buffer.data() + offset, &timestamp, sizeof(timestamp));
    offset += sizeof(timestamp);
    
    // 负载大小
    std::memcpy(buffer.data() + offset, &payload_size, sizeof(payload_size));
    offset += sizeof(payload_size);
    
    // 校验和
    std::memcpy(buffer.data() + offset, &checksum, sizeof(checksum));
    
    return buffer;
}

bool MessageHeader::deserialize(const buffer_t& data) {
    if (data.size() < header_size()) {
        return false;
    }
    
    size_t offset = 0;
    
    // 魔数
    std::memcpy(&magic_number, data.data() + offset, sizeof(magic_number));
    offset += sizeof(magic_number);
    
    if (magic_number != 0x55AA55AA) {
        return false;
    }
    
    // 版本
    std::memcpy(&version, data.data() + offset, sizeof(version));
    offset += sizeof(version);
    
    // 消息类型
    uint16_t type_val;
    std::memcpy(&type_val, data.data() + offset, sizeof(type_val));
    type = static_cast<MessageType>(type_val);
    offset += sizeof(type_val);
    
    // 优先级
    uint16_t priority_val;
    std::memcpy(&priority_val, data.data() + offset, sizeof(priority_val));
    priority = static_cast<Priority>(priority_val);
    offset += sizeof(priority_val);
    
    // 序列号
    std::memcpy(&sequence_id, data.data() + offset, sizeof(sequence_id));
    offset += sizeof(sequence_id);
    
    // 时间戳
    std::memcpy(&timestamp, data.data() + offset, sizeof(timestamp));
    offset += sizeof(timestamp);
    
    // 负载大小
    std::memcpy(&payload_size, data.data() + offset, sizeof(payload_size));
    offset += sizeof(payload_size);
    
    // 校验和
    std::memcpy(&checksum, data.data() + offset, sizeof(checksum));
    
    return true;
}

// Message 实现
Message::Message(MessageType type, const buffer_t& data, Priority priority) {
    header.type = type;
    header.priority = priority;
    header.payload_size = static_cast<uint32_t>(data.size());
    payload = data;
}

Message::Message(MessageType type, const string_t& data, Priority priority) {
    header.type = type;
    header.priority = priority;
    header.payload_size = static_cast<uint32_t>(data.size());
    payload.assign(data.begin(), data.end());
}

size_t Message::total_size() const {
    return MessageHeader::header_size() + payload.size();
}

bool Message::is_valid() const {
    return header.magic_number == 0x55AA55AA && 
           header.payload_size == payload.size();
}

// MessageProtocol 实现
MessageProtocol::MessageProtocol()
    : sequence_counter_(0)
    , protocol_version_(1)
    , compression_enabled_(false)
    , encryption_enabled_(false)
    , max_message_size_(MAX_BUFFER_SIZE)
{
    LOG_DEBUG("MessageProtocol initialized");
}

std::optional<buffer_t> MessageProtocol::serialize(const Message& message) {
    try {
        if (message.payload.size() > max_message_size_) {
            LOG_ERROR("Message payload too large: " + std::to_string(message.payload.size()));
            return std::nullopt;
        }
        
        // 创建消息副本用于处理
        Message msg = message;
        msg.header.version = protocol_version_;
        msg.header.timestamp = get_timestamp();
        
        // 处理负载数据
        buffer_t processed_payload = msg.payload;
        
        if (compression_enabled_) {
            processed_payload = compress_data(processed_payload);
            LOG_DEBUG("Payload compressed: " + std::to_string(msg.payload.size()) + 
                     " -> " + std::to_string(processed_payload.size()));
        }
        
        if (encryption_enabled_) {
            processed_payload = encrypt_data(processed_payload);
            LOG_DEBUG("Payload encrypted");
        }
        
        msg.header.payload_size = static_cast<uint32_t>(processed_payload.size());
        msg.header.checksum = calculate_checksum(processed_payload);
        
        // 序列化头部
        auto header_bytes = msg.header.serialize();
        
        // 组装完整消息
        buffer_t result;
        result.reserve(header_bytes.size() + processed_payload.size());
        result.insert(result.end(), header_bytes.begin(), header_bytes.end());
        result.insert(result.end(), processed_payload.begin(), processed_payload.end());
        
        LOG_DEBUG("Message serialized: " + std::to_string(result.size()) + " bytes");
        return result;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Serialization error: " + std::string(e.what()));
        return std::nullopt;
    }
}

std::optional<Message> MessageProtocol::deserialize(const buffer_t& data) {
    try {
        if (data.size() < MessageHeader::header_size()) {
            LOG_ERROR("Data too small for message header");
            return std::nullopt;
        }
        
        Message message;
        
        // 反序列化头部
        buffer_t header_data(data.begin(), data.begin() + MessageHeader::header_size());
        if (!message.header.deserialize(header_data)) {
            LOG_ERROR("Failed to deserialize message header");
            return std::nullopt;
        }
        
        // 验证版本兼容性
        if (message.header.version > protocol_version_) {
            LOG_WARN("Message version higher than supported: " + 
                    std::to_string(message.header.version));
        }
        
        // 提取负载
        if (data.size() < MessageHeader::header_size() + message.header.payload_size) {
            LOG_ERROR("Data size mismatch with header payload size");
            return std::nullopt;
        }
        
        buffer_t payload_data(data.begin() + MessageHeader::header_size(),
                             data.begin() + MessageHeader::header_size() + message.header.payload_size);
        
        // 验证校验和
        uint32_t calculated_checksum = calculate_checksum(payload_data);
        if (calculated_checksum != message.header.checksum) {
            LOG_ERROR("Checksum mismatch");
            return std::nullopt;
        }
        
        // 处理负载数据
        buffer_t processed_payload = payload_data;
        
        if (encryption_enabled_) {
            processed_payload = decrypt_data(processed_payload);
            LOG_DEBUG("Payload decrypted");
        }
        
        if (compression_enabled_) {
            processed_payload = decompress_data(processed_payload);
            LOG_DEBUG("Payload decompressed");
        }
        
        message.payload = processed_payload;
        
        LOG_DEBUG("Message deserialized: " + std::to_string(data.size()) + " bytes");
        return message;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Deserialization error: " + std::string(e.what()));
        return std::nullopt;
    }
}

Message MessageProtocol::create_heartbeat() {
    Message msg;
    msg.header.type = MessageType::HEARTBEAT;
    msg.header.priority = Priority::LOW;
    msg.header.sequence_id = get_next_sequence_id();
    msg.header.timestamp = get_timestamp();
    msg.payload = {'H', 'B'};
    msg.header.payload_size = static_cast<uint32_t>(msg.payload.size());
    
    LOG_DEBUG("Created heartbeat message");
    return msg;
}

Message MessageProtocol::create_data_message(const buffer_t& payload, Priority priority) {
    Message msg;
    msg.header.type = MessageType::DATA;
    msg.header.priority = priority;
    msg.header.sequence_id = get_next_sequence_id();
    msg.header.timestamp = get_timestamp();
    msg.payload = payload;
    msg.header.payload_size = static_cast<uint32_t>(msg.payload.size());
    
    LOG_DEBUG("Created data message: " + std::to_string(payload.size()) + " bytes");
    return msg;
}

Message MessageProtocol::create_string_message(const string_t& data, Priority priority) {
    buffer_t payload(data.begin(), data.end());
    return create_data_message(payload, priority);
}

Message MessageProtocol::create_control_message(const string_t& command, Priority priority) {
    Message msg;
    msg.header.type = MessageType::CONTROL;
    msg.header.priority = priority;
    msg.header.sequence_id = get_next_sequence_id();
    msg.header.timestamp = get_timestamp();
    msg.payload.assign(command.begin(), command.end());
    msg.header.payload_size = static_cast<uint32_t>(msg.payload.size());
    
    LOG_DEBUG("Created control message: " + command);
    return msg;
}

Message MessageProtocol::create_response_message(uint32_t response_to_seq, const buffer_t& response_data) {
    Message msg;
    msg.header.type = MessageType::RESPONSE;
    msg.header.priority = Priority::HIGH;
    msg.header.sequence_id = get_next_sequence_id();
    msg.header.timestamp = get_timestamp();
    msg.payload = response_data;
    msg.header.payload_size = static_cast<uint32_t>(msg.payload.size());
    
    // 在元数据中记录响应的序列号
    msg.metadata["response_to"] = std::to_string(response_to_seq);
    
    LOG_DEBUG("Created response message for sequence: " + std::to_string(response_to_seq));
    return msg;
}

Message MessageProtocol::create_error_message(ErrorCode error_code, const string_t& error_message) {
    Message msg;
    msg.header.type = MessageType::ERROR;
    msg.header.priority = Priority::CRITICAL;
    msg.header.sequence_id = get_next_sequence_id();
    msg.header.timestamp = get_timestamp();
    
    // 错误消息格式：错误码 + ":" + 错误描述
    string_t error_text = std::to_string(static_cast<int>(error_code)) + ":" + error_message;
    msg.payload.assign(error_text.begin(), error_text.end());
    msg.header.payload_size = static_cast<uint32_t>(msg.payload.size());
    
    LOG_DEBUG("Created error message: " + error_message);
    return msg;
}

bool MessageProtocol::validate_message(const buffer_t& data) {
    auto message = deserialize(data);
    return message.has_value() && message->is_valid();
}

void MessageProtocol::set_compression_enabled(bool enable) {
    compression_enabled_ = enable;
    LOG_INFO("Compression " + std::string(enable ? "enabled" : "disabled"));
}

void MessageProtocol::set_encryption_enabled(bool enable, const string_t& key) {
    encryption_enabled_ = enable;
    if (!key.empty()) {
        encryption_key_ = key;
    }
    LOG_INFO("Encryption " + std::string(enable ? "enabled" : "disabled"));
}

uint32_t MessageProtocol::get_next_sequence_id() {
    return ++sequence_counter_;
}

void MessageProtocol::reset_sequence_id() {
    sequence_counter_ = 0;
    LOG_INFO("Sequence counter reset");
}

void MessageProtocol::set_protocol_version(uint16_t version) {
    protocol_version_ = version;
    LOG_INFO("Protocol version set to: " + std::to_string(version));
}

uint16_t MessageProtocol::get_protocol_version() const {
    return protocol_version_;
}

size_t MessageProtocol::get_max_message_size() const {
    return max_message_size_;
}

// 私有方法实现
uint32_t MessageProtocol::calculate_checksum(const buffer_t& data) {
    // 简单的CRC32校验和算法
    uint32_t checksum = 0xFFFFFFFF;
    
    for (byte b : data) {
        checksum ^= b;
        for (int i = 0; i < 8; ++i) {
            if (checksum & 1) {
                checksum = (checksum >> 1) ^ 0xEDB88320;
            } else {
                checksum >>= 1;
            }
        }
    }
    
    return ~checksum;
}

uint32_t MessageProtocol::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    return static_cast<uint32_t>(timestamp.count());
}

buffer_t MessageProtocol::compress_data(const buffer_t& data) {
    // 简单的压缩实现（这里只是占位符）
    // 实际项目中应该使用真正的压缩算法如zlib
    LOG_WARN("Compression not implemented, returning original data");
    return data;
}

buffer_t MessageProtocol::decompress_data(const buffer_t& data) {
    // 简单的解压缩实现（这里只是占位符）
    LOG_WARN("Decompression not implemented, returning original data");
    return data;
}

buffer_t MessageProtocol::encrypt_data(const buffer_t& data) {
    // 简单的加密实现（这里只是占位符）
    // 实际项目中应该使用真正的加密算法如AES
    LOG_WARN("Encryption not implemented, returning original data");
    return data;
}

buffer_t MessageProtocol::decrypt_data(const buffer_t& data) {
    // 简单的解密实现（这里只是占位符）
    LOG_WARN("Decryption not implemented, returning original data");
    return data;
}

template<typename T>
buffer_t MessageProtocol::to_bytes(T value) {
    buffer_t bytes(sizeof(T));
    std::memcpy(bytes.data(), &value, sizeof(T));
    return bytes;
}

template<typename T>
T MessageProtocol::from_bytes(const buffer_t& data, size_t offset) {
    T value;
    std::memcpy(&value, data.data() + offset, sizeof(T));
    return value;
}

// MessageBuilder 实现
MessageBuilder& MessageBuilder::set_type(MessageType type) {
    message_.header.type = type;
    return *this;
}

MessageBuilder& MessageBuilder::set_priority(Priority priority) {
    message_.header.priority = priority;
    return *this;
}

MessageBuilder& MessageBuilder::set_payload(const buffer_t& payload) {
    message_.payload = payload;
    message_.header.payload_size = static_cast<uint32_t>(payload.size());
    return *this;
}

MessageBuilder& MessageBuilder::set_payload(const string_t& payload) {
    message_.payload.assign(payload.begin(), payload.end());
    message_.header.payload_size = static_cast<uint32_t>(message_.payload.size());
    return *this;
}

MessageBuilder& MessageBuilder::add_metadata(const string_t& key, const string_t& value) {
    message_.metadata[key] = value;
    return *this;
}

MessageBuilder& MessageBuilder::set_sequence_id(uint32_t seq_id) {
    message_.header.sequence_id = seq_id;
    return *this;
}

Message MessageBuilder::build() {
    return std::move(message_);
}

// 辅助函数实现
string_t message_type_to_string(MessageType type) {
    switch (type) {
        case MessageType::HEARTBEAT: return "HEARTBEAT";
        case MessageType::DATA: return "DATA";
        case MessageType::CONTROL: return "CONTROL";
        case MessageType::RESPONSE: return "RESPONSE";
        case MessageType::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

string_t priority_to_string(Priority priority) {
    switch (priority) {
        case Priority::LOW: return "LOW";
        case Priority::NORMAL: return "NORMAL";
        case Priority::HIGH: return "HIGH";
        case Priority::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

string_t error_code_to_string(ErrorCode error) {
    switch (error) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::SOCKET_INIT_FAILED: return "SOCKET_INIT_FAILED";
        case ErrorCode::SOCKET_CREATE_FAILED: return "SOCKET_CREATE_FAILED";
        case ErrorCode::SOCKET_BIND_FAILED: return "SOCKET_BIND_FAILED";
        case ErrorCode::SOCKET_SEND_FAILED: return "SOCKET_SEND_FAILED";
        case ErrorCode::SOCKET_RECEIVE_FAILED: return "SOCKET_RECEIVE_FAILED";
        case ErrorCode::INVALID_ADDRESS: return "INVALID_ADDRESS";
        case ErrorCode::TIMEOUT: return "TIMEOUT";
        case ErrorCode::INVALID_PARAMETER: return "INVALID_PARAMETER";
        case ErrorCode::PROTOCOL_ERROR: return "PROTOCOL_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}

} // namespace udp2docker 