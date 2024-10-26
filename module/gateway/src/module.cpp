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

#include <gateway/user_session_handle.h>

#include "token.h"
#include "proto_mgr.h"
#include "net/tcp_server.h"
#include "net/tcp_connection.h"

namespace million {
namespace gateway {

enum GatewayMsgType {
    kConnection,
    kRecvPacket,

    kRegisterService,
    kUnRegisterService,
    kRecvProtoMsg,
    kSendProtoMsg,
};
using GatewayMsgBase = MsgBaseT<GatewayMsgType>;
MILLION_MSG_DEFINE(ConnectionMsg, kConnection, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(RecvPacketMsg, kRecvPacket, (net::TcpConnection*) connection, (net::Packet) packet)

MILLION_MSG_DEFINE(RegisterServiceMsg, kRegisterService, (ServiceHandle) service_handle)
MILLION_MSG_DEFINE(UnRegisterServiceMsg, kUnRegisterService, (ServiceHandle) service_handle)
MILLION_MSG_DEFINE(RecvProtoMsg, kRecvProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(SendProtoMsg, kSendProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)

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
        
        UserHeader header;
        auto res = proto_mgr_.DecodeMessage(msg->packet, &header);
        if (!res) co_return;
        auto&& [proto_msg, msg_id, sub_msg_id] = *res;

        // 如果没登录，分发给登录，登录了，分发给register_services_
        if () {
            co_await OnProtoMsg(, msg_id, sub_msg_id, std::move(proto_msg));
        }
        else {
            for (auto& service : register_services_) {
                Send<RecvProtoMsg>(service, , std::move(proto_msg));
            }
        }
       
        // 在登录后才创建UserSession
        auto token = token_generator_.Generate();

        co_return;
    }

    MILLION_MSG_HANDLE(RegisterServiceMsg, msg) {
        register_services_.push_back(msg->service_handle);
    }

    MILLION_MSG_HANDLE(UnRegisterServiceMsg, msg) {
        auto remove = std::remove(register_services_.begin(), register_services_.end(), msg->service_handle);
        register_services_.erase(remove, register_services_.end());
    }

    MILLION_MSG_HANDLE(SendProtoMsg, msg) {
        msg->session_handle.user_session().Send(*msg->proto_msg);
    }

    MILLION_PROTO_MSG_DISPATCH();
    
    MILLION_PROTO_MSG_ID(MSG_ID_USER);

    MILLION_PROTO_MSG_HANDLE(SubMsgIdUser::SUB_MSG_ID_USER_LOGIN, UserRegisterReq, req) {
        std::cout << "login:" << req->user_name() << req->password() << std::endl;
        co_return;
    }



private:
    net::TcpServer tcp_server_;
    ProtoMgr proto_mgr_;
    TokenGenerator token_generator_;
    std::vector<ServiceHandle> register_services_;
    std::list<UserSession> users_;
};

MILLION_FUNC_EXPORT bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<GatewayService>();

    return true;
}

} // namespace gateway
} // namespace million