#include "user_session.h"

namespace million {
namespace gateway {

UserSession::UserSession(net::TcpServer* server, asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor)
    : TcpConnection(server, std::move(socket), std::move(executor)) {}
UserSession::~UserSession() = default;

} // namespace gateway
} // namespace million
