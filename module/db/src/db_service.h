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

// Lock时，返回所有权给外部服务
// 外部必须有一份数据，dbservice也可能有一份
// 外部修改后，通过发送标记脏消息来通知dbservice可以更新此消息

// 另外可以支持跨节点访问其他db，可以由redis来支持，存储该路径的行位于哪个节点的db中
// 另外也可以由redis来确定该行是否被锁定，以及该行数据是否成功回写sql
// 或者说不使用本地wal，而是通过redis来支持事务？

// 置脏的数据会被放到redis中，来保证可用，需要批量写的数据，就批量写到redis中

// 1.置脏后，数据放入redis，同时db也会有一份这个数据，等待tick回写到sql
// 2.tick触发后，回写到sql

// 目前看只能让用户分开处理，如果玩家在线，就投递给玩家服务处理，否则再自己抢占写锁
// 抢占写锁交给db服务进行
// 当然可能会出现，首先判断玩家不在线，然后准备抢占时，玩家上线并抢占了写锁，导致失败的情况
// 这种情况发生的概率很低，让交易服务抢占失败也不是不行，最多就是玩家重试一次交易

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
            // co_await期间可能会有新的DbRowPushMsg消息被处理，这里移动过来再回写
            msg->tmp_row.MoveFrom(msg->cache_row.get());
            auto res = co_await CallOrNull<SqlUpdateMsg>(sql_service_, &msg->tmp_row);
            if (!res) {
                logger().Err("SqlUpdateMsg Timeout.");
            }
        }
        auto sync_tick = msg->sync_tick;
        Timeout(sync_tick, std::move(msg_));
        co_return nullptr;
    }

    MILLION_MUT_MSG_HANDLE(DbRowCreateMsg, msg) {
        auto db_row = DbRow(std::move(msg->row_msg));
        co_await Call<SqlInsertMsg>(sql_service_, &db_row);
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

        auto res_msg = co_await Call<CacheGetMsg>(cache_service_, msg->primary_key, row, false);
        // Lock过程中res_msg->success也会返回false，不用担心获取到空数据
        if (!res_msg->success) {
            auto res_msg = co_await Call<SqlQueryMsg>(sql_service_, msg->primary_key, row, false);
            TaskAssert(res_msg->success, "SqlQueryMsg failed.");
        }

        // 仅查询的情况，不在db cache
        msg->db_row.emplace(std::move(*row));
        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(DbRowLockMsg, msg) {
        auto& desc = msg->table_desc;

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");

        // 这里用该行是否在redis中来判断是否被占用，存在已锁定，否则未锁定
        auto lock_res = co_await Call<CacheLockMsg>(cache_service_, std::format("million:db:{}:{}", table_name, msg->primary_key), false);
        if (!lock_res->success) {
            // 已被锁定
            co_return std::move(msg_);
        }

        auto proto_msg = imillion().proto_mgr().NewMessage(desc);
        TaskAssert(proto_msg, "proto_mgr().NewMessage failed.");

        auto row = DbRow(std::move(proto_msg));
        auto sql_res = co_await Call<SqlQueryMsg>(sql_service_, msg->primary_key, &row, false);
        TaskAssert(sql_res->success, "SqlQueryMsg failed.");
        
        row.MarkDirty();
        co_await Call<CacheSetMsg>(cache_service_, &row);
        row.ClearDirty();

        msg->db_row.emplace(row);
        CacheDbRow(desc, std::move(msg->primary_key), std::move(row));
        co_return std::move(msg_);
    }


    MILLION_MSG_HANDLE(DbRowUpdateMsg, msg) {
        auto db_row = msg->db_row;
        const auto& desc = db_row->GetDescriptor();
        const auto& reflection = db_row->GetReflection();

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        auto& table_name = options.name();
        TaskAssert(table_name.empty(), "table_name is empty.");
        
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
        TaskAssert(res.second, "Duplicate cached db row.");

        const MessageOptionsTable& options = desc.options().GetExtension(::million::db::table);
        auto sync_tick = options.sync_tick();

        auto tmp_msg = imillion().proto_mgr().NewMessage(desc);
        TaskAssert(tmp_msg, "proto_mgr().NewMessage Failed.");
        
        Timeout<DbRowTickSyncMsg>(sync_tick, sync_tick, &res.first->second, DbRow(std::move(tmp_msg)));
        return true;
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