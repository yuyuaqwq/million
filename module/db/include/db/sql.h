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

MILLION_MSG_DEFINE(MILLION_DB_API, SqlTableInitMsgReq, (const google::protobuf::Descriptor&) desc)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlTableInitMsgResp, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, SqlQueryMsgReq, (DBRow) db_row, (std::string) primary_key)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlQueryMsgResp, (std::optional<DBRow>) db_row)

MILLION_MSG_DEFINE(MILLION_DB_API, SqlUpdateMsgReq, (DBRow) db_row, (uint64_t) old_db_version)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlUpdateMsgResp, (bool) success)

MILLION_MSG_DEFINE(MILLION_DB_API, SqlInsertMsgReq, (DBRow) db_row, (bool) success)
MILLION_MSG_DEFINE(MILLION_DB_API, SqlInsertMsgResp, (bool) success)

} // namespace db
} // namespace million