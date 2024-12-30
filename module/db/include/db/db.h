#pragma once

#include <optional>
#include <vector>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

namespace protobuf = google::protobuf;
class MILLION_DB_API DbProtoCodec : noncopyable{
public:
    DbProtoCodec(const ProtoMgr& proto_mgr);

    // 初始化
    void Init();

    const protobuf::FileDescriptor* RegisterProto(const std::string& proto_file_name);

    //const protobuf::Descriptor* GetMsgDesc(const std::string& table_name);

    std::optional<ProtoMsgUnique> NewMessage(const protobuf::Descriptor& desc);

    // const auto& table_map() const { return table_map_; }

private:
    const ProtoMgr& proto_mgr_;
    // std::unordered_map<std::string, const protobuf::Descriptor*> table_map_;
};


MILLION_MSG_DEFINE(MILLION_DB_API, DbRegisterProtoCodecMsg, (nonnull_ptr<DbProtoCodec>) proto_codec);
MILLION_MSG_DEFINE(MILLION_DB_API, DbRegisterProtoMsg, (std::string) proto_file_name, (bool) success);
MILLION_MSG_DEFINE(MILLION_DB_API, DbRowExistMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (bool) exist);
MILLION_MSG_DEFINE(MILLION_DB_API, DbRowGetMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (std::optional<DbRow>) db_row);
MILLION_MSG_DEFINE(MILLION_DB_API, DbRowSetMsg, (std::string) table_name, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row);
MILLION_MSG_DEFINE(MILLION_DB_API, DbRowDeleteMsg, (std::string) table_name, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row);

} // namespace db
} // namespace million