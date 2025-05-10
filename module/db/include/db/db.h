#pragma once

#include <optional>
#include <vector>
#include <any>

#include <million/imillion.h>

#include <db/db_options.pb.h>
#include <db/api.h>
#include <db/db_row.h>

namespace million {
namespace db {

constexpr const char* kDBServiceName = "DBService";

using BatchId = uint64_t;
constexpr BatchId kBatchIdNull = 0;

MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowExistReq, (const google::protobuf::Descriptor&) table_desc, (int32_t) key_field_number, (ProtoFieldAny) key);
MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowExistResp, (bool) exist);

MILLION_MESSAGE_DEFINE_NONCOPYABLE(MILLION_DB_API, DBRowCreateReq, (ProtoMessageUnique) row_msg);
MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowCreateResp, (bool) success);

MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowQueryReq
    , (const google::protobuf::Descriptor&) table_desc, (int32_t) key_field_number, (ProtoFieldAny) key);
MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowQueryResp, (std::optional<const DBRow>) db_row);

MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowLoadReq
    , (const google::protobuf::Descriptor&) table_desc, (int32_t) key_field_number, (ProtoFieldAny) key);
MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowLoadResp, (std::optional<DBRow>) db_row);

/*(bool) update_to_cache*/
MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowUpdateReq, (DBRow) db_row, (uint64_t) old_db_version);
MILLION_MESSAGE_DEFINE_EMPTY(MILLION_DB_API, DBRowUpdateResp);

// MILLION_MESSAGE_DEFINE(MILLION_DB_API, DbRowBatchUpdateReq, (std::vector<nonnull_ptr<DbRow>>) db_rows);

MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowDeleteReq, (std::string) table_name, (int32_t) key_field_number, (ProtoFieldAny) key);
MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBRowDeleteResp, (bool) success);

MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBBatchBeginReq, (BatchId) batch_id);
MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBBatchBeginResp, (bool) success);

MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBBatchCommitReq, (BatchId) batch_id);
MILLION_MESSAGE_DEFINE(MILLION_DB_API, DBBatchCommitResp, (bool) success);

} // namespace db
} // namespace million