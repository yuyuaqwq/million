#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto_msg.h>

namespace million {
namespace db {

enum class CacheMsgType : uint32_t {
    kParseFromCache,
    kSerializeToCache,
    kCacheGet,
    kCacheSet,
};
using CacheMsgBase = MsgBaseT<CacheMsgType>;
MILLION_MSG_DEFINE(ParseFromCacheMsg, CacheMsgType::kParseFromCache, (google::protobuf::Message*) proto_msg, (std::vector<bool>*) dirty_bits, (bool) success)
MILLION_MSG_DEFINE(SerializeToCacheMsg, CacheMsgType::kSerializeToCache, (google::protobuf::Message*) proto_msg, (const std::vector<bool>*) dirty_bits)
MILLION_MSG_DEFINE(CacheGetMsg, CacheMsgType::kCacheGet, (std::string) key_value)
MILLION_MSG_DEFINE(CacheSetMsg, CacheMsgType::kCacheSet, (std::string) key, (std::string) value, (bool) success)

} // namespace db
} // namespace million