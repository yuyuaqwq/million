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

MILLION_MSG_DEFINE(MILLION_DB_API, CacheGetMsg, (std::string_view) primary_key, (DbRow*) db_row, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheSetMsg, (const DbRow&) db_row, (uint64_t) old_db_version, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheDelMsg, (const DbRow&) db_row, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheBatchSetMsg, (std::vector<std::reference_wrapper<const DbRow>>) db_rows, (std::vector<uint64_t>) old_db_version_list, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, CacheGetBytesMsg, (std::string) key_value)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheSetBytesMsg, (std::string) key, (std::string) value, (bool) success)

} // namespace db
} // namespace million