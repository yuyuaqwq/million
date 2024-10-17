#include <iostream>
#include <queue>
#include <any>

#include "million/imillion.h"

#include <sw/redis++/redis++.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#undef GetMessage

using ProtoMsgUnique = std::unique_ptr<google::protobuf::Message>;

enum CacheMsgType {
    kParseFromCache,
    kSerializeToCache,
};

using CacheMsgBase = million::MsgBaseT<CacheMsgType>;

struct ParseFromCacheMsg : million::MsgT<kParseFromCache> {
    ParseFromCacheMsg(auto&& msg)
        : proto_msg(std::move(msg)) { }

    ProtoMsgUnique proto_msg;
};

struct SerializeToCacheMsg : million::MsgT<kSerializeToCache> {
    SerializeToCacheMsg(auto&& msg)
        : proto_msg(std::move(msg)) { }

    ProtoMsgUnique proto_msg;
};

// 缓存服务
class CacheService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    virtual void OnInit() override {
        try {
            // 创建 Redis 对象并连接到 Redis 服务器
            redis_.emplace("tcp://127.0.0.1:6379");
        }
        catch (const sw::redis::Error& e) {
            std::cerr << "Redis error: " << e.what() << std::endl;
        }

        thread_.emplace([this]() {
            while (true) {
                try {
                    std::unique_lock guard(queue_mutex_);
                    while (queue_.empty()) {
                        queue_cv_.wait(guard);
                    }
                    auto& msg = queue_.front();

                    MILLION_HANDLE_MSG_BEGIN(msg, CacheMsgBase);

                    MILLION_HANDLE_MSG(msg, ParseFromCacheMsg, {
                        ParseFromCache(msg->proto_msg.get());
                    });
                    MILLION_HANDLE_MSG(msg, SerializeToCacheMsg, {
                        SerializeToCache(*msg->proto_msg);
                    });

                    MILLION_HANDLE_MSG_END();

                    queue_.pop();
                }
                catch (const sw::redis::Error& e) {
                    std::cerr << "Redis error: " << e.what() << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
            }
        });
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        Post(std::move(msg));
        co_return;
    }

    virtual void OnExit() override {
        // Close();
    }

    void Post(million::MsgUnique msg) {
        auto guard = std::lock_guard(queue_mutex_);
        queue_.emplace(std::move(msg));
    }

    void ParseFromCache(google::protobuf::Message* msg) {
        const google::protobuf::Descriptor* descriptor = msg->GetDescriptor();
        const google::protobuf::Reflection* reflection = msg->GetReflection();
        auto& table = descriptor->name();

        // 通过 Redis 哈希表存取 Protobuf 字段
        std::unordered_map<std::string, std::string> redis_hash;
        std::vector<std::pair<std::string, std::string>> redis_data;

        // 从 Redis 哈希表中获取数据
        redis_->hgetall(table, std::back_inserter(redis_data));

        // 遍历 Protobuf 字段并设置对应的值
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = descriptor->field(i);
            auto iter = redis_hash.find(field->name());
            if (iter == redis_hash.end()) {
                continue; // 字段在 Redis 中未找到，跳过
            }

            std::string value = iter->second;
            if (field->is_repeated()) {
                google::protobuf::Message* sub_message = reflection->MutableMessage(msg, field);
                sub_message->ParseFromString(value);
            }
            else {
                switch (field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                    reflection->SetDouble(msg, field, std::stod(value));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                    reflection->SetFloat(msg, field, std::stof(value));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_INT64:
                case google::protobuf::FieldDescriptor::TYPE_SINT64:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED64: {
                    reflection->SetInt64(msg, field, std::stoll(value));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_UINT64:
                case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
                    reflection->SetUInt64(msg, field, std::stoull(value));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_INT32:
                case google::protobuf::FieldDescriptor::TYPE_SINT32:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED32: {
                    reflection->SetInt32(msg, field, std::stoi(value));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_UINT32:
                case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
                    reflection->SetUInt32(msg, field, static_cast<uint32_t>(std::stoul(value)));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                    reflection->SetBool(msg, field, value == "1" || value == "true");
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_STRING:
                case google::protobuf::FieldDescriptor::TYPE_BYTES: {
                    reflection->SetString(msg, field, value);
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                    const google::protobuf::EnumValueDescriptor* enum_value =
                        field->enum_type()->FindValueByName(value);
                    reflection->SetEnum(msg, field, enum_value);
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                    google::protobuf::Message* sub_message = reflection->MutableMessage(msg, field);
                    sub_message->ParseFromString(value);
                    break;
                }
                default: {
                    throw std::runtime_error("Unsupported field type.");
                }
                }
            }
        }
    }

    void SerializeToCache(const google::protobuf::Message& msg) {
        const google::protobuf::Descriptor* descriptor = msg.GetDescriptor();
        const google::protobuf::Reflection* reflection = msg.GetReflection();
        auto& table = descriptor->name();

        // 使用 std::unordered_map 来存储要更新到 Redis 的字段和值
        std::unordered_map<std::string, std::string> redis_hash;

        // 遍历 Protobuf 的所有字段，将字段和值存入 redis_hash 中
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = descriptor->field(i);
            
            if (field->is_repeated()) {
                const google::protobuf::Message& sub_message = reflection->GetMessage(msg, field);
                redis_hash[field->name()] = sub_message.SerializeAsString();
            }
            else {
                switch (field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                    redis_hash[field->name()] = std::to_string(reflection->GetDouble(msg, field));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                    redis_hash[field->name()] = std::to_string(reflection->GetFloat(msg, field));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_INT64:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
                case google::protobuf::FieldDescriptor::TYPE_SINT64: {
                    redis_hash[field->name()] = std::to_string(reflection->GetInt64(msg, field));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_UINT64:
                case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
                    redis_hash[field->name()] = std::to_string(reflection->GetUInt64(msg, field));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_INT32:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
                case google::protobuf::FieldDescriptor::TYPE_SINT32: {
                    redis_hash[field->name()] = std::to_string(reflection->GetInt32(msg, field));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_UINT32:
                case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
                    redis_hash[field->name()] = std::to_string(reflection->GetUInt32(msg, field));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                    redis_hash[field->name()] = reflection->GetBool(msg, field) ? "true" : "false";
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_STRING:
                case google::protobuf::FieldDescriptor::TYPE_BYTES: {
                    redis_hash[field->name()] = reflection->GetString(msg, field);
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                    int enum_value = reflection->GetEnumValue(msg, field);
                    redis_hash[field->name()] = std::to_string(enum_value);
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                    const google::protobuf::Message& sub_message = reflection->GetMessage(msg, field);
                    redis_hash[field->name()] = sub_message.SerializeAsString();
                    break;
                }
                default: {
                    throw std::runtime_error("Unsupported field type.");
                }
                }
            }
        }
        redis_->hmset(table.data(), redis_hash.begin(), redis_hash.end());

    }

private:
    std::optional<sw::redis::Redis> redis_;

    std::optional<std::jthread> thread_;
    std::queue<million::MsgUnique> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};
