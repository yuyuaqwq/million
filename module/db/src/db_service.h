#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <unordered_map>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

#include <million/imillion.h>
#include <million/proto.h>

#include <db/db.h>
#include <db/cache.h>
#include <db/sql.h>

#include <protogen/db/db_options.pb.h>
#include <protogen/db/db_example.pb.h>

namespace million {
namespace db {

namespace protobuf = google::protobuf;
class DbProtoCodec : noncopyable{
public:
    // 初始化
    void Init() {
    }

    const protobuf::FileDescriptor* RegisterProto(const std::string& proto_file_name) {
        const auto* pool = protobuf::DescriptorPool::generated_pool();
        auto* db = pool->internal_generated_database();

        std::vector<std::string> file_names;
        db->FindAllFileNames(&file_names);   // 遍历得到所有proto文件名
        //for (const std::string& filename : file_names) {
        //    const auto* file_desc = pool->FindFileByName(filename);
        //}

        auto file_desc = pool->FindFileByName(proto_file_name);
        if (!file_desc) {
            return nullptr;
        }

        // 获取该文件的options，确认是否设置了db
        const auto& file_options = file_desc->options();
        // 检查 db 是否被设置
        if (!file_options.HasExtension(Db::db)) {
            return nullptr;
        }
        const auto& db_options = file_options.GetExtension(Db::db);

        int message_count = file_desc->message_type_count();
        for (int i = 0; i < message_count; i++) {
            const auto* desc = file_desc->message_type(i);
            if (!desc) continue;
            const auto& msg_options = desc->options();
            if (!msg_options.HasExtension(Db::table)) {
                continue;
            }
            // 得加这个才会初始化，不知道具体原因
            message_factory_ = protobuf::MessageFactory::generated_factory();
            const auto* msg = message_factory_->GetPrototype(desc);
            if (!msg) {
                continue;
            }

            const auto& table_options = msg_options.GetExtension(Db::table);
            const auto& table_name = table_options.name();
            // table_map_.emplace(table_name, desc);
        }
        return file_desc;
    }

    //const protobuf::Descriptor* GetMsgDesc(const std::string& table_name) {
    //    auto iter = table_map_.find(table_name);
    //    if (iter == table_map_.end()) {
    //        return nullptr;
    //    }
    //    return iter->second;
    //}

    std::optional<ProtoMsgUnique> NewMessage(const protobuf::Descriptor& desc) {
        const auto* proto_msg = message_factory_->GetPrototype(&desc);
        if (proto_msg != nullptr) {
            return ProtoMsgUnique(proto_msg->New());
        }
        return std::nullopt;
    }

    // const auto& table_map() const { return table_map_; }

private:
    protobuf::MessageFactory* message_factory_;

    // std::unordered_map<std::string, const protobuf::Descriptor*> table_map_;
};


// DbService使用lru来淘汰row
// 可能n秒脏row需要入库

// 查询时，会返回所有权给外部服务，避免lru淘汰导致指针无效
// 外部必须有一份数据，dbservice也可能有一份，但是会通过lru来自动淘汰
// 外部修改后，通过发送标记脏消息来通知dbservice可以更新此消息


MILLION_MSG_DEFINE(, DbRowTickSyncMsg, (int32_t) tick_second, (nonnull_ptr<DbRow>) db_row);

class DbService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        logger().Info("DbService Init");

        proto_codec_.Init();

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

    virtual void OnStop() override {
        
    }

    MILLION_MSG_DISPATCH(DbService);

    MILLION_MSG_HANDLE(DbRowTickSyncMsg, msg) {
        if (msg->db_row->IsDirty()) {
            // co_await期间可能会有新的DbRowSetMsg消息被处理，所以需要复制一份
            try {
                auto db_row = msg->db_row->CopyDirtyTo();
                msg->db_row->ClearDirty();
                co_await Call<SqlUpdateMsg>(sql_service_, make_nonnull(&db_row));
                co_await Call<CacheSetMsg>(cache_service_, make_nonnull(&db_row));
            }
            catch (const std::exception& e) {
                logger().Err("DbRowTickSyncMsg: {}", e.what());
            }
        }
        auto tick_second = msg->tick_second;

        // auto msg2 = std::make_unique<DbRowTickSyncMsg>(100, msg->db_row);
        // Timeout(tick_second, std::move(msg2));
        co_return;
    }

    MILLION_MSG_HANDLE(DbProtoRegisterMsg, msg) {
        const auto* file_desc = proto_codec_.RegisterProto(msg->proto_file_name);
        if (!file_desc) {
            logger().Err("DbService RegisterProto failed: {}.", msg->proto_file_name);
            co_return;
        }
        int message_count = file_desc->message_type_count();
        for (int i = 0; i < message_count; i++) {
            const auto* desc = file_desc->message_type(i);
            if (!desc) continue;
            
            const auto& msg_options = desc->options();
            if (!msg_options.HasExtension(Db::table)) {
                continue;
            }
            const auto& table_options = msg_options.GetExtension(Db::table);

            co_await CallOrNull<SqlTableInitMsg>(sql_service_, *desc);

            const auto& table_name = table_options.name();
            logger().Info("SqlTableInitMsg success: {}", table_name);

            Reply(std::move(msg));
        }
    }

    MILLION_MSG_HANDLE(DbRowGetMsg, msg) {
        auto& desc = msg->table_desc; // proto_codec_.GetMsgDesc(msg->table_name);
        if (!desc.options().HasExtension(Db::table)) {
            logger().Err("HasExtension Db::table failed.");
            co_return;
        }
        const Db::MessageOptionsTable& options = desc.options().GetExtension(Db::table);

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

            auto proto_msg_opt = proto_codec_.NewMessage(desc);
            if (!proto_msg_opt) {
                logger().Err("proto_codec_.NewMessage failed.");
                co_return;
            }
            auto proto_msg = std::move(*proto_msg_opt);

            auto row = DbRow(std::move(proto_msg));
            auto res_msg = co_await Call<CacheGetMsg>(cache_service_, msg->primary_key, make_nonnull(&row), false);
            if (!res_msg->success) {
                auto res_msg = co_await Call<SqlQueryMsg>(sql_service_, msg->primary_key, make_nonnull(&row), false);
                if (!res_msg->success) {
                    co_await Call<SqlInsertMsg>(sql_service_, make_nonnull(&row));
                }
                row.MarkDirty();
                co_await Call<CacheSetMsg>(cache_service_, make_nonnull(&row));
                row.ClearDirty();
            }

            auto res = table.emplace(std::move(msg->primary_key), std::move(row));
            assert(res.second);
            row_iter = res.first;

            const Db::MessageOptionsTable& options = desc.options().GetExtension(Db::table);

            auto tick_second = options.tick_second();

            Timeout<DbRowTickSyncMsg>(tick_second, tick_second, make_nonnull(&row_iter->second));

        } while (false);
        msg->db_row = row_iter->second;

        Reply(std::move(msg));
    }

    MILLION_MSG_HANDLE(DbRowSetMsg, msg) {
        auto db_row = msg->db_row;
        const auto& desc = db_row->GetDescriptor();
        const auto& reflection = db_row->GetReflection();
        if (!desc.options().HasExtension(Db::table)) {
            logger().Err("HasExtension Db::table failed.");
            co_return;
        }
        const Db::MessageOptionsTable& options = desc.options().GetExtension(Db::table);
        auto& table_name = options.name();

        auto table_iter = tables_.find(&desc);
        if (table_iter == tables_.end()) {
            logger().Err("Table does not exist.");
            co_return;
        }

        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        if (!primary_key_field_desc) {
            logger().Err("FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());
            co_return;
        }
        std::string primary_key;
        primary_key = reflection.GetStringReference(db_row->get(), primary_key_field_desc, &primary_key);
        if (primary_key.empty()) {
            logger().Err("primary_key is empty.");
            co_return;
        }

        auto row_iter = table_iter->second.find(primary_key);
        if (row_iter == table_iter->second.end()) {
            logger().Err("Row does not exist.");
            co_return;
        }

        row_iter->second.CopyFromDirty(*msg->db_row);

        co_return;
    }


private:
    DbProtoCodec proto_codec_;

    // 改用lru
    using DbTable = std::unordered_map<std::string, DbRow>;
    std::unordered_map<const protobuf::Descriptor*, DbTable> tables_;

    ServiceHandle cache_service_;
    ServiceHandle sql_service_;
};

} // namespace db
} // namespace million