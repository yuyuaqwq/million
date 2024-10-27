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

#include <protogen/db/db_options.pb.h>

namespace million {
namespace db {

namespace protobuf = google::protobuf;
class DbProtoMgr : noncopyable{
public:
    // 初始化消息映射
    void InitMsgMap() {
        const protobuf::DescriptorPool* pool = protobuf::DescriptorPool::generated_pool();
        protobuf::DescriptorDatabase* db = pool->internal_generated_database();
        if (db == nullptr) {
            return;
        }

        std::vector<std::string> file_names;
        db->FindAllFileNames(&file_names);   // 遍历得到所有proto文件名
        for (const std::string& filename : file_names) {
            const protobuf::FileDescriptor* file_descriptor = pool->FindFileByName(filename);
            if (file_descriptor == nullptr) continue;

            // 获取该文件的options，确认是否设置了db
            const auto& file_options = file_descriptor->options();
            // 检查 db 是否被设置
            if (!file_options.HasExtension(Db::db)) {
                continue;
            }
            const auto& db_options = file_options.GetExtension(Db::db);

            int message_count = file_descriptor->message_type_count();
            for (int i = 0; i < message_count; i++) {
                const protobuf::Descriptor* descriptor = file_descriptor->message_type(i);
                if (!descriptor) continue;
                const auto& msg_options = descriptor->options();
                if (!msg_options.HasExtension(Db::table)) {
                    continue;
                }
                const auto& table_options = msg_options.GetExtension(Db::table);
                const auto& table_name = table_options.name();
                table_map_.emplace(table_name, descriptor);
            }
        }
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



private:
    std::unordered_map<std::string, const protobuf::Descriptor*> table_map_;
};

enum class DbMsgType : uint32_t {
    kDbQuery,
};
using DbMsgBase = MsgBaseT<DbMsgType>;

MILLION_MSG_DEFINE(DbQueryMsg, DbMsgType::kDbQuery, (std::string) table_name, (std::string) key, (google::protobuf::Message*) proto_msg);

class DbService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual void OnInit() override {
        proto_mgr_.InitMsgMap();
    }

    virtual Task OnMsg(MsgUnique msg) override {

        co_return;
    }

    virtual void OnExit() override {
        
    }

    MILLION_MSG_DISPATCH(DbService, DbMsgBase);

    MILLION_MSG_HANDLE(DbQueryMsg, msg) {
        const protobuf::Descriptor* desc = proto_mgr_.GetMsgDesc(msg->table_name);
        if (!desc) {
            co_return;
        }

        auto rows_iter = tables_.find(desc);
        if (rows_iter == tables_.end()) {
            auto res = tables_.emplace(desc, Rows());
            assert(res.second);
            rows_iter = res.first;
        }
        auto& rows = rows_iter->second;

        do {
            auto row_iter = rows.find(msg->key);
            if (row_iter != rows.end()) {
                msg->proto_msg = row_iter->second.first.get();
                break;
            }
            auto proto_msg_opt = proto_mgr_.NewMessage(*desc);
            if (!proto_msg_opt) {
                co_return;
            }

            auto row = Row(std::move(*proto_msg_opt), std::vector<bool>(desc->field_count()));
            auto res_msg = co_await Call<ParseFromCacheMsg>(cache_service_, row.first.get(), &row.second, false);
            if (!res_msg->success) {
                // 不成功还要走一次SQL，还不成功就回包

                if (!res_msg->success) {
                    msg->proto_msg = nullptr;
                    break;
                }
            }
            auto res = rows.emplace(std::move(msg->key), std::move(row));
            assert(res.second);
            row_iter = res.first;
        } while (false);
        Reply(std::move(msg));
        co_return;
    }

    //void Query(std::string table_name, std::string key) {
    //    
    //}

private:
    DbProtoMgr proto_mgr_;

    using Row = std::pair<ProtoMsgUnique, std::vector<bool>>;
    using Rows = std::unordered_map<std::string, Row>;
    std::unordered_map<const protobuf::Descriptor*, Rows> tables_;

    ServiceHandle cache_service_;
};

} // namespace db
} // namespace million