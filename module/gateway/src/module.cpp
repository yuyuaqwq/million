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

namespace million {
namespace gateway {

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

enum GatewayMsgType {
    kConnection,
    kRecvPacket,

    kRegisterService,
    // kUnRegisterService,
    kRecvProtoMsg,
    kSendProtoMsg,
};
using GatewayMsgBase = MsgBaseT<GatewayMsgType>;
MILLION_MSG_DEFINE(ConnectionMsg, kConnection, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(RecvPacketMsg, kRecvPacket, (net::TcpConnection*) connection, (net::Packet) packet)

MILLION_MSG_DEFINE(RegisterServiceMsg, kRegisterService, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
// MILLION_MSG_DEFINE(UnRegisterServiceMsg, kUnRegisterService, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
MILLION_MSG_DEFINE(RecvProtoMsg, kRecvProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(SendProtoMsg, kSendProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)

class GatewayService : public IService {
public:
    using Base = IService;
    GatewayService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion, &proto_mgr_) { }

    virtual void OnInit() override {
        proto_mgr_.InitMsgMap();
        proto_mgr_.RegistrySubMsg(Cs::MSG_ID_USER, Cs::cs_sub_msg_id_user);

        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto connection) {
            Send<ConnectionMsg>(service_handle(), std::move(connection));
        });
        server_.set_on_msg([this](auto& connection, auto&& packet) {
            Send<RecvPacketMsg>(service_handle(), &connection, std::move(packet));
        });
        server_.Start(8001);
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

        auto session = static_cast<UserSession*>(msg->connection);
        auto handle = UserSessionHandle(session);

        if (session->header().token == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            session->header().token = header.token;

            // todo: 需要断开原先token指向的连接
        }

        // 没登录，只能分发给UserMsg
        if (session->header().token == kInvaildToken && msg_id != Cs::MSG_ID_USER) {
            co_return;
        }

        auto iter = register_services_.find(msg_id);
        if (iter != register_services_.end()) {
            auto& services = iter->second;
            for (auto& service : services) {
                Send<RecvProtoMsg>(service, handle, std::move(proto_msg));
            }
        }

        // auto token = token_generator_.Generate();

        co_return;
    }

    MILLION_MSG_HANDLE(RegisterServiceMsg, msg) {
        register_services_[msg->cs_msg_id].push_back(msg->service_handle);
        co_return;
    }

    //MILLION_MSG_HANDLE(UnRegisterServiceMsg, msg) {
    //    auto iter = register_services_.find(msg->cs_msg_id);
    //    if (iter == register_services_.end()) co_return;
    //    auto& services = iter->second;
    //    auto remove = std::remove(services.begin(), services.end(), msg->service_handle);
    //    services.erase(remove, services.end());
    //    Reply(msg->sender(), msg->session_id());
    //    co_return;
    //}

    MILLION_MSG_HANDLE(SendProtoMsg, msg) {
        msg->session_handle.user_session().Send(*msg->proto_msg);
        co_return;
    }

    MILLION_PROTO_MSG_DISPATCH();
    
    MILLION_PROTO_MSG_ID(MSG_ID_USER);

    MILLION_PROTO_MSG_HANDLE(SubMsgIdUser::SUB_MSG_ID_USER_LOGIN, UserRegisterReq, req) {
        
        std::cout << "login:" << req->user_name() << req->password() << std::endl;
        co_return;
    }

private:
    ProtoMgr proto_mgr_;
    GatewayServer server_;
    TokenGenerator token_generator_;
    std::unordered_map<Cs::MsgId, std::vector<ServiceHandle>> register_services_;
    std::list<UserSession> users_;
};

MILLION_FUNC_EXPORT bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<GatewayService>();

    return true;
}

} // namespace gateway
} // namespace million