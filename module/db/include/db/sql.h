#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(MILLION_DB_API, SqlTableInitMsg, (const google::protobuf::Descriptor&) desc)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlQueryMsg, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlUpdateMsg, (nonnull_ptr<DbRow>) db_row)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlInsertMsg, (nonnull_ptr<DbRow>) db_row)

} // namespace db
} // namespace million