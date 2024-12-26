#pragma once

#include <million/net/tcp_server.h>

#include "user_session.h"

namespace million {
namespace gateway {

using UserSessionShared = std::shared_ptr<UserSession>;

class GatewayServer : public net::TcpServer {
public:
    using TcpServer = net::TcpServer;
    GatewayServer(IMillion* million)
        : TcpServer(million) {}

    virtual ::million::net::TcpConnectionShared MakeTcpConnectionShared(TcpServer* server, asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor) const override {
        return std::make_shared<UserSession>(server, std::move(socket), std::move(executor));
    }

};

} // namespace gateway
} // namespace million
