#include <million/imillion.h>
#include <million/proto_msg.h>

enum CacheMsgType {
    kParseFromCache,
    kSerializeToCache,
    kCacheGet,
    kCacheSet,
};
using CacheMsgBase = million::MsgBaseT<CacheMsgType>;
MILLION_MSG_DEFINE(ParseFromCacheMsg, kParseFromCache, (million::ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(SerializeToCacheMsg, kSerializeToCache, (million::ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(CacheGetReqMsg, kCacheGet, (std::string) key)
MILLION_MSG_DEFINE(CacheGetResMsg, kCacheGet, (std::string) value)
MILLION_MSG_DEFINE(CacheSetReqMsg, kCacheSet, (std::string) key, (std::string) value)
MILLION_MSG_DEFINE(CacheSetResMsg, kCacheSet, (bool) success)