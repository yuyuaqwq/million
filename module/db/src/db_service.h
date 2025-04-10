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
// cache可能存储行，原本设计是用来支持快速的持久化，但引入成本较高，目前暂且不使用
// sql存储行
// 跨行事务基于sql
// 数据模型：
// 一个服务持有所有权(指定定时回写)
// 其他服务可以直接向db查询该行数据(如果跨节点且没有启用redis的话，就得等回写才能读取到最新数据)
// 其他服务一般不会直接修改该行数据，除非是一些特殊场景
// 比如交易场景
// 拍卖行服务需要事务性的跨行操作，可能涉及到分布式事务

struct DBRowCache {
    DBRow db_row;
    uint64_t sql_db_version;
};
using DBTable = std::unordered_map<std::string, DBRowCache>;

MILLION_MSG_DEFINE(, DBRowTickSyncMsg, (int32_t) sync_tick, (DBRowCache*) cache);

class DBService : public IService {
    MILLION_SERVICE_DEFINE(DBService);

public:
    using Base = IService;
    DBService(IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        logger().Info("DbService Init");

        imillion().SetServiceName(service_handle(), kDbServiceName);

        auto handle = imillion().GetServiceByName(kSqlServiceName);
        if (!handle) {
            logger().Err("Unable to find SqlService.");
            return false;
        }
        sql_service_ = *handle;

        handle = imillion().GetServiceByName(kCacheServiceName);
        if (!handle) {
            logger().Err("Unable to find SqlService.");
            return false;
        }
        cache_service_ = *handle;

        return true;
    }

    MILLION_MSG_HANDLE(DBRowTickSyncMsg, msg) {
        if (msg->cache->db_row.db_version() > msg->cache->sql_db_version) {
            // co_await期间可能会有新的DbRowUpdateMsg消息被处理，这里复制过来再回写
            // 可以考虑优化成移动
            auto db_row = msg->cache->db_row.CopyDirtyTo();
            auto res = co_await CallOrNull<SqlUpdateMsgReq, SqlUpdateMsgResp>(sql_service_, std::move(db_row), msg->cache->sql_db_version);
            if (!res) {
                // 超时，可能只是sql暂时不能提供服务，可以重试
                logger().Err("SqlUpdate Timeout.");
            }
            else if (!res->success) {
                // todo: 这里未来考虑优化，比如发一个消息让原始服务得知指定行的db需要重新拉取版本

                // 这里不匹配的原因，可能是其他节点回写了这行的数据到SQL，一般是应用设计问题
                // 比如某玩家的基本数据，被两个节点的服务同时query并设置了回写，不是一个常见的场景
                TaskAbort("Sql tick sync failed, check the db version: {} -> {}.", msg->cache->sql_db_version, db_row.db_version());
            }
            else {
                msg->cache->sql_db_version = db_row.db_version();
            }
        }
        auto sync_tick = msg->sync_tick;
        Timeout(sync_tick, std::move(msg_));
        co_return nullptr;
    }

    MILLION_MSG_HANDLE(DbRowCreateMsg, msg) {
        auto res = co_await Call<SqlInsertMsgReq, SqlInsertMsgResp>(sql_service_, DBRow(std::move(msg->row_msg)), false);
        TaskAssert(res->success, "SqlInsert failed.");
        co_return std::move(msg_);
    }

    MILLION_MSG_HANDLE(DbRowQueryMsg, msg) {
        auto& desc = msg->table_desc;

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);

        auto db_row_cache = LocalQueryDBRow(desc, msg->primary_key);
        if (db_row_cache) {
            // 如果找到了，就直接返回
            msg->db_row.emplace(db_row_cache->db_row);
            co_return std::move(msg_);
        }

        auto proto_msg = imillion().proto_mgr().NewMessage(desc);
        TaskAssert(proto_msg, "proto_mgr().NewMessage failed.");
        auto db_row = DBRow(std::move(proto_msg));

        auto res = co_await RemoteQueryDBRow(msg->primary_key, std::move(db_row));

        if (msg->tick_write_back) {
            auto sql_db_version = res->db_row->db_version();
            msg->db_row.emplace(db_row);
            LocalCacheDBRow(desc, std::move(msg->primary_key), std::move(db_row), sql_db_version);
        }
        else {
            msg->db_row.emplace(std::move(db_row));
        }
        co_return std::move(msg_);
    }

    MILLION_MSG_HANDLE(DbRowUpdateMsg, msg) {
        auto db_row = msg->db_row;
        const auto& desc = db_row.GetDescriptor();
        const auto& reflection = db_row.GetReflection();

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");
        
        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        TaskAssert(primary_key_field_desc, "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());
        auto primary_key = GetField(db_row.get(), *primary_key_field_desc);
        TaskAssert(!primary_key.empty(), "primary_key is empty:{}.{}", table_name, options.primary_key());

        auto table_iter = tables_.find(&desc);
        TaskAssert(table_iter != tables_.end(), "Table does not exist.");
        auto row_iter = table_iter->second.find(primary_key);
        TaskAssert(row_iter != table_iter->second.end(), "Row does not exist.");

        if (msg->old_db_version > msg->db_row.db_version()
            ||  msg->db_row.db_version() <= row_iter->second.db_row.db_version()) {
            // 传入版本错误
            // 或需要更新的行比本地缓存的版本旧
            // 一般是当前节点有多个服务查询并更新了同一行
            TaskAbort("Local cache update failed, check the db version.");
        }

        //if (msg->update_to_cache) {
        //    // 可以通过redis支持不写sql的数据持久化
        //    auto set_res = co_await Call<CacheSetMsg>(cache_service_, msg->db_row, msg->old_db_version, false);
        //    if (!set_res->success) {
        //        // todo: 这里未来考虑优化，比如发一个消息让原始服务得知指定行的db需要重新拉取版本

        //        // 如果db_version不匹配，给一个异常，让外部应该重新query并重试
        //        TaskAbort("Remote cache update failed, check the db version.");
        //    }
        //}

        row_iter->second.db_row = std::move(msg->db_row);

        co_return std::move(msg_);
    }

private:
    DBTable& LocalGetTable(const google::protobuf::Descriptor& desc) {
        auto table_iter = tables_.find(&desc);
        if (table_iter == tables_.end()) {
            auto res = tables_.emplace(&desc, DbTable());
            assert(res.second);
            table_iter = res.first;
        }
        return table_iter->second;
    }

    DBRowCache* LocalQueryDBRow(const google::protobuf::Descriptor& desc, const std::string& primary_key) {
        auto& table = LocalGetTable(desc);
        auto iter = table.find(primary_key);
        if (iter == table.end()) {
            return nullptr;
        }
        return &iter->second;
    }

    bool LocalCacheDBRow(const google::protobuf::Descriptor& desc, std::string&& primary_key, DBRow&& row, uint64_t sql_db_version) {
        const MessageOptionsTable& options = desc.options().GetExtension(::million::db::table);
        auto sync_tick = options.sync_tick();

        auto& table = LocalGetTable(desc);

        auto res = table.emplace(std::move(primary_key), DBRowCache{ .db_row = std::move(row), .sql_db_version = sql_db_version });
        TaskAssert(res.second, "Duplicate cached db row.");

        Timeout<DBRowTickSyncMsg>(sync_tick, sync_tick, &res.first->second);

        return true;
    }

    Task<std::unique_ptr<SqlQueryMsgResp>> RemoteQueryDBRow(const std::string& primary_key, DBRow&& db_row) {
        auto sql_res = co_await Call<SqlQueryMsgReq, SqlQueryMsgResp>(sql_service_, std::move(db_row), primary_key);
        TaskAssert(sql_res->success, "SqlQueryMsg failed.");


        //// 直接读cache的row->db_version，可能不是sql的version
        //// 因为cache里的数据可能没有回写到sql中，所以还是需要先向sql查询获取sql的db_version
        //// 这里再读一遍cache是因为cache可能有更新的数据，读不到就还是使用sql的数据
        // auto proto_msg = imillion().proto_mgr().NewMessage(row->GetDescriptor());
        // TaskAssert(proto_msg, "proto_mgr().NewMessage failed.");
        // auto cache_row = DbRow(std::move(proto_msg));
        // co_await Call<CacheGetMsg>(cache_service_, primary_key, &cache_row, false);
        //
        //// 这里再检查下Cache版本，如果Cache版本低于Sql版本，则删除该Cache
        //// 需要由Del进行CAS的版本检查，如果删除时发现该行版本又变动了，则不删除该Cache
        // if (cache_row.db_version() < sql_db_version) {
        //     co_await Call<CacheDelMsg>(cache_service_, cache_row, false);
        // }
        // else {
        //     *row = std::move(cache_row);
        // }

        co_return std::move(sql_res);
    }

private:
    std::unordered_map<const protobuf::Descriptor*, DBTable> tables_;

    ServiceHandle cache_service_;
    ServiceHandle sql_service_;

    // 回写可以负载均衡，避免出现多个回写挤在同一秒进行
};

} // namespace db
} // namespace million