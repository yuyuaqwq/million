#pragma once

#include <million/iservice.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

MILLION_MSG_DEFINE(, ConnectionMsg, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(, RecvPacketMsg, (net::TcpConnection*) connection, (net::Packet) packet)

class ClusterService : public IService {
public:
    using Base = IService;
    ClusterService(IMillion* imillion)
        : Base(imillion)
        // , server_(imillion, &proto_mgr_)
    { }

    virtual void OnInit() override {
        // proto_mgr_.InitMsgMap();
        // proto_mgr_.RegistrySubMsg(Cs::MSG_ID_USER, Cs::cs_sub_msg_id_user);

        // io线程回调，发给work线程处理
        //server_.set_on_connection([this](auto connection) {
        //    Send<ConnectionMsg>(service_handle(), std::move(connection));
        //});
        //server_.set_on_msg([this](auto& connection, auto&& packet) {
        //    Send<RecvPacketMsg>(service_handle(), &connection, std::move(packet));
        //});
        //server_.Start(8002);
    }

    //MILLION_MSG_DISPATCH(GatewayService);

    //MILLION_MSG_HANDLE(ConnectionMsg, msg) {
    //    co_return;
    //}

    //MILLION_MSG_HANDLE(RecvPacketMsg, msg) {
    //    UserHeader header;
    //    auto res = proto_mgr_.DecodeMessage(msg->packet, &header);
    //    if (!res) co_return;
    //    auto&& [proto_msg, msg_id, sub_msg_id] = *res;

    //    auto session = static_cast<UserSession*>(msg->connection);
    //    auto handle = UserSessionHandle(session);

    //    if (session->header().token == kInvaildToken) {
    //        // 连接没token，但是发来了token，当成断线重连处理
    //        session->header().token = header.token;

    //        // todo: 需要断开原先token指向的连接
    //    }

    //    // 没登录，只能分发给UserMsg
    //    if (session->header().token == kInvaildToken && msg_id != Cs::MSG_ID_USER) {
    //        co_return;
    //    }

    //    auto iter = register_services_.find(msg_id);
    //    if (iter != register_services_.end()) {
    //        auto& services = iter->second;
    //        for (auto& service : services) {
    //            Send<RecvProtoMsg>(service, handle, std::move(proto_msg));
    //        }
    //    }

    //    // auto res_proto_msg = co_await Cluster.Call<ProtoMsgUnique>(src_unique_name, target_unique_name, proto_msg);



    //    // auto token = token_generator_.Generate();

    //    co_return;
    //}

    //MILLION_MSG_HANDLE(RegisterServiceMsg, msg) {
    //    register_services_[msg->cs_msg_id].push_back(msg->service_handle);
    //    co_return;
    //}

    ////MILLION_MSG_HANDLE(UnRegisterServiceMsg, msg) {
    ////    auto iter = register_services_.find(msg->cs_msg_id);
    ////    if (iter == register_services_.end()) co_return;
    ////    auto& services = iter->second;
    ////    auto remove = std::remove(services.begin(), services.end(), msg->service_handle);
    ////    services.erase(remove, services.end());
    ////    Reply(msg->sender(), msg->session_id());
    ////    co_return;
    ////}

    //MILLION_MSG_HANDLE(SendProtoMsg, msg) {
    //    msg->session_handle.user_session().Send(*msg->proto_msg);
    //    co_return;
    //}

    //MILLION_PROTO_MSG_DISPATCH(Cs);
    //
    //MILLION_PROTO_MSG_ID(Cs, MSG_ID_USER);

    //MILLION_PROTO_MSG_HANDLE(Cs, SubMsgIdUser::SUB_MSG_ID_USER_LOGIN, UserRegisterReq, req) {
    //    
    //    std::cout << "login:" << req->user_name() << req->password() << std::endl;
    //    co_return;
    //}

private:
    // CsProtoMgr proto_mgr_;
    //ClusterServer server_;
    // TokenGenerator token_generator_;
    // std::unordered_map<Cs::MsgId, std::vector<ServiceHandle>> register_services_;
    // std::list<UserSession> users_;
};

} // namespace gateway
} // namespace million
