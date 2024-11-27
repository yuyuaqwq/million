#include "node_session.h"

namespace million {
namespace cluster {


NodeSession::NodeSession(net::TcpServer* server, asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor)
    : TcpConnection(server, std::move(socket), std::move(executor))
{

}
NodeSession::~NodeSession() = default;




} // namespace gateway
} // namespace million
