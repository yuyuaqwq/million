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
using CacheMsgBase = million::MsgBaseT<CacheMsgType>;
MILLION_MSG_DEFINE(ParseFromCacheMsg, CacheMsgType::kParseFromCache, (million::ProtoMsgUnique) proto_msg, (bool) success)
MILLION_MSG_DEFINE(SerializeToCacheMsg, CacheMsgType::kSerializeToCache, (million::ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(CacheGetMsg, CacheMsgType::kCacheGet, (std::string) key_value)
MILLION_MSG_DEFINE(CacheSetMsg, CacheMsgType::kCacheSet, (std::string) key, (std::string) value, (bool) success)

} // namespace db
} // namespace million