#include <atomic>
#include <optional>
#include <mutex>
#include <queue>
#include <sstream>
#include <format>
#include <iomanip>

#include <yaml-cpp/yaml.h>

#include <protogen/cs/cs_user.pb.h>

#include <million/imillion.h>
#include <million/imsg.h>
#include <million/proto_msg.h>

#include <db/cache.h>

#include "token_generator.h"
#include "proto_mgr.h"
#include "net/tcp_server.h"

namespace million {

class NetSession {

};

enum GatewayMsgType {
    kConnection,
    kRecvPacket,
};
using GatewayMsgBase = million::MsgBaseT<GatewayMsgType>;
MILLION_MSG_DEFINE(ConnectionMsg, kConnection, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(RecvPacketMsg, kRecvPacket, (net::TcpConnection*) connection, (net::Packet) packet)

class GatewayService : public million::IService {
public:
    using Base = million::IService;
    GatewayService(million::IMillion* imillion)
        : Base(imillion)
        , tcp_server_(imillion) { }

    virtual void OnInit() override {
        proto_mgr_.Init();
        proto_mgr_.Registry(Cs::MSG_ID_USER, Cs::cs_sub_msg_id_user);

        // io线程回调，发给work线程处理
        tcp_server_.set_on_connection([this](auto connection) {
            Send<ConnectionMsg>(service_handle(), std::move(connection));
        });
        tcp_server_.set_on_msg([this](auto& connection, auto&& packet) {
            Send<RecvPacketMsg>(service_handle(), &connection, std::move(packet));
        });
        tcp_server_.Start(8001);
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        // 服务是单线程处理的

        MILLION_HANDLE_MSG_BEGIN(std::move(msg), GatewayMsgBase)

        MILLION_HANDLE_MSG(msg, ConnectionMsg, {
            //// 新连接来了，创建Session -> token
            //auto token = token_generator_.Generate();
        })

        MILLION_HANDLE_MSG(msg, RecvPacketMsg, {
            //// 开始处理
            //ProtoAdditionalHeader additional;
            //auto msg = proto_manager_.DecodeMessage(packet, &additional);
        })

        MILLION_HANDLE_MSG_END()

        co_return;
    }

private:
    million::net::TcpServer tcp_server_;
    million::ProtoMgr proto_mgr_;
    million::TokenGenerator token_generator_;
};


MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    auto& config = imillion->config();
    auto handle = imillion->NewService<GatewayService>();

    return true;
}

} // namespace million