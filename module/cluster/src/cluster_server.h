#pragma once

#include <million/net/tcp_server.h>

#include "node_session.h"

namespace million {
namespace cluster {

using NodeSessionShared = std::shared_ptr<NodeSession>;

class ClusterServer : public net::TcpServer {
public:
    using TcpServer = net::TcpServer;
    ClusterServer(IMillion* million)
        : TcpServer(million) {}

    virtual ::million::net::TcpConnectionShared MakeTcpConnectionShared(TcpServer* server, asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor) const override {
        return std::make_shared<NodeSession>(server, std::move(socket), std::move(executor));
    }

};

} // namespace AGENT
} // namespace million
