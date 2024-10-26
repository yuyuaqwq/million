#pragma once

#include <format>
#include <iostream>
#include <queue>

#include <sw/redis++/redis++.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <protogen/db/db_options.pb.h>

#include <million/imillion.h>
#include <million/proto_msg.h>

#include <db/cache_msg.h>

namespace million {
namespace db {

// �������
class CacheService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    static inline const std::string_view host = "127.0.0.1";
    static inline const std::string_view port = "6379";

    virtual void OnInit() override {
        try {
            // ���� Redis �������ӵ� Redis ������
            redis_.emplace(std::format("tcp://{}:{}", host, port));
        }
        catch (const sw::redis::Error& e) {
            std::cerr << "Redis error: " << e.what() << std::endl;
        }

        thread_.emplace([this]() {
            while (run_) {
                try {
                    std::unique_lock guard(queue_mutex_);
                    while (run_ && queue_.empty()) {
                        queue_cv_.wait(guard);
                    }
                    if (!run_) return;

                    auto msg = std::move(queue_.front());
                    queue_.pop();
                    auto task = MsgDispatch(std::move(msg));
                    task.rethrow_if_exception();
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
        // ����redis�߳�ȥ��д����������work�߳�
        auto guard = std::lock_guard(queue_mutex_);
        queue_.emplace(std::move(msg));
        co_return;
    }

    virtual void OnExit() override {
        queue_cv_.notify_one();
        run_ = false;
        thread_->join();
        redis_ = std::nullopt;
    }

    MILLION_MSG_DISPATCH(CacheService, CacheMsgBase);

    MILLION_MSG_HANDLE(ParseFromCacheMsg, msg) {
        auto proto_msg = msg->proto_msg.get();
        const google::protobuf::Descriptor* descriptor = proto_msg->GetDescriptor();
        const google::protobuf::Reflection* reflection = proto_msg->GetReflection();
        auto& table = descriptor->name();

        // ͨ�� Redis ��ϣ����ȡ Protobuf �ֶ�
        std::unordered_map<std::string, std::string> redis_hash;
        redis_->hgetall(table, std::inserter(redis_hash, redis_hash.end()));
        if (redis_hash.empty()) {
            msg->success = false;
        }
        else {
            // ���� Protobuf �ֶβ����ö�Ӧ��ֵ
            for (int i = 0; i < descriptor->field_count(); ++i) {
                const google::protobuf::FieldDescriptor* field = descriptor->field(i);
                auto iter = redis_hash.find(field->name());
                if (iter == redis_hash.end()) {
                    continue; // �ֶ��� Redis ��δ�ҵ�������
                }
                SetField(proto_msg, field, reflection, iter->second);
            }
            msg->success = true;
        }

        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(SerializeToCacheMsg, msg) {
        auto& proto_msg = *msg->proto_msg;
        const google::protobuf::Descriptor* descriptor = proto_msg.GetDescriptor();
        const google::protobuf::Reflection* reflection = proto_msg.GetReflection();

        const Db::MessageOptionsTable& options = descriptor->options().GetExtension(Db::table);
        int32_t ttl = 0;
        if (options.has_cache()) {
            const Db::TableCacheOptions& cache_options = options.cache();
            ttl = cache_options.ttl();
        }
        options.tick_second();

        std::string_view key;
        // ʹ�� std::unordered_map ���洢Ҫ���µ� Redis ���ֶκ�ֵ
        std::unordered_map<std::string, std::string> redis_hash;

        // ���� Protobuf �������ֶΣ����ֶκ�ֵ���� redis_hash ��
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = descriptor->field(i);
            const Db::FieldOptionsColumn& options = field->options().GetExtension(Db::column);

            redis_hash[field->name()] = GetField(proto_msg, field, reflection);

            if (options.has_cache()) {
                if (options.cache().index()) {
                    if (!key.empty()) {
                        std::cerr << "there can only be one index:" << field->name() << std::endl;
                    }
                    if (field->is_repeated()) {
                        std::cerr << "index cannot be an array:" << field->name() << std::endl;
                    }
                    else {
                        key = redis_hash[field->name()];
                    }
                }
            }
        }

        if (options.name().empty() || key.empty()) {
            std::cerr << "name is empty or index is empty:" << options.name() << "," << key << std::endl;
            co_return;
        }
        auto table = std::format("db:{}:{}", options.name(), key);

        redis_->hmset(table.data(), redis_hash.begin(), redis_hash.end());

        if (ttl > 0) {
            redis_->expire(table.data(), ttl);
        }

        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(CacheGetMsg, msg) {
        auto value =  redis_->get("cache:" + msg->key_value);
        if (!value) {
            co_return;
        }
        msg->key_value = std::move(*value);
        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(CacheSetMsg, msg) {
        msg->success = redis_->set("cache:" + msg->key, msg->value);
        Reply(std::move(msg));
        co_return;
    }

    void SetField(google::protobuf::Message* proto_msg, const google::protobuf::FieldDescriptor* field, const google::protobuf::Reflection* reflection, const std::string& value) {
        if (field->is_repeated()) {
            google::protobuf::Message* sub_message = reflection->MutableMessage(proto_msg, field);
            sub_message->ParseFromString(value);
        }
        else {
            switch (field->type()) {
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                reflection->SetDouble(proto_msg, field, std::stod(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                reflection->SetFloat(proto_msg, field, std::stof(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_SINT64:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64: {
                reflection->SetInt64(proto_msg, field, std::stoll(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
                reflection->SetUInt64(proto_msg, field, std::stoull(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_SINT32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED32: {
                reflection->SetInt32(proto_msg, field, std::stoi(value));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
                reflection->SetUInt32(proto_msg, field, static_cast<uint32_t>(std::stoul(value)));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                reflection->SetBool(proto_msg, field, value == "1" || value == "true");
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_STRING:
            case google::protobuf::FieldDescriptor::TYPE_BYTES: {
                reflection->SetString(proto_msg, field, value);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                const google::protobuf::EnumValueDescriptor* enum_value =
                    field->enum_type()->FindValueByName(value);
                reflection->SetEnum(proto_msg, field, enum_value);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                google::protobuf::Message* sub_message = reflection->MutableMessage(proto_msg, field);
                sub_message->ParseFromString(value);
                break;
            }
            default: {
                throw std::runtime_error("Unsupported field type.");
            }
            }
        }
    }

    std::string GetField(const google::protobuf::Message& proto_msg, const google::protobuf::FieldDescriptor* field, const google::protobuf::Reflection* reflection) {
        std::string value;
        if (field->is_repeated()) {
            const google::protobuf::Message& sub_message = reflection->GetMessage(proto_msg, field);
            value = sub_message.SerializeAsString();
        }
        else {
            switch (field->type()) {
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                value = std::to_string(reflection->GetDouble(proto_msg, field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                value = std::to_string(reflection->GetFloat(proto_msg, field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            case google::protobuf::FieldDescriptor::TYPE_SINT64: {
                value = std::to_string(reflection->GetInt64(proto_msg, field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
                value = std::to_string(reflection->GetUInt64(proto_msg, field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SINT32: {
                value = std::to_string(reflection->GetInt32(proto_msg, field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
                value = std::to_string(reflection->GetUInt32(proto_msg, field));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                value = reflection->GetBool(proto_msg, field) ? "true" : "false";
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_STRING:
            case google::protobuf::FieldDescriptor::TYPE_BYTES: {
                value = reflection->GetString(proto_msg, field);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                int enum_value = reflection->GetEnumValue(proto_msg, field);
                value = std::to_string(enum_value);
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                const google::protobuf::Message& sub_message = reflection->GetMessage(proto_msg, field);
                value = sub_message.SerializeAsString();
                break;
            }
            default: {
                std::cerr << "unsupported field type:" << field->type() << std::endl;
            }
            }
        }
        return value;
    }

private:
    std::optional<sw::redis::Redis> redis_;
    std::optional<std::jthread> thread_;
    bool run_;

    std::queue<million::MsgUnique> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

} // namespace db
} // namespace million