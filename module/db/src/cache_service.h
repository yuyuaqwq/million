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

    virtual bool OnInit() override {
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

    virtual void OnExit() override {
        redis_ = std::nullopt;
    }

    MILLION_MSG_DISPATCH(CacheService);

    MILLION_MUT_MSG_HANDLE(CacheGetMsg, msg) {
        auto& proto_msg = msg->db_row->get();
        const auto& desc = msg->db_row->GetDescriptor();

        auto redis_key = GetRedisKey(*msg->db_row);

        // 通过 Redis 哈希表存取 Protobuf 字段
        std::unordered_map<std::string, std::string> redis_hash;
        redis_->hgetall(redis_key, std::inserter(redis_hash, redis_hash.end()));
        if (redis_hash.empty()) {
            msg->success = false;
        }
        else {
            auto db_version = std::stoull(redis_hash["__db_version__"]);
            msg->db_row->set_db_version(db_version);

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

    MILLION_MUT_MSG_HANDLE(CacheSetMsg, msg) {
        std::vector<std::string> keys; 
        std::vector<std::string> args;
        MakeKeysAndArgs(msg->db_row, msg->old_db_version, &keys, &args);
        
        auto res = redis_->eval<long long>(kLuaSetScript, keys.begin(), keys.end(), args.begin(), args.end());
        if (res < 0) {
            msg->success = false;
            co_return std::move(msg_);
        }

        msg->success = true;
        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(CacheDelMsg, msg) {
        auto redis_key = GetRedisKey(msg->db_row);
        auto db_version = std::to_string(msg->db_row.db_version());
        auto res = redis_->eval<long long>(kLuaDelScript, { redis_key , db_version }, {});
        if (res < 0) {
            msg->success = false;
            co_return std::move(msg_);
        }
        msg->success = true;
        co_return std::move(msg_);
    }


    //MILLION_MUT_MSG_HANDLE(CacheBatchSetMsg, msg) {
    //    TaskAssert(msg->db_rows.size() == msg->old_db_version_list.size(), "DbRows Parameter quantity mismatch.");

    //    // redis_->watch();

    //    auto trans = redis_->transaction();

    //    std::vector<std::string> keys;
    //    std::vector<std::string> args;
    //    auto i = 0;
    //    for (auto& db_row : msg->db_rows) {
    //        keys.clear();
    //        args.clear();
    //        MakeKeysAndArgs(db_row.get(), msg->old_db_version_list[i], &keys, &args);
    //        trans.eval(kLuaSetScript, keys.begin(), keys.end(), args.begin(), args.end());
    //        ++i;
    //    }

    //    trans.exec();
    //    co_return std::move(msg_);
    //}

    MILLION_MUT_MSG_HANDLE(CacheGetBytesMsg, msg) {
        auto value =  redis_->get(msg->key_value);
        if (!value) {
            co_return std::move(msg_);
        }
        msg->key_value = std::move(*value);
        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(CacheSetBytesMsg, msg) {
        msg->success = redis_->set(msg->key, msg->value);
        co_return std::move(msg_);
    }

private:
    std::string GetRedisKey(const DbRow& db_row) {
        auto& proto_msg = db_row.get();
        const auto& desc = db_row.GetDescriptor();

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        const auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");

        //int32_t ttl = 0;
        //if (options.has_cache()) {
        //    const TableCacheOptions& cache_options = options.cache();
        //    ttl = cache_options.ttl();
        //}
        // options.tick_second();

        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        TaskAssert(primary_key_field_desc,
            "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());

        auto primary_key = GetField(proto_msg, *primary_key_field_desc);
        TaskAssert(!primary_key.empty(), "primary_key is empty:{}.{}", table_name, options.primary_key());

        // 使用 std::unordered_map 来存储要更新到 Redis 的字段和值
        // std::unordered_map<std::string, std::string> redis_hash;

        // 遍历 Protobuf 的所有字段，将字段和值存入 redis_hash 中
        auto redis_key = std::format("million:db:{}:{}", table_name, primary_key);

        return redis_key;
    }

    void MakeKeysAndArgs(const DbRow& db_row, uint64_t old_db_version, std::vector<std::string>* keys, std::vector<std::string>* args) {
        auto& proto_msg = db_row.get();
        const auto& desc = db_row.GetDescriptor();

        auto redis_key = GetRedisKey(db_row);

        keys->emplace_back(redis_key);
        keys->emplace_back(std::to_string(old_db_version));

        args->emplace_back("__db_version__");
        args->emplace_back(std::to_string(db_row.db_version()));
        for (int i = 0; i < desc.field_count(); ++i) {
            if (!db_row.IsDirtyFromFIeldIndex(i)) {
                continue;
            }

            const auto* field = desc.field(i);
            if (!field) {
                logger().Err("desc.field failed: {}.", i);
                continue;
            }

            const FieldOptionsColumn& options = field->options().GetExtension(column);

            //redis_hash[field->name()] = GetField(proto_msg, *field);

            args->emplace_back(field->name());
            args->emplace_back(GetField(proto_msg, *field));

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

        // redis_->hmset(redis_key, redis_hash.begin(), redis_hash.end());

        //if (ttl > 0) {
        //    redis_->expire(redis_key.data(), ttl);
        //}

    }

    
private:
    constexpr static std::string_view kLuaSetScript = R"(
        local unpack_func = (_VERSION == "Lua 5.1" or _VERSION == "Lua 5.2") and unpack or table.unpack
        local old_db_version = redis.call('HGET', KEYS[1], '__db_version__')
        local new_db_version = ARGV[2]
        if old_db_version == false or old_db_version == KEYS[2] and new_db_version > old_db_version then
            return redis.call('HSET', KEYS[1], unpack_func(ARGV))
        else
            return -1
        end
    )";
    constexpr static std::string_view kLuaDelScript = R"(
        local unpack_func = (_VERSION == "Lua 5.1" or _VERSION == "Lua 5.2") and unpack or table.unpack
        local old_db_version = redis.call('HGET', KEYS[1], '__db_version__')
        if old_db_version == KEYS[2] then
            return redis.call('DEL', KEYS[1])
        else
            return -1
        end
    )";

    std::optional<sw::redis::Redis> redis_;
};

} // namespace db
} // namespace million