#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto_msg.h>

namespace million {
namespace db {

enum class SqlMsgType : uint32_t {
    kSqlCreateTable,
    kParseFromSql,
    kSerializeToSql,
};
using SqlMsgBase = MsgBaseT<SqlMsgType>;
MILLION_MSG_DEFINE(SqlCreateTableMsg, SqlMsgType::kSqlCreateTable, (const google::protobuf::Descriptor*) desc)
MILLION_MSG_DEFINE(ParseFromSqlMsg, SqlMsgType::kParseFromSql, (std::string_view) primary_key, (google::protobuf::Message*) proto_msg, (std::vector<bool>*) dirty_bits, (bool) success)
//MILLION_MSG_DEFINE(SerializeToSqlMsg, SqlMsgType::kSerializeToSql, (std::string) key_value)

} // namespace db
} // namespace million