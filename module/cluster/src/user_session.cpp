#include "user_session.h"

#include <million/net/tcp_connection.h>

// #include "cs_proto_mgr.h"

namespace million {
namespace cluster {

UserSession::UserSession(net::TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor)
    : TcpConnection(server, std::move(socket), executor)
{
    header_.token = kInvaildToken;
}
UserSession::~UserSession() = default;

} // namespace gateway
} // namespace million
