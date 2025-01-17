#pragma once

#include <optional>
#include <vector>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/db_options.pb.h>
#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

using BatchId = uint64_t;
constexpr BatchId kBatchIdNull = 0;

MILLION_MSG_DEFINE(MILLION_DB_API, DbRowExistMsg, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (bool) exist);

MILLION_MSG_DEFINE_NONCOPYABLE(MILLION_DB_API, DbRowCreateMsg, (ProtoMsgUnique) row_msg);

MILLION_MSG_DEFINE(MILLION_DB_API, DbRowQueryMsg
    , (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key
    , (std::optional<DbRow>) db_row);

MILLION_MSG_DEFINE(MILLION_DB_API, DbRowUpdateMsg, (nonnull_ptr<DbRow>) db_row);

MILLION_MSG_DEFINE(MILLION_DB_API, DbRowDeleteMsg, (std::string) table_name, (std::string) primary_key);


MILLION_MSG_DEFINE(MILLION_DB_API, DbBatchBeginMsg, (BatchId) batch_id);

MILLION_MSG_DEFINE(MILLION_DB_API, DbBatchCommitMsg, (BatchId) batch_id);


} // namespace db
} // namespace million