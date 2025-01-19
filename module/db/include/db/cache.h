#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(MILLION_DB_API, CacheGetMsg, (std::string_view) primary_key, (nonnull_ptr<DbRow>) db_row, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheSetMsg, (nonnull_ptr<DbRow>) db_row, (uint64_t) old_db_version, (bool) success)
// MILLION_MSG_DEFINE(MILLION_DB_API, CacheBatchSetMsg, (std::vector<nonnull_ptr<DbRow>>) db_rows, (std::vector<uint64_t>) old_db_version_list, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, CacheGetBytesMsg, (std::string) key_value)
MILLION_MSG_DEFINE(MILLION_DB_API, CacheSetBytesMsg, (std::string) key, (std::string) value, (bool) success)

} // namespace db
} // namespace million