#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <unordered_map>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

#include <million/imillion.h>
#include <million/proto_msg.h>

#include <db/cache_msg.h>
#include <db/sql_msg.h>

#include <protogen/db/db_options.pb.h>

#include <million/logger/logger.h>

#include <db/api.h>

namespace million {
namespace db {

namespace protobuf = google::protobuf;
class DbProtoMgr : noncopyable{
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


MILLION_MSG_DEFINE_EMPTY(DB_CLASS_API, DbSqlInitMsg);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowQueryMsg, (std::string) table_name, (std::string) primary_key, (const google::protobuf::Message*) proto_msg);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowExistMsg, (std::string) table_name, (std::string) primary_key, (bool) exist);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowUpdateMsg, (std::string) table_name, (std::string) primary_key, (const google::protobuf::Message*) proto_msg);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowDeleteMsg, (std::string) table_name, (std::string) primary_key, (const google::protobuf::Message*) proto_msg);


class DbService : public IService {
public:

    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        LOG_DEBUG("DbService Init");

        proto_mgr_.Init();

        return true;
    }

    virtual void OnExit() override {
        
    }

    MILLION_MSG_DISPATCH(DbService);

    MILLION_MSG_HANDLE(DbSqlInitMsg, msg) {
        sql_service_ = msg->sender();
        for (const auto& table_info : proto_mgr_.table_map()) {
            co_await Call<SqlCreateTableMsg>(sql_service_, table_info.second);
        }

        //Db::User user;
        //user.set_name("sb");
        //user.set_email("fake@qq.com");
        //user.set_phone_number("1234567890");
        //user.set_password_hash("AWDaoDWHGOAUGH");

        //co_await Call<SqlInsertMsg>(sql_service_, &user);

        //std::vector<bool> dirty_bits(user.GetDescriptor()->field_count());
        //auto res = co_await Call<SqlQueryMsg>(sql_service_, "1", &user, &dirty_bits, false);

        co_return;
    }

    MILLION_MSG_HANDLE(DbRowQueryMsg, msg) {
        const protobuf::Descriptor* desc = proto_mgr_.GetMsgDesc(msg->table_name);
        if (!desc) {
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

            auto proto_msg_opt = proto_mgr_.NewMessage(*desc);
            if (!proto_msg_opt) {
                co_return;
            }

            auto row = DbRow(std::move(*proto_msg_opt), std::vector<bool>(desc->field_count()));
            auto res_msg = co_await Call<CacheQueryMsg>(cache_service_, msg->primary_key, row.proto_msg.get(), &row.dirty_bits, false);
            if (!res_msg->success) {
                auto res_msg = co_await Call<SqlQueryMsg>(sql_service_, msg->primary_key, row.proto_msg.get(), &row.dirty_bits, false);
            }

            auto res = rows.emplace(std::move(msg->primary_key), std::move(row));
            assert(res.second);
            row_iter = res.first;
        } while (false);
        msg->proto_msg = row_iter->second.proto_msg.get();
        Reply(std::move(msg));
        co_return;
    }


private:
    DbProtoMgr proto_mgr_;
    struct DbRow {
        ProtoMsgUnique proto_msg;
        std::vector<bool> dirty_bits;
        // DbRowMeta meta;
    };
    using DbRows = std::unordered_map<std::string, DbRow>;
    std::unordered_map<const protobuf::Descriptor*, DbRows> tables_;

    ServiceHandle cache_service_;
    ServiceHandle sql_service_;
};

} // namespace db
} // namespace million