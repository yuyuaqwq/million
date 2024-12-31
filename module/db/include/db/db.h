#pragma once

#include <optional>
#include <vector>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(MILLION_DB_API, DbRowExistMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (bool) exist);
MILLION_MSG_DEFINE(MILLION_DB_API, DbRowGetMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (std::optional<DbRow>) db_row);
MILLION_MSG_DEFINE(MILLION_DB_API, DbRowSetMsg, (std::string) table_name, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row);
MILLION_MSG_DEFINE(MILLION_DB_API, DbRowDeleteMsg, (std::string) table_name, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row);

} // namespace db
} // namespace million