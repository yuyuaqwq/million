#include <million/imillion.h>

#include <table/api.h>

namespace million {
namespace table {

MILLION_MSG_DEFINE(MILLION_TABLE_API, TableQueryMsg, (const google::protobuf::Descriptor&) table_desc, (std::optional<ProtoMsgShared>) table)
MILLION_MSG_DEFINE(MILLION_TABLE_API, TableUpdateMsg, (const google::protobuf::Descriptor&) table_desc)


} // namespace table
} // namespace million