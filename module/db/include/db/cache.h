#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(DB_CLASS_API, CacheGetMsg, (std::string_view) primary_key, (DbRow*) db_row, (bool) success)
MILLION_MSG_DEFINE(DB_CLASS_API, CacheSetMsg, (DbRow*) db_row)
MILLION_MSG_DEFINE(DB_CLASS_API, CacheGetBytesMsg, (std::string) key_value)
MILLION_MSG_DEFINE(DB_CLASS_API, CacheSetBytesMsg, (std::string) key, (std::string) value, (bool) success)

} // namespace db
} // namespace million