#include "user_session.h"

namespace million {
namespace gateway {

UserSession::UserSession(net::TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor)
    : TcpConnection(server, std::move(socket), executor) {}
UserSession::~UserSession() = default;

} // namespace gateway
} // namespace million
