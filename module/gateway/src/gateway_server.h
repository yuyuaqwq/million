#pragma once

#include <million/net/tcp_server.h>

#include "user_session.h"

namespace million {
namespace gateway {

class GatewayServer : public net::TcpServer {
public:
    using TcpServer = net::TcpServer;
    GatewayServer(IMillion* million, CommProtoMgr<UserHeader>* proto_mgr)
        : proto_mgr_(proto_mgr)
        , TcpServer(million) {}

    virtual ::million::net::TcpConnectionShared MakeTcpConnectionShared(TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor) const override {
        return std::make_shared<UserSession>(proto_mgr_, server, std::move(socket), executor);
    }

private:
    CommProtoMgr<UserHeader>* proto_mgr_;
};

} // namespace gateway
} // namespace million
