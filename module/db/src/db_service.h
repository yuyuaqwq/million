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

// 另外可以支持跨节点访问其他db，可以由redis来支持，存储该路径的行位于哪个节点的db中
// 另外也可以由redis来确定该行是否被锁定，以及该行数据是否成功回写sql
// 或者说不使用本地wal，而是通过redis来支持事务？

// 置脏的数据会被放到redis中，来保证可用，需要批量写的数据，就批量写到redis中

// 1.置脏后，数据放入redis，同时db也会有一份这个数据，等待tick回写到sql
// 2.tick触发后，回写到sql，并且当前

MILLION_MSG_DEFINE(, DbRowTickSyncMsg, (int32_t) sync_tick, (nonnull_ptr<DbRow>) cache_row, (DbRow) tmp_row);

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
        if (msg->cache_row->IsDirty()) {
            // co_await期间可能会有新的DbRowPushMsg消息被处理，这里移动过来再更新
            try {
                msg->tmp_row.MoveFrom(msg->cache_row.get());
                co_await Call<SqlUpdateMsg>(sql_service_, &msg->tmp_row);

                // co_await Call<CacheSetMsg>(cache_service_, &msg->tmp_row);
            }
            catch (const std::exception& e) {
                logger().Err("DbRowTickSyncMsg: {}", e.what());
            }
        }
        auto sync_tick = msg->sync_tick;
        Timeout(sync_tick, std::move(msg_));
        co_return nullptr;
    }

    MILLION_MUT_MSG_HANDLE(DbRowCreateMsg, msg) {
        auto& desc = *msg->row_msg->GetDescriptor();
        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        
        const auto& reflection = *msg->row_msg->GetReflection();

        const MessageOptionsTable& options = desc.options().GetExtension(table);
        auto& table_name = options.name();

        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        TaskAssert(primary_key_field_desc, "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());
        
        auto primary_key = GetField(*msg->row_msg.get(), *primary_key_field_desc);

        auto db_row = DbRow(std::move(msg->row_msg));
        msg->db_row = db_row;
        co_await Call<SqlInsertMsg>(sql_service_, &*msg->db_row);
        CacheDbRow(desc, std::move(primary_key), std::move(db_row));
        co_return std::move(msg_);
    }


    MILLION_MUT_MSG_HANDLE(DbRowGetMsg, msg) {
        auto& desc = msg->table_desc;
        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        
        const MessageOptionsTable& options = desc.options().GetExtension(table);

        auto row = FindDbRow(desc, msg->primary_key);
        if (row) {
            // 如果找到了，就直接返回
            msg->db_row = *row;
            co_return std::move(msg_);
        }

        auto proto_msg = imillion().proto_mgr().NewMessage(desc);
        TaskAssert(proto_msg, "proto_mgr().NewMessage failed.");
        
        auto tmp_row = DbRow(std::move(proto_msg));
        row = &tmp_row;

        auto res_msg = co_await Call<CacheGetMsg>(cache_service_, msg->primary_key, row, false);
        if (!res_msg->success) {
            auto res_msg = co_await Call<SqlQueryMsg>(sql_service_, msg->primary_key, row, false);
            TaskAssert(res_msg->success, "SqlQueryMsg failed.");
            
            //row.MarkDirty();
            //co_await Call<CacheSetMsg>(cache_service_, row);
            //row.ClearDirty();
        }

        CacheDbRow(desc, std::move(msg->primary_key), std::move(*row));
        co_return std::move(msg_);
    }

    MILLION_MSG_HANDLE(DbRowUpdateMsg, msg) {
        auto db_row = msg->db_row;
        const auto& desc = db_row->GetDescriptor();
        const auto& reflection = db_row->GetReflection();
        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        auto& table_name = options.name();

        auto table_iter = tables_.find(&desc);
        TaskAssert(table_iter != tables_.end(), "Table does not exist.");
        
        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        TaskAssert(primary_key_field_desc, "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());
        
        auto primary_key = GetField(db_row->get(), *primary_key_field_desc);
        TaskAssert(!primary_key.empty(), "primary_key is empty:{}.{}", table_name, options.primary_key());

        auto row_iter = table_iter->second.find(primary_key);
        TaskAssert(row_iter != table_iter->second.end(), "Row does not exist.");

        row_iter->second.CopyFromDirty(*msg->db_row);

        // 先回写到redis，让redis保证数据可靠
        co_await Call<CacheSetMsg>(cache_service_, msg->db_row);

        co_return nullptr;
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

    bool CacheDbRow(const google::protobuf::Descriptor& desc, std::string&& primary_key, DbRow&& row) {
        auto& table = GetTable(desc);

        auto res = table.emplace(std::move(primary_key), std::move(row));
        if (!res.second) {
            return false;
        }
        assert(res.second);

        const MessageOptionsTable& options = desc.options().GetExtension(::million::db::table);
        auto sync_tick = options.sync_tick();

        auto tmp_msg = imillion().proto_mgr().NewMessage(desc);
        TaskAssert(tmp_msg, "proto_mgr().NewMessage Failed.");
        
        Timeout<DbRowTickSyncMsg>(sync_tick, sync_tick, &res.first->second, DbRow(std::move(tmp_msg)));

        return true;
    }


private:
    // 改用lru
    using DbTable = std::unordered_map<std::string, DbRow>;
    std::unordered_map<const protobuf::Descriptor*, DbTable> tables_;

    ServiceHandle cache_service_;
    ServiceHandle sql_service_;


    // 回写可以负载均衡，避免出现多个回写挤在同一秒进行

};

} // namespace db
} // namespace million