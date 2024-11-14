#include "user_session.h"

namespace million {
namespace gateway {


UserSession::UserSession(CommProtoMgr<UserHeader>* proto_mgr, net::TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor)
    : proto_mgr_(proto_mgr)
    , TcpConnection(server, std::move(socket), executor)
{
    header_.token = kInvaildToken;
}
UserSession::~UserSession() = default;


bool UserSession::Send(const google::protobuf::Message& message) {
    auto buffer = proto_mgr_->EncodeMessage(header_, message);
    assert(buffer);
    if (!buffer) return false;
    TcpConnection::Send(std::move(*buffer));
    return true;
}


} // namespace gateway
} // namespace million
