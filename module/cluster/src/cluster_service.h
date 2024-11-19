#pragma once

#include <million/iservice.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

MILLION_MSG_DEFINE(, ClusterTcpConnectionMsg, (net::TcpConnectionShared)connection)
MILLION_MSG_DEFINE(, ClusterTcpRecvPacketMsg, (net::TcpConnectionShared)connection, (net::Packet)packet)

class ClusterService : public IService {
public:
    using Base = IService;
    ClusterService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) {
            Send<ClusterTcpConnectionMsg>(service_handle(), connection);
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) {
            Send<ClusterTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
        });
        server_.Start(8002);

        return true;
    }

    MILLION_MSG_DISPATCH(ClusterService);

private:
    ClusterServer server_;
};

} // namespace gateway
} // namespace million
