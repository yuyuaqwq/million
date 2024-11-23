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
        LOG_INFO("Gateway init start.");
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<GatewayTcpConnectionMsg>(service_handle(), connection);
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<GatewayTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
            co_return;
        });
        const auto& config = imillion_->YamlConfig();
        const auto& gateway_config = config["gateway"];
        if (!gateway_config) {
            LOG_ERR("cannot find 'gateway'.");
            return false;
        }
        const auto& port_config = gateway_config["port"];
        if (!port_config)
        {
            LOG_ERR("cannot find 'gateway.port'.");
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        LOG_INFO("Gateway init success.");
        return true;
    }

    MILLION_MSG_DISPATCH(GatewayService);

    MILLION_MSG_HANDLE(GatewayTcpConnectionMsg, msg) {
        auto& connection = msg->connection;
        auto session = static_cast<UserSession*>(msg->connection.get());

        auto& ep = connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        if (connection->Connected()) {
            auto user_session_id = ++user_session_id_;
            session->info().user_session_id = user_session_id;
            users_.emplace(session->info().user_session_id, session);
            LOG_DEBUG("Gateway connection establishment: ip: {}, port: {}", ip, port);
        }
        else {
            users_.erase(session->info().user_session_id);
            LOG_DEBUG("Gateway Disconnection: ip: {}, port: {}", ip, port);
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayTcpRecvPacketMsg, msg) {
        auto session = static_cast<UserSession*>(msg->connection.get());
        auto session_handle = UserSessionHandle(session);

        if (session->info().token == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            // session->info().token = info.token;

            // todo: 需要断开原先token指向的连接
        }
        auto user_session_id = session->info().user_session_id;
        // 没有token
        auto span = net::PacketSpan(msg->packet.begin() + kGatewayHeaderSize, msg->packet.end());
        if (session->info().token == kInvaildToken) {
            Send<GatewayRecvPacketMsg>(user_service_, user_session_id, std::move(msg->packet), span);
        }
        else {
            auto iter = agent_services_.find(session->info().user_session_id);
            if (iter != agent_services_.end()) {
                Send<GatewayRecvPacketMsg>(iter->second, user_session_id, std::move(msg->packet), span);
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

        auto header_packet = net::Packet(kGatewayHeaderSize);
        iter->second->Send(std::move(header_packet), net::PacketSpan(header_packet.begin(), header_packet.end()), header_packet.size() + msg->packet.size());
        iter->second->Send(std::move(msg->packet), net::PacketSpan(msg->packet.begin(), msg->packet.end()), 0);
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

    static constexpr uint32_t kGatewayHeaderSize = 8;
};

} // namespace gateway
} // namespace million
