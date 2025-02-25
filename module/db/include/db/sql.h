#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(MILLION_DB_API, SqlTableInitMsg, (const google::protobuf::Descriptor&) desc)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlQueryMsg, (std::string) primary_key, (DbRow*) db_row, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlUpdateMsg, (const DbRow&) db_row, (uint64_t) old_db_version, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlInsertMsg, (const DbRow&) db_row, (bool) success)

} // namespace db
} // namespace million