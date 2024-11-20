#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto_msg.h>

#include <db/api.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(DB_CLASS_API, SqlCreateTableMsg, (const google::protobuf::Descriptor*) desc)
MILLION_MSG_DEFINE(DB_CLASS_API, SqlQueryMsg, (std::string) primary_key, (google::protobuf::Message*) proto_msg, (std::vector<bool>*) dirty_bits, (bool) success)
MILLION_MSG_DEFINE(DB_CLASS_API, SqlUpdateMsg, (const google::protobuf::Message*) proto_msg)
MILLION_MSG_DEFINE(DB_CLASS_API, SqlInsertMsg, (const google::protobuf::Message*) proto_msg)

} // namespace db
} // namespace million