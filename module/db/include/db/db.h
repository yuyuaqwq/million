#pragma once

#include <optional>
#include <vector>

#include <million/imillion.h>
#include <million/proto.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

namespace protobuf = google::protobuf;
class DB_CLASS_API DbProtoCodec : noncopyable{
public:
    DbProtoCodec(const protobuf::DescriptorPool& desc_pool, protobuf::DescriptorDatabase& desc_db, protobuf::MessageFactory& message_factory);

    // ��ʼ��
    void Init();

    const protobuf::FileDescriptor* RegisterProto(const std::string& proto_file_name);

    //const protobuf::Descriptor* GetMsgDesc(const std::string& table_name);

    std::optional<ProtoMsgUnique> NewMessage(const protobuf::Descriptor& desc);

    // const auto& table_map() const { return table_map_; }

private:
    const protobuf::DescriptorPool& desc_pool_;
    protobuf::DescriptorDatabase& desc_db_;
    protobuf::MessageFactory& message_factory_;
    // std::unordered_map<std::string, const protobuf::Descriptor*> table_map_;
};


MILLION_MSG_DEFINE(DB_CLASS_API, DbRegisterProtoCodecMsg, (nonnull_ptr<DbProtoCodec>) proto_codec);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRegisterProtoMsg, (std::string) proto_file_name, (bool) success);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowExistMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (bool) exist);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowGetMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (std::optional<million::db::DbRow>) db_row);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowSetMsg, (std::string) table_name, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowDeleteMsg, (std::string) table_name, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row);

} // namespace db
} // namespace million