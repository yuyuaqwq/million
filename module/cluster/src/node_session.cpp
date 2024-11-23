#include "node_session.h"

namespace million {
namespace cluster {


NodeSession::NodeSession(net::TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor)
    : TcpConnection(server, std::move(socket), executor)
{

}
NodeSession::~NodeSession() = default;




} // namespace gateway
} // namespace million
