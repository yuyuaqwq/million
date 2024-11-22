#pragma once

#include <gateway/gateway_msg.h>

#include <million/iservice.h>

#include <gateway/api.h>

#include "gateway_server.h"

#define MILLION_CS_PROTO_MSG_DISPATCH() MILLION_PROTO_MSG_DISPATCH(Cs, ::million::gateway::UserSessionHandle)
#define MILLION_CS_PROTO_MSG_ID(MSG_ID_) MILLION_PROTO_MSG_ID(Cs, MSG_ID_)
#define MILLION_CS_PROTO_MSG_HANDLE(NAMESPACE_, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_) MILLION_PROTO_MSG_HANDLE(Cs::##NAMESPACE_, ::million::gateway::UserSessionHandle, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_)

namespace million {
namespace gateway {

namespace protobuf = google::protobuf; 

MILLION_MSG_DEFINE(, GatewayTcpConnectionMsg, (net::TcpConnectionShared)connection)
MILLION_MSG_DEFINE(, GatewayTcpRecvPacketMsg, (net::TcpConnectionShared)connection, (net::Packet)packet)

class GatewayService : public IService {
public:
    using Base = IService;
    GatewayService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) {
            Send<GatewayTcpConnectionMsg>(service_handle(), connection);
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) {
            Send<GatewayTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
        });
        const auto& config = imillion_->YamlConfig();
        const auto& gateway_config = config["gateway"];
        if (!gateway_config) {
            std::cerr << "[gateway] [config] [error] cannot find 'gateway'." << std::endl;
            return false;
        }
        const auto& port_config = gateway_config["port"];
        if (!port_config)
        {
            std::cerr << "[gateway] [config] [error] cannot find 'gateway.port'." << std::endl;
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        return true;
    }

    MILLION_MSG_DISPATCH(GatewayService);

    MILLION_MSG_HANDLE(GatewayTcpConnectionMsg, msg) {
        auto& connection = msg->connection;
        auto session = static_cast<UserSession*>(msg->connection.get());
        if (connection->Connected()) {
            auto user_session_id = ++user_session_id_;
            session->header().user_session_id = user_session_id;
            users_.emplace(session->header().user_session_id, session);
        }
        else {
            users_.erase(session->header().user_session_id);
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayTcpRecvPacketMsg, msg) {
        auto session = static_cast<UserSession*>(msg->connection.get());
        auto session_handle = UserSessionHandle(session);

        if (session->header().token == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            // session->header().token = header.token;

            // todo: 需要断开原先token指向的连接
        }
        auto user_session_id = session->header().user_session_id;
        // 没有token
        if (session->header().token == kInvaildToken) {
            Send<GatewayRecvPacketMsg>(user_service_, user_session_id, std::move(msg->packet));
        }
        else {
            auto iter = agent_services_.find(session->header().user_session_id);
            if (iter != agent_services_.end()) {
                Send<GatewayRecvPacketMsg>(iter->second, user_session_id, std::move(msg->packet));
            }
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayRegisterUserServiceMsg, msg) {
        user_service_ = msg->user_service;
        co_return;
    }

    MILLION_MSG_HANDLE(GatewaySureAgentMsg, msg) {
        agent_services_.emplace(msg->session, msg->agent_service);
        co_return;
    }

    MILLION_MSG_HANDLE(GatewaySendPacketMsg, msg) {
        auto iter = users_.find(msg->session);
        if (iter == users_.end()) {
            co_return;
        }
        iter->second->Send(std::move(msg->packet));
        co_return;
    }

private:
    GatewayServer server_;
    TokenGenerator token_generator_;
    ServiceHandle user_service_;
    // 需要改掉UserSession*
    std::atomic_uint64_t user_session_id_ = 0;
    std::unordered_map<uint64_t, UserSession*> users_;
    std::unordered_map<uint64_t, ServiceHandle> agent_services_;
};

} // namespace gateway
} // namespace million
