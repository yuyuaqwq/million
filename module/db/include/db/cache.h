#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

    constexpr const char* kCacheServiceName = "CacheService";
#define MILLION_CACHE_SERVICE_NAME "CacheService"

MILLION_MSG_DEFINE(MILLION_DB_API, CacheGetReq,  (DBRow) db_row, (std::string_view) primary_key)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheGetResp, (std::optional<DBRow>) db_row)

MILLION_MSG_DEFINE(MILLION_DB_API, CacheSetReq, (DBRow) db_row, (uint64_t) old_db_version)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheSetResp, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, CacheDelReq, (DBRow) db_row)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheDelResp, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, CacheBatchSetReq, (std::vector<DBRow>) db_rows, (std::vector<uint64_t>) old_db_version_list)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheBatchSetResp, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, CacheGetBytesReq, (std::string) key)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheGetBytesResp, (std::optional<std::string>) value)

MILLION_MSG_DEFINE(MILLION_DB_API, CacheSetBytesReq, (std::string) key)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheSetBytesResp, (std::optional<std::string>) value)

} // namespace db
} // namespace million