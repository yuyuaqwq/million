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

#include <db/api.h>
#include <db/db_row.h>
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
        //const protobuf::DescriptorPool* pool = protobuf::DescriptorPool::generated_pool();
        //protobuf::DescriptorDatabase* db = pool->internal_generated_database();
        //if (db == nullptr) {
        //    return;
        //}
        //std::vector<std::string> file_names;
        //db->FindAllFileNames(&file_names);   // 遍历得到所有proto文件名
        //for (const std::string& filename : file_names) {
        //    const protobuf::FileDescriptor* file_desc = pool->FindFileByName(filename);
        //    if (file_desc == nullptr) continue;
        //    RegisterProto(*file_desc);
        //}
    }

    bool RegisterProto(const protobuf::FileDescriptor& file_desc) {
        // 获取该文件的options，确认是否设置了db
        const auto& file_options = file_desc.options();
        // 检查 db 是否被设置
        if (!file_options.HasExtension(Db::db)) {
            return false;
        }
        const auto& db_options = file_options.GetExtension(Db::db);

        int message_count = file_desc.message_type_count();
        for (int i = 0; i < message_count; i++) {
            const protobuf::Descriptor* desc = file_desc.message_type(i);
            if (!desc) continue;
            const auto& msg_options = desc->options();
            if (!msg_options.HasExtension(Db::table)) {
                continue;
            }
            const auto& table_options = msg_options.GetExtension(Db::table);
            const auto& table_name = table_options.name();
            table_map_.emplace(table_name, desc);
        }
        return true;
    }

    const protobuf::Descriptor* GetMsgDesc(const std::string& table_name) {
        auto iter = table_map_.find(table_name);
        if (iter == table_map_.end()) {
            return nullptr;
        }
        return iter->second;
    }

    std::optional<ProtoMsgUnique> NewMessage(const protobuf::Descriptor& desc) {
        const protobuf::Message* proto_msg = protobuf::MessageFactory::generated_factory()->GetPrototype(&desc);
        if (proto_msg != nullptr) {
            return ProtoMsgUnique(proto_msg->New());
        }
        return std::nullopt;
    }

    const auto& table_map() const { return table_map_; }

private:
    std::unordered_map<std::string, const protobuf::Descriptor*> table_map_;
};



// DbService使用lru来淘汰row
// 可能n秒脏row需要入库

// 查询时，会返回所有权给外部服务，避免lru淘汰导致指针无效
// 外部必须有一份数据，dbservice也可能有一份，但是会通过lru来自动淘汰
// 外部修改后，通过发送标记脏消息来通知dbservice可以更新此消息


MILLION_MSG_DEFINE_EMPTY(DB_CLASS_API, DbSqlInitMsg);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowExistMsg, (std::string) table_name, (std::string) primary_key, (bool) exist);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowGetMsg, (std::string) table_name, (std::string) primary_key, (DbRow) db_row);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowSetMsg, (std::string) table_name, (std::string) primary_key, (DbRow*) db_row);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowDeleteMsg, (std::string) table_name, (std::string) primary_key, (DbRow*) db_row);


class DbService : public IService {
public:

    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        logger().Info("DbService Init");

        proto_codec_.Init();

        return true;
    }

    virtual void OnExit() override {
        
    }

    MILLION_MSG_DISPATCH(DbService);

    MILLION_MSG_HANDLE(DbSqlInitMsg, msg) {
        sql_service_ = msg->sender();
        for (const auto& table_info : proto_codec_.table_map()) {
            co_await Call<SqlCreateTableMsg>(sql_service_, table_info.second);
        }

        //::Million::Proto::Db::Example::User user;
        //user.set_name("sb");
        //user.New();
        // db_row.MarkDirty(user.kNameFieldNumber);

        // user.kEmailFieldNumber;

        //user.set_email("fake@qq.com");
        //user.set_phone_number("1234567890");
        //user.set_password_hash("AWDaoDWHGOAUGH");

        //co_await Call<SqlInsertMsg>(sql_service_, &user);

        //std::vector<bool> dirty_bits(user.GetDescriptor()->field_count());
        //auto res = co_await Call<SqlQueryMsg>(sql_service_, "1", &user, &dirty_bits, false);

        co_return;
    }

    MILLION_MSG_HANDLE(DbRowGetMsg, msg) {
        const auto* desc = proto_codec_.GetMsgDesc(msg->table_name);
        if (!desc) {
            logger().Err("Unregistered table name: {}.", msg->table_name);
            co_return;
        }

        auto rows_iter = tables_.find(desc);
        if (rows_iter == tables_.end()) {
            auto res = tables_.emplace(desc, DbRows());
            assert(res.second);
            rows_iter = res.first;
        }
        auto& rows = rows_iter->second;

        auto row_iter = rows.find(msg->primary_key);
        do {
            if (row_iter != rows.end()) {
                break;
            }

            auto proto_msg_opt = proto_codec_.NewMessage(*desc);
            if (!proto_msg_opt) {
                logger().Err("proto_codec_.NewMessage failed: {}.", msg->table_name);
                co_return;
            }
            auto proto_msg = std::move(*proto_msg_opt);

            auto row = DbRow(*desc, std::move(proto_msg));
            auto res_msg = co_await Call<CacheGetMsg>(cache_service_, msg->primary_key, &row, false);
            if (!res_msg->success) {
                auto res_msg = co_await Call<SqlQueryMsg>(sql_service_, msg->primary_key, &row, false);
            }

            auto res = rows.emplace(std::move(msg->primary_key), std::move(row));
            assert(res.second);
            row_iter = res.first;
        } while (false);
        msg->db_row = row_iter->second;
        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(DbRowSetMsg, msg) {

        co_return;
    }

private:
    DbProtoCodec proto_codec_;

    // 改用lru
    using DbRows = std::unordered_map<std::string, DbRow>;
    std::unordered_map<const protobuf::Descriptor*, DbRows> tables_;

    ServiceHandle cache_service_;
    ServiceHandle sql_service_;
};

} // namespace db
} // namespace million