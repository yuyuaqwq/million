#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <unordered_map>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/db.h>
#include <db/cache.h>
#include <db/sql.h>

#include <db/db_options.pb.h>
#include <db/db_example.pb.h>

namespace million {
namespace db {

// 设计：
// 本地db存储行，用于定期回写
// cache可能存储行，版本 >= sql，并且基于redis的事务来支持批量写
// sql存储行，版本一般都是最旧的

MILLION_MSG_DEFINE(, DbRowTickSyncMsg, (int32_t) sync_tick, (nonnull_ptr<DbRow>) cache_row, (uint64_t) old_db_version);

class DbService : public IService {
public:
    using Base = IService;
    DbService(IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit(MsgPtr msg) override {
        logger().Info("DbService Init");

        auto handle = imillion().GetServiceByName("SqlService");
        if (!handle) {
            logger().Err("Unable to find SqlService.");
            return false;
        }
        sql_service_ = *handle;

        handle = imillion().GetServiceByName("CacheService");
        if (!handle) {
            logger().Err("Unable to find SqlService.");
            return false;
        }
        cache_service_ = *handle;

        return true;
    }

    virtual void OnStop(::million::ServiceHandle sender, ::million::SessionId session_id) override {
        
    }

    MILLION_MSG_DISPATCH(DbService);

    MILLION_MUT_MSG_HANDLE(DbRowTickSyncMsg, msg) {
        if (msg->cache_row->db_version() > msg->old_db_version) {
            // co_await期间可能会有新的DbRowUpdateMsg消息被处理，这里复制过来再回写
            // 可以考虑优化成移动
            auto db_row = msg->cache_row->CopyDirtyTo();
            auto old_db_version = msg->old_db_version;
            msg->old_db_version = db_row.db_version();
            auto res = co_await CallOrNull<SqlUpdateMsg>(sql_service_, &db_row, old_db_version, false);
            if (!res) {
                // 超时，可能只是sql暂时不能提供服务，可以重试
                logger().Err("SqlUpdate Timeout.");
            }
            else {
                // todo: 这里未来考虑优化，比如回一个消息让原始服务得知指定行的db需要重新拉取版本

                // 如果为false，一般是回写失败，可能是当前行数据是低版本，无法入库，应用设计问题
                TaskAssert(res->success, "SqlUpdate Write back failed.");
            }
        }
        auto sync_tick = msg->sync_tick;
        Timeout(sync_tick, std::move(msg_));
        co_return nullptr;
    }

    MILLION_MUT_MSG_HANDLE(DbRowCreateMsg, msg) {
        auto db_row = DbRow(std::move(msg->row_msg));
        auto res = co_await Call<SqlInsertMsg>(sql_service_, &db_row, false);
        TaskAssert(res->success, "SqlInsert failed.");
        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(DbRowQueryMsg, msg) {
        auto& desc = msg->table_desc;

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);

        auto row = FindDbRow(desc, msg->primary_key);
        if (row) {
            // 如果找到了，就直接返回
            msg->db_row.emplace(*row);
            co_return std::move(msg_);
        }

        auto proto_msg = imillion().proto_mgr().NewMessage(desc);
        TaskAssert(proto_msg, "proto_mgr().NewMessage failed.");
        
        auto tmp_row = DbRow(std::move(proto_msg));
        row = &tmp_row;

        uint64_t sql_db_version = co_await QueryDbRow(msg->primary_key, row);

        msg->db_row.emplace(*row);

        if (msg->tick_write_back) {
            CacheDbRow(desc, std::move(msg->primary_key), std::move(*row), sql_db_version);
        }
        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(DbRowUpdateMsg, msg) {
        auto db_row = msg->db_row;
        const auto& desc = db_row.GetDescriptor();
        const auto& reflection = db_row.GetReflection();

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");
        
        auto table_iter = tables_.find(&desc);
        TaskAssert(table_iter != tables_.end(), "Table does not exist.");
        
        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        TaskAssert(primary_key_field_desc, "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());

        auto primary_key = GetField(db_row.get(), *primary_key_field_desc);
        TaskAssert(!primary_key.empty(), "primary_key is empty:{}.{}", table_name, options.primary_key());

        auto row_iter = table_iter->second.find(primary_key);
        TaskAssert(row_iter != table_iter->second.end(), "Row does not exist.");

        if (msg->db_row.db_version() <= row_iter->second.db_version()) {
            TaskAbort("msg->db_row->db_version() <= row_iter->second.db_version()", msg->db_row.db_version(), row_iter->second.db_version());
        }

        if (msg->update_to_cache) {
            // 可以通过redis保证数据持久化
            auto set_res = co_await Call<CacheSetMsg>(cache_service_, &msg->db_row, msg->old_db_version, false);
            if (!set_res->success) {
                // todo: 这里未来考虑优化，比如回一个消息让原始服务得知指定行的db需要重新拉取版本

                // 如果db_version不匹配，给一个异常，让外部应该重新get并重试
                TaskAbort("CacheSet failed.");
            }
        }

        row_iter->second = std::move(msg->db_row);

        co_return std::move(msg_);
    }

private:
    auto& GetTable(const google::protobuf::Descriptor& desc) {
        auto table_iter = tables_.find(&desc);
        if (table_iter == tables_.end()) {
            auto res = tables_.emplace(&desc, DbTable());
            assert(res.second);
            table_iter = res.first;
        }
        return table_iter->second;
    }

    DbRow* FindDbRow(const google::protobuf::Descriptor& desc, const std::string& primary_key) {
        auto& table = GetTable(desc);
        auto iter = table.find(primary_key);
        if (iter == table.end()) {
            return nullptr;
        }
        return &iter->second;
    }

    bool CacheDbRow(const google::protobuf::Descriptor& desc, std::string&& primary_key, DbRow&& row, uint64_t sql_db_version) {
        auto& table = GetTable(desc);

        auto res = table.emplace(std::move(primary_key), std::move(row));
        TaskAssert(res.second, "Duplicate cached db row.");

        const MessageOptionsTable& options = desc.options().GetExtension(::million::db::table);
        auto sync_tick = options.sync_tick();

        Timeout<DbRowTickSyncMsg>(sync_tick, sync_tick, &res.first->second, sql_db_version);
        return true;
    }

    Task<uint64_t> QueryDbRow(const std::string& primary_key, DbRow* row) {
        auto sql_res = co_await Call<SqlQueryMsg>(sql_service_, primary_key, row, false);
        TaskAssert(sql_res->success, "SqlQueryMsg failed.");
        auto sql_db_version = row->db_version();

        // 直接读cache的row->db_version，可能不是sql的version
        // 因为cache里的数据可能没有回写到sql中，所以还是需要先向sql查询获取sql的db_version
        // 这里再读一遍cache是因为cache可能有更新的数据，读不到就还是使用sql的数据
        co_await Call<CacheGetMsg>(cache_service_, primary_key, row, false);

        co_return sql_db_version;
    }

private:
    using DbTable = std::unordered_map<std::string, DbRow>;
    std::unordered_map<const protobuf::Descriptor*, DbTable> tables_;

    ServiceHandle cache_service_;
    ServiceHandle sql_service_;

    // 回写可以负载均衡，避免出现多个回写挤在同一秒进行
};

} // namespace db
} // namespace million