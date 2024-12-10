#pragma once

#include <format>
#include <iostream>
#include <queue>

#include <sw/redis++/redis++.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <protogen/db/db_options.pb.h>

#include <million/imillion.h>
#include <million/proto.h>

#include <db/cache.h>

namespace million {
namespace db {

// 缓存服务
class CacheService : public IService {
public:
    using Base = IService;
    using Base::Base;

    static inline const std::string_view host = "127.0.0.1";
    static inline const std::string_view port = "6379";

    virtual bool OnInit() override {
        try {
            // 创建 Redis 对象并连接到 Redis 服务器
            redis_.emplace(std::format("tcp://{}:{}", host, port));
        }
        catch (const sw::redis::Error& e) {
            logger().Err("Redis error:{}", e.what());
        }

        EnableSeparateWorker();

        return true;
    }

    virtual void OnExit() override {
        redis_ = std::nullopt;
    }

    MILLION_MSG_DISPATCH(CacheService);

    MILLION_MSG_HANDLE(CacheGetMsg, msg) {
        auto& proto_msg = msg->db_row->get();
        const auto& desc = msg->db_row->GetDescriptor();
        const auto& reflection = msg->db_row->GetReflection();
        if (!desc.options().HasExtension(Db::table)) {
            logger().Err("HasExtension Db::table failed.");
            co_return;
        }
        const Db::MessageOptionsTable& options = desc.options().GetExtension(Db::table);
        const auto& table_name = options.name();
        if (table_name.empty()) {
            logger().Err("table_name is empty.");
            co_return;
        }

        // 通过 Redis 哈希表存取 Protobuf 字段
        std::unordered_map<std::string, std::string> redis_hash;
        auto redis_key = std::format("million_db:{}:{}", table_name, msg->primary_key);
        redis_->hgetall(redis_key, std::inserter(redis_hash, redis_hash.end()));
        if (redis_hash.empty()) {
            msg->success = false;
        }
        else {
            // 遍历 Protobuf 字段并设置对应的值
            for (int i = 0; i < desc.field_count(); ++i) {
                const google::protobuf::FieldDescriptor* field = desc.field(i);

                auto iter = redis_hash.find(field->name());
                if (iter == redis_hash.end()) {
                    continue;
                }
                SetField(&proto_msg, *field, iter->second);
            }
            msg->success = true;
        }

        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(CacheSetMsg, msg) {
        auto& proto_msg = msg->db_row->get();
        const auto& desc = msg->db_row->GetDescriptor();
        const auto& reflection = msg->db_row->GetReflection();
        if (!desc.options().HasExtension(Db::table)) {
            logger().Err("HasExtension Db::table failed.");
            co_return;
        }
        const Db::MessageOptionsTable& options = desc.options().GetExtension(Db::table);
        const auto& table_name = options.name();
        if (table_name.empty()) {
            logger().Err("table_name is empty.");
            co_return;
        }

        int32_t ttl = 0;
        if (options.has_cache()) {
            const Db::TableCacheOptions& cache_options = options.cache();
            ttl = cache_options.ttl();
        }
        // options.tick_second();

        std::string_view key;
        // 使用 std::unordered_map 来存储要更新到 Redis 的字段和值
        std::unordered_map<std::string, std::string> redis_hash;

        // 遍历 Protobuf 的所有字段，将字段和值存入 redis_hash 中
        for (int i = 0; i < desc.field_count(); ++i) {
            if (!msg->db_row->IsDirtyFromFIeldIndex(i)) {
                continue;
            }

            const auto* field = desc.field(i);
            if (!field) {
                logger().Err("field({}) is null.", i);
                continue;
            }
            const Db::FieldOptionsColumn& options = field->options().GetExtension(Db::column);

            redis_hash[field->name()] = GetField(proto_msg, *field);

            if (options.has_cache()) {
                const auto& cache_options = options.cache();
                if (cache_options.index()) {
                    if (!key.empty()) {
                        logger().Err("there can only be one index:{}", field->name());
                    }
                    if (field->is_repeated()) {
                        logger().Err("index cannot be an array:{}", field->name());
                    }
                    else {
                        key = redis_hash[field->name()];
                    }
                }
            }
        }

        if (key.empty()) {
            logger().Err("index is empty:", key);
            co_return;
        }
        auto redis_key = std::format("million_db:{}:{}", table_name, key);

        redis_->hmset(redis_key, redis_hash.begin(), redis_hash.end());

        if (ttl > 0) {
            redis_->expire(redis_key.data(), ttl);
        }

        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(CacheGetBytesMsg, msg) {
        auto value =  redis_->get("million_db:" + msg->key_value);
        if (!value) {
            co_return;
        }
        msg->key_value = std::move(*value);
        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(CacheSetBytesMsg, msg) {
        msg->success = redis_->set("million_db:" + msg->key, msg->value);
        Reply(std::move(msg));
        co_return;
    }


private:
    void SetField(google::protobuf::Message* proto_msg, const google::protobuf::FieldDescriptor& field, const std::string& value) {
        const google::protobuf::Descriptor* desc = proto_msg->GetDescriptor();
        const google::protobuf::Reflection* reflection = proto_msg->GetReflection();

        const Db::MessageOptionsTable& options = desc->options().GetExtension(Db::table);
        const auto& table_name = options.name();

        if (field.is_repeated()) {
            logger().Err("db repeated fields are not supported: {}.{}", table_name, field.name());
        }
        else {
            switch (field.type()) {
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                reflection->SetDouble(proto_msg, &field, std::stod(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                reflection->SetFloat(proto_msg, &field, std::stof(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_SINT64:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64: {
                reflection->SetInt64(proto_msg, &field, std::stoll(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
                reflection->SetUInt64(proto_msg, &field, std::stoull(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_SINT32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED32: {
                reflection->SetInt32(proto_msg, &field, std::stoi(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
                reflection->SetUInt32(proto_msg, &field, static_cast<uint32_t>(std::stoul(value)));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                reflection->SetBool(proto_msg, &field, value == "1" || value == "true");
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_STRING:
            case google::protobuf::FieldDescriptor::TYPE_BYTES: {
                reflection->SetString(proto_msg, &field, value);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                const google::protobuf::EnumValueDescriptor* enum_value =
                    field.enum_type()->FindValueByName(value);
                reflection->SetEnum(proto_msg, &field, enum_value);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                google::protobuf::Message* sub_message = reflection->MutableMessage(proto_msg, &field);
                sub_message->ParseFromString(value);
                break;
            }
            default: {
                logger().Err("Unsupported field type: {}", field.name());
            }
            }
        }
    }

    std::string GetField(const google::protobuf::Message& proto_msg, const google::protobuf::FieldDescriptor& field) {
        const google::protobuf::Descriptor* desc = proto_msg.GetDescriptor();
        const google::protobuf::Reflection* reflection = proto_msg.GetReflection();

        const Db::MessageOptionsTable& options = desc->options().GetExtension(Db::table);
        const auto& table_name = options.name();

        std::string value;
        if (field.is_repeated()) {
            logger().Err("db repeated fields are not supported: {}.{}", table_name, field.name());
        }
        else {
            switch (field.type()) {
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                value = std::to_string(reflection->GetDouble(proto_msg, &field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                value = std::to_string(reflection->GetFloat(proto_msg, &field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            case google::protobuf::FieldDescriptor::TYPE_SINT64: {
                value = std::to_string(reflection->GetInt64(proto_msg, &field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
                value = std::to_string(reflection->GetUInt64(proto_msg, &field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SINT32: {
                value = std::to_string(reflection->GetInt32(proto_msg, &field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
                value = std::to_string(reflection->GetUInt32(proto_msg, &field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                value = reflection->GetBool(proto_msg, &field) ? "true" : "false";
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_STRING:
            case google::protobuf::FieldDescriptor::TYPE_BYTES: {
                value = reflection->GetString(proto_msg, &field);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                int enum_value = reflection->GetEnumValue(proto_msg, &field);
                value = std::to_string(enum_value);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                const google::protobuf::Message& sub_message = reflection->GetMessage(proto_msg, &field);
                value = sub_message.SerializeAsString();
                break;
            }
            default: {
                logger().Err("Unsupported field type: {}", field.name());
            }
            }
        }
        return value;
    }

private:
    std::optional<sw::redis::Redis> redis_;
};

} // namespace db
} // namespace million