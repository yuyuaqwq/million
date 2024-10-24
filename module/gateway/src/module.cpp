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
        proto_mgr_.InitMsgMap();
        proto_mgr_.RegistrySubMsg(Cs::MSG_ID_USER, Cs::cs_sub_msg_id_user);

        /*Cs::UserLoginReq req;
        req.set_user_name("666");
        req.set_password("sbsb");
        ProtoAdditionalHeader header;
        auto buffer = proto_mgr_.EncodeMessage(header, req);
        Send<RecvPacketMsg>(service_handle(), nullptr, std::move(*buffer));*/

        // io线程回调，发给work线程处理
        tcp_server_.set_on_connection([this](auto connection) {
            Send<ConnectionMsg>(service_handle(), std::move(connection));
        });
        tcp_server_.set_on_msg([this](auto& connection, auto&& packet) {
            Send<RecvPacketMsg>(service_handle(), &connection, std::move(packet));
        });
        tcp_server_.Start(8001);
    }

    MILLION_MSG_DISPATCH(GatewayService, GatewayMsgBase);

    MILLION_MSG_HANDLE(ConnectionMsg, msg) {
        co_return;
    }

    MILLION_MSG_HANDLE(RecvPacketMsg, msg) {
        co_await OnProtoMsg(msg->packet);

        // 在登录后才创建UserSession
        auto token = token_generator_.Generate();

        co_return;
    }

    MILLION_PROTO_MSG_DISPATCH();
    
    MILLION_PROTO_MSG_ID(MSG_ID_USER);

    /*MILLION_PROTO_MSG_HANDLE(SubMsgIdUser::SUB_MSG_ID_USER_LOGIN, UserRegisterReq, req) {
        std::cout << "login:" << req->user_name() << req->password() << std::endl;
        co_return;
    }*/
    ::million::Task _MILLION_PROTO_MSG_HANDLE_UserRegisterReq_I(::million::ProtoMsgUnique MILLION_PROTO_MSG_) {
        auto msg = ::std::unique_ptr<::Cs::UserRegisterReq>(static_cast<::Cs::UserRegisterReq*>(MILLION_PROTO_MSG_.release()));
        co_await _MILLION_PROTO_MSG_HANDLE_UserRegisterReq_II(std::move(msg));
        co_return;
    } 
    const bool MILLION_PROTO_MSG_HANDLE_REGISTER_UserRegisterReq = [this] { _MILLION_PROTO_MSG_HANDLE_MAP_.insert(::std::make_pair(::million::ProtoMgr::CalcKey(_MILLION_PROTO_MSG_HANDLE_CURRENT_MSG_ID_, Cs::SubMsgIdUser::SUB_MSG_ID_USER_LOGIN), &_MILLION_SERVICE_TYPE_::_MILLION_PROTO_MSG_HANDLE_UserRegisterReq_I)); return true; }(); ::million::Task _MILLION_PROTO_MSG_HANDLE_UserRegisterReq_II(::std::unique_ptr<::Cs::UserRegisterReq> req) {
        std::cout << "login:" << req->user_name() << req->password() << std::endl;
        co_return;
    }

private:
    net::TcpServer tcp_server_;
    ProtoMgr proto_mgr_;
    TokenGenerator token_generator_;
    std::list<UserSession> users_;
};

MILLION_FUNC_EXPORT bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<GatewayService>();

    return true;
}

} // namespace million