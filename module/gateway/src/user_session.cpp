#include "user_session.h"

#include "proto_mgr.h"
#include "net/tcp_connection.h"

namespace million {
namespace gateway {


UserSession::UserSession(ProtoMgr* proto_mgr, net::TcpConnectionShared&& connection, const Token& token)
    : proto_mgr_(proto_mgr)
    , connection_(std::move(connection))
{
    header_.token = token;
}
UserSession::~UserSession() = default;


bool UserSession::Send(const protobuf::Message& message) {
    auto buffer = proto_mgr_->EncodeMessage(header_, message);
    assert(buffer);
    if (!buffer) return false;
    connection_->Send(std::move(*buffer));
    return true;
}


} // namespace gateway
} // namespace million
