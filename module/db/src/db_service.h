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

// DbService使用lru来淘汰row
// 可能n秒脏row需要入库

// 查询时，会返回所有权给外部服务，避免lru淘汰导致指针无效
// 外部必须有一份数据，dbservice也可能有一份，但是会通过lru来自动淘汰
// 外部修改后，通过发送标记脏消息来通知dbservice可以更新此消息

// 使用本地wal来支持事务和持久化
// wal写满时，就将所有内存中的数据回写到sql，然后清空wal

// 另外支持跨节点访问其他db，由sql支持，sql提供当前行被哪个节点所持有

MILLION_MSG_DEFINE(, DbRowTickSyncMsg, (int32_t) sync_tick, (nonnull_ptr<DbRow>) db_row);

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

    MILLION_MSG_HANDLE(DbRowTickSyncMsg, msg) {
        if (msg->db_row->IsDirty()) {
            // co_await期间可能会有新的DbRowSetMsg消息被处理，所以需要复制一份
            try {
                auto db_row = msg->db_row->CopyDirtyTo();
                msg->db_row->ClearDirty();
                co_await Call<SqlUpdateMsg>(sql_service_, &db_row);
                // co_await Call<CacheSetMsg>(cache_service_, &db_row);
            }
            catch (const std::exception& e) {
                logger().Err("DbRowTickSyncMsg: {}", e.what());
            }
        }
        auto sync_tick = msg->sync_tick;
        Timeout(sync_tick, std::move(msg_));
        co_return nullptr;
    }


    MILLION_MUT_MSG_HANDLE(DbRowGetMsg, msg) {
        auto& desc = msg->table_desc;
        if (!desc.options().HasExtension(table)) {
            logger().Err("HasExtension table failed.");
            co_return std::move(msg_);
        }
        const MessageOptionsTable& options = desc.options().GetExtension(table);

        auto table_iter = tables_.find(&desc);
        if (table_iter == tables_.end()) {
            auto res = tables_.emplace(&desc, DbTable());
            assert(res.second);
            table_iter = res.first;
        }
        auto& table = table_iter->second;

        auto row_iter = table.find(msg->primary_key);
        do {
            if (row_iter != table.end()) {
                break;
            }

            auto proto_msg = imillion().proto_mgr().NewMessage(desc);
            if (!proto_msg) {
                logger().Err("proto_mgr().NewMessage failed.");
                co_return nullptr;
            }

            auto row = DbRow(std::move(proto_msg));
            //auto res_msg = co_await Call<CacheGetMsg>(cache_service_, msg->primary_key, &row, false);
            //if (!res_msg->success) {
                auto res_msg = co_await Call<SqlQueryMsg>(sql_service_, msg->primary_key, &row, false);
                if (!res_msg->success) {
                    co_await Call<SqlInsertMsg>(sql_service_, &row);
                }
                //row.MarkDirty();
                //co_await Call<CacheSetMsg>(cache_service_, &row);
                //row.ClearDirty();
            //}

            auto res = table.emplace(std::move(msg->primary_key), std::move(row));
            assert(res.second);
            row_iter = res.first;

            const MessageOptionsTable& options = desc.options().GetExtension(::million::db::table);

            auto sync_tick = options.sync_tick();
            Timeout<DbRowTickSyncMsg>(sync_tick, sync_tick, &row_iter->second);

        } while (false);
        msg->db_row = row_iter->second;

        co_return std::move(msg_);
    }

    MILLION_MSG_HANDLE(DbRowSetMsg, msg) {
        auto db_row = msg->db_row;
        const auto& desc = db_row->GetDescriptor();
        const auto& reflection = db_row->GetReflection();
        if (!desc.options().HasExtension(table)) {
            logger().Err("HasExtension table failed.");
            co_return nullptr;
        }
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        auto& table_name = options.name();

        auto table_iter = tables_.find(&desc);
        if (table_iter == tables_.end()) {
            logger().Err("Table does not exist.");
            co_return nullptr;
        }

        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        if (!primary_key_field_desc) {
            logger().Err("FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());
            co_return nullptr;
        }
        std::string primary_key;
        primary_key = reflection.GetStringReference(db_row->get(), primary_key_field_desc, &primary_key);
        if (primary_key.empty()) {
            logger().Err("primary_key is empty.");
            co_return nullptr;
        }

        auto row_iter = table_iter->second.find(primary_key);
        if (row_iter == table_iter->second.end()) {
            logger().Err("Row does not exist.");
            co_return nullptr;
        }

        row_iter->second.CopyFromDirty(*msg->db_row);

        co_return nullptr;
    }


private:
    // 改用lru
    using DbTable = std::unordered_map<std::string, DbRow>;
    std::unordered_map<const protobuf::Descriptor*, DbTable> tables_;

    ServiceHandle cache_service_;
    ServiceHandle sql_service_;


    // 回写负载均衡

};

} // namespace db
} // namespace million