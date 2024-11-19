#pragma once

#include <million/net/tcp_server.h>

#include "user_session.h"

namespace million {
namespace cluster {

class ClusterServer : public net::TcpServer {
public:
    using TcpServer = net::TcpServer;
    ClusterServer(IMillion* million)
        : TcpServer(million) {}

    virtual ::million::net::TcpConnectionShared MakeTcpConnectionShared(TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor) const override {
        return std::make_shared<UserSession>(server, std::move(socket), executor);
    }

};

} // namespace gateway
} // namespace million
