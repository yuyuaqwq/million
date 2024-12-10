#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto.h>

#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

MILLION_MSG_DEFINE(DB_CLASS_API, DbRowExistMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (bool) exist);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowGetMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (DbRow) db_row);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowSetMsg, (std::string) table_name, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row);
MILLION_MSG_DEFINE(DB_CLASS_API, DbRowDeleteMsg, (std::string) table_name, (std::string) primary_key, (nonnull_ptr<DbRow>) db_row);

} // namespace db
} // namespace million