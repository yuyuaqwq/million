#pragma once

#include "net/tcp_server.h"

namespace million {
namespace gateway {

class ProtoMgr;
class GatewayServer : public net::TcpServer {
public:
    using TcpServer = net::TcpServer;
    GatewayServer(IMillion* million, ProtoMgr* proto_mgr)
        : proto_mgr_(proto_mgr)
        , TcpServer(million) {}

    virtual ::million::net::TcpConnectionShared MakeTcpConnectionShared(TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor) const override {
        return std::make_shared<UserSession>(proto_mgr_, server, std::move(socket), executor);
    }

private:
    ProtoMgr* proto_mgr_;
};

} // namespace gateway
} // namespace million
