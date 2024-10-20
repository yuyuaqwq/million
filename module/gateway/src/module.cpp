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

#include "token.h"
#include "proto_mgr.h"
#include "net/tcp_server.h"
#include "net/tcp_connection.h"

namespace million {

class UserSession {
public:
    UserSession(ProtoMgr* proto_mgr, net::TcpConnectionShared&& connection, const Token& token)
        : proto_mgr_(proto_mgr)
        , connection_(std::move(connection))
    {
        header_.token = token;
    }

    bool Send(const protobuf::Message& message) {
        auto buffer = proto_mgr_->EncodeMessage(header_, message);
        assert(buffer);
        if (!buffer) return false;
        connection_->Send(std::move(*buffer));
        return true;
    }

private:
    ProtoMgr* proto_mgr_;
    net::TcpConnectionShared connection_;
    ProtoAdditionalHeader header_;
};

enum GatewayMsgType {
    kConnection,
    kRecvPacket,
};
using GatewayMsgBase = MsgBaseT<GatewayMsgType>;
MILLION_MSG_DEFINE(ConnectionMsg, kConnection, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(RecvPacketMsg, kRecvPacket, (net::TcpConnection*) connection, (net::Packet) packet)

class GatewayService : public IService {
public:
    using Base = IService;
    GatewayService(IMillion* imillion)
        : Base(imillion)
        , tcp_server_(imillion) { }

    virtual void OnInit() override {
        proto_mgr_.Init();
        proto_mgr_.Registry(Cs::MSG_ID_USER, Cs::cs_sub_msg_id_user);

        // io线程回调，发给work线程处理
        //tcp_server_.set_on_connection([this](auto connection) {
        //    Send<ConnectionMsg>(service_handle(), std::move(connection));
        //});
        tcp_server_.set_on_msg([this](auto& connection, auto&& packet) {
            Send<RecvPacketMsg>(service_handle(), &connection, std::move(packet));
        });
        tcp_server_.Start(8001);
    }

    virtual Task OnMsg(MsgUnique msg) override {
        // 服务是单线程处理的

        MILLION_MSG_DISPATCH(GatewayMsgBase, std::move(msg));

        co_return;
    }

    MILLION_MSG_HANDLE_INIT(GatewayMsgBase);

    MILLION_MSG_HANDLE_BEGIN(ConnectionMsg, msg) {
        // 新连接来了，不过没必要处理
        auto token = token_generator_.Generate();

        // 还需要到redis中查询token状态
        users_.emplace_back(&proto_mgr_, std::move(msg->connection), token);

        co_return;
    }
    MILLION_MSG_HANDLE_END(ConnectionMsg);


    MILLION_MSG_HANDLE_BEGIN(RecvPacketMsg, msg) {
        // 在登录后才创建
        ProtoAdditionalHeader header;
        auto res = proto_mgr_.DecodeMessage(msg->packet, &header);
        if (!res) co_return;
        auto& [proto_msg, msg_id, sub_msg_id] = *res;

        co_return;
    }
    MILLION_MSG_HANDLE_END(RecvPacketMsg);


private:
    net::TcpServer tcp_server_;
    ProtoMgr proto_mgr_;
    TokenGenerator token_generator_;
    std::list<UserSession> users_;
};

MILLION_FUNC_EXPORT bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->config();
    auto handle = imillion->NewService<GatewayService>();

    return true;
}

} // namespace million