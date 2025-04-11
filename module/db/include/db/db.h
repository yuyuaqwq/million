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

constexpr const char* kDbServiceName = "DbService";

using BatchId = uint64_t;
constexpr BatchId kBatchIdNull = 0;

MILLION_MSG_DEFINE(MILLION_DB_API, DBRowExistReq, (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key);
MILLION_MSG_DEFINE(MILLION_DB_API, DBRowExistResp, (bool) exist);

MILLION_MSG_DEFINE_NONCOPYABLE(MILLION_DB_API, DBRowCreateReq, (ProtoMsgUnique) row_msg);
MILLION_MSG_DEFINE(MILLION_DB_API, DBRowCreateResp, (bool) success);

MILLION_MSG_DEFINE(MILLION_DB_API, DBRowQueryReq
    , (const google::protobuf::Descriptor&) table_desc, (std::string) primary_key, (bool) tick_write_back);
MILLION_MSG_DEFINE(MILLION_DB_API, DBRowQueryResp, (std::optional<DBRow>) db_row);

/*(bool) update_to_cache*/
MILLION_MSG_DEFINE(MILLION_DB_API, DBRowUpdateReq, (DBRow) db_row, (uint64_t) old_db_version);
MILLION_MSG_DEFINE_EMPTY(MILLION_DB_API, DBRowUpdateResp);

// MILLION_MSG_DEFINE(MILLION_DB_API, DbRowBatchUpdateReq, (std::vector<nonnull_ptr<DbRow>>) db_rows);

MILLION_MSG_DEFINE(MILLION_DB_API, DBRowDeleteReq, (std::string) table_name, (std::string) primary_key);
MILLION_MSG_DEFINE(MILLION_DB_API, DBRowDeleteResp, (bool) success);

MILLION_MSG_DEFINE(MILLION_DB_API, DBBatchBeginReq, (BatchId) batch_id);
MILLION_MSG_DEFINE(MILLION_DB_API, DBBatchBeginResp, (bool) success);

MILLION_MSG_DEFINE(MILLION_DB_API, DBBatchCommitReq, (BatchId) batch_id);
MILLION_MSG_DEFINE(MILLION_DB_API, DBBatchCommitResp, (bool) success);

} // namespace db
} // namespace million