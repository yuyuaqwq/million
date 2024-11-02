#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto_msg.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(SqlCreateTableMsg, (const google::protobuf::Descriptor*) desc)
MILLION_MSG_DEFINE(SqlQueryMsg, (std::string) primary_key, (google::protobuf::Message*) proto_msg, (std::vector<bool>*) dirty_bits, (bool) success)
MILLION_MSG_DEFINE(SqlUpdateMsg, (google::protobuf::Message*) proto_msg)
MILLION_MSG_DEFINE(SqlInsertMsg, (google::protobuf::Message*) proto_msg)

} // namespace db
} // namespace million