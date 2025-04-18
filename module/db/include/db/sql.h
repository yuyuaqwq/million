#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

constexpr const char* kSqlServiceName = "SqlService";
#define MILLION_SQL_SERVICE_NAME "SqlService"

MILLION_MSG_DEFINE(MILLION_DB_API, SqlTableInitReq, (const google::protobuf::Descriptor&) desc)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlTableInitResp, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, SqlQueryReq, (DBRow) db_row, (int) key_field_number, (ProtoFieldAny) key)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlQueryResp, (std::optional<DBRow>) db_row)

MILLION_MSG_DEFINE(MILLION_DB_API, SqlUpdateReq, (DBRow) db_row, (uint64_t) old_db_version)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlUpdateResp, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, SqlInsertReq, (DBRow) db_row, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlInsertResp, (bool) success)

} // namespace db
} // namespace million