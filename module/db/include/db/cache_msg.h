#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto_msg.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(CacheQueryMsg, (std::string_view) primary_key, (google::protobuf::Message*) proto_msg, (std::vector<bool>*) dirty_bits, (bool) success)
MILLION_MSG_DEFINE(CacheUpdateMsg, (google::protobuf::Message*) proto_msg, (const std::vector<bool>*) dirty_bits)
MILLION_MSG_DEFINE(CacheGetMsg, (std::string) key_value)
MILLION_MSG_DEFINE(CacheSetMsg, (std::string) key, (std::string) value, (bool) success)

} // namespace db
} // namespace million