#pragma once

#include <gateway/getaway_msg.h>

#include <million/iservice.h>
#include <million/proto_msg.h>

#include <gateway/api.h>

#include "gateway_server.h"

#include <protogen/cs/cs_msgid.pb.h>
#include <protogen/cs/cs_test.pb.h>

#define MILLION_CS_PROTO_MSG_DISPATCH() MILLION_PROTO_MSG_DISPATCH(Cs, ::million::gateway::UserSessionHandle)
#define MILLION_CS_PROTO_MSG_ID(MSG_ID_) MILLION_PROTO_MSG_ID(Cs, MSG_ID_)
#define MILLION_CS_PROTO_MSG_HANDLE(NAMESPACE_, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_) MILLION_PROTO_MSG_HANDLE(Cs::##NAMESPACE_, ::million::gateway::UserSessionHandle, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_)

namespace million {
namespace gateway {

namespace protobuf = google::protobuf; 

MILLION_MSG_DEFINE(, ConnectionMsg, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(, RecvPacketMsg, (net::TcpConnection*) connection, (net::Packet) packet)

class GatewayService : public IService {
public:
    using Base = IService;
    GatewayService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion, &proto_mgr_) { }

    virtual void OnInit() override {
        proto_mgr_.Init();
        const protobuf::DescriptorPool* pool = protobuf::DescriptorPool::generated_pool();
        protobuf::DescriptorDatabase* db = pool->internal_generated_database();
        auto cs_user = pool->FindFileByName("cs_test.proto");
        if (cs_user) {
            proto_mgr_.RegisterProto(*cs_user, Cs::cs_msg_id, Cs::Test::cs_sub_msg_id_user);
        }
        
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto connection) {
            Send<ConnectionMsg>(service_handle(), std::move(connection));
        });
        server_.set_on_msg([this](auto& connection, auto&& packet) {
            Send<RecvPacketMsg>(service_handle(), &connection, std::move(packet));
        });
        server_.Start(8001);
    }

    MILLION_MSG_DISPATCH(GatewayService);

    MILLION_MSG_HANDLE(ConnectionMsg, msg) {
        co_return;
    }

    MILLION_MSG_HANDLE(RecvPacketMsg, msg) {
        UserHeader header;
        auto res = proto_mgr_.DecodeMessage(msg->packet, &header);
        if (!res) co_return;
        auto&& [proto_msg, msg_id_u32, sub_msg_id] = *res;

        auto session = static_cast<UserSession*>(msg->connection);
        auto handle = UserSessionHandle(session);

        if (session->header().token == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            session->header().token = header.token;

            // todo: 需要断开原先token指向的连接
        }
        auto msg_id = static_cast<Cs::MsgId>(msg_id_u32);
        co_await ProtoMsgDispatch(handle, msg_id, sub_msg_id, std::move(proto_msg));

        // 没登录，只能分发给UserMsg
        if (session->header().token == kInvaildToken && msg_id != Cs::MSG_ID_USER) {            co_return;
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

    MILLION_CS_PROTO_MSG_DISPATCH();
    
    MILLION_CS_PROTO_MSG_ID(MSG_ID_USER);

    MILLION_CS_PROTO_MSG_HANDLE(Test, SubMsgIdUser::SUB_MSG_ID_USER_LOGIN_REQ, UserLoginReq, req) {
        
        std::cout << "login:" << req->user_name() << req->password() << std::endl;
        
        Cs::Test::UserLoginRes res;
        res.set_success(true);
        handle.user_session().Send(res);
        co_return;
    }

private:
    CommProtoMgr<UserHeader> proto_mgr_;
    GatewayServer server_;
    TokenGenerator token_generator_;
    std::unordered_map<Cs::MsgId, std::vector<ServiceHandle>> register_services_;
    std::list<UserSession> users_;
};

} // namespace gateway
} // namespace million
