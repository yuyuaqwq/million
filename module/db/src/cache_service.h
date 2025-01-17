#pragma once

#include <format>
#include <iostream>
#include <queue>

#include <sw/redis++/redis++.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <db/db_options.pb.h>

#include <million/imillion.h>
#include <million/msg.h>

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

    virtual bool OnInit(MsgPtr msg) override {
        try {
            // 创建 Redis 对象并连接到 Redis 服务器
            redis_.emplace(std::format("tcp://{}:{}", host, port));
        }
        catch (const sw::redis::Error& e) {
            logger().Err("Redis error:{}", e.what());
        }

        imillion().EnableSeparateWorker(service_handle());

        return true;
    }

    virtual void OnExit(ServiceHandle sender, SessionId session_id) override {
        redis_ = std::nullopt;
    }

    MILLION_MSG_DISPATCH(CacheService);

    MILLION_MUT_MSG_HANDLE(CacheGetMsg, msg) {
        auto& proto_msg = msg->db_row->get();
        const auto& desc = msg->db_row->GetDescriptor();
        const auto& reflection = msg->db_row->GetReflection();

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);

        const auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");

        // 通过 Redis 哈希表存取 Protobuf 字段
        std::unordered_map<std::string, std::string> redis_hash;
        auto redis_key = std::format("million:db:{}:{}", table_name, msg->primary_key);
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

        co_return std::move(msg_);
    }

    MILLION_MSG_HANDLE(CacheSetMsg, msg) {
        auto& proto_msg = msg->db_row->get();
        if (!msg->db_row->IsDirty()) {
            co_return std::move(msg_);
        }

        const auto& desc = msg->db_row->GetDescriptor();
        const auto& reflection = msg->db_row->GetReflection();

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        const auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");

        int32_t ttl = 0;
        if (options.has_cache()) {
            const TableCacheOptions& cache_options = options.cache();
            ttl = cache_options.ttl();
        }
        // options.tick_second();

        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());

        TaskAssert(primary_key_field_desc, 
            "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());
        
        auto primary_key = GetField(proto_msg, *primary_key_field_desc);
        TaskAssert(!primary_key.empty(), "primary_key is empty:{}.{}", table_name, options.primary_key());

        // 使用 std::unordered_map 来存储要更新到 Redis 的字段和值
        std::unordered_map<std::string, std::string> redis_hash;

        // 遍历 Protobuf 的所有字段，将字段和值存入 redis_hash 中
        for (int i = 0; i < desc.field_count(); ++i) {
            if (!msg->db_row->IsDirtyFromFIeldIndex(i)) {
                continue;
            }

            const auto* field = desc.field(i);
            if (!field) {
                logger().Err("desc.field failed: {}.", i);
                continue;
            }

            const FieldOptionsColumn& options = field->options().GetExtension(column);

            redis_hash[field->name()] = GetField(proto_msg, *field);

            // 主键在上面获取
            //if (options.has_cache()) {
            //    const auto& cache_options = options.cache();
            //    if (cache_options.index()) {
            //        if (!primary_key.empty()) {
            //            logger().Err("there can only be one index:{}", field->name());
            //        }
            //        if (field->is_repeated()) {
            //            logger().Err("index cannot be an array:{}", field->name());
            //        }
            //        else {
            //            primary_key = redis_hash[field->name()];
            //        }
            //    }
            //}
        }

        auto redis_key = std::format("million:db:{}:{}", table_name, primary_key);

        redis_->hmset(redis_key, redis_hash.begin(), redis_hash.end());

        if (ttl > 0) {
            redis_->expire(redis_key.data(), ttl);
        }

        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(CacheGetBytesMsg, msg) {
        auto value =  redis_->get("million:" + msg->key_value);
        if (!value) {
            co_return std::move(msg_);
        }
        msg->key_value = std::move(*value);
        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(CacheSetBytesMsg, msg) {
        msg->success = redis_->set("million:" + msg->key, msg->value);
        co_return std::move(msg_);
    }


private:

private:
    std::optional<sw::redis::Redis> redis_;
};

} // namespace db
} // namespace million