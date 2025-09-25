#pragma once

#include <vector>

#include <million/imillion.h>

#include <million/db/api.h>
#include <million/db/db_row.h>

namespace million {
namespace db {

MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheGetReq,  (DBRow) db_row, (std::string_view) primary_key)
MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheGetResp, (std::optional<DBRow>) db_row)

MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheSetReq, (DBRow) db_row, (uint64_t) old_db_version)
MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheSetResp, (bool) success)

MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheDelReq, (DBRow) db_row)
MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheDelResp, (bool) success)

MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheBatchSetReq, (std::vector<DBRow>) db_rows, (std::vector<uint64_t>) old_db_version_list)
MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheBatchSetResp, (bool) success)

MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheGetBytesReq, (std::string) key)
MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheGetBytesResp, (std::optional<std::string>) value)

MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheSetBytesReq, (std::string) key)
MILLION_MESSAGE_DEFINE(MILLION_DB_API, CacheSetBytesResp, (std::optional<std::string>) value)

} // namespace db
} // namespace million