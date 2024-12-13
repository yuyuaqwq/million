#pragma once

#include <yaml-cpp/yaml.h>

#include <million/iservice.h>

#include <gateway/api.h>
#include <gateway/gateway.h>

#include "gateway_server.h"

#define MILLION_CS_PROTO_MSG_DISPATCH() MILLION_PROTO_MSG_DISPATCH(Cs, ::million::agent::UserSessionHandle)
#define MILLION_CS_PROTO_MSG_ID(MSG_ID_) MILLION_PROTO_MSG_ID(Cs, MSG_ID_)
#define MILLION_CS_PROTO_MSG_HANDLE(NAMESPACE_, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_) MILLION_PROTO_MSG_HANDLE(Cs::##NAMESPACE_, ::million::agent::UserSessionHandle, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_)

namespace million {
namespace gateway {

namespace protobuf = google::protobuf; 

MILLION_MSG_DEFINE(, GatewayTcpConnectionMsg, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(, GatewayTcpRecvPacketMsg, (net::TcpConnectionShared) connection, (net::Packet) packet)

// 网关服务有两种模式
// 集群、非集群
// 集群模式：
    // 网关服务分为主服务及代理服务，主服务负责收包，部署在单节点上
    // 其他需要接收网关转发的包的节点，需要部署一个代理服务，代理服务负责接收集群服务转发来的网关主服务发的包，再转发给节点内的其他服务
class GatewayService : public IService {
public:
    using Base = IService;
    GatewayService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        logger().Info("Gateway init start.");
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<GatewayTcpConnectionMsg>(service_handle(), connection);
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<GatewayTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
            co_return;
        });
        const auto& config = imillion().YamlConfig();
        const auto& gateway_config = config["gateway"];
        if (!gateway_config) {
            logger().Err("cannot find 'gateway'.");
            return false;
        }
        const auto& port_config = gateway_config["port"];
        if (!port_config)
        {
            logger().Err("cannot find 'gateway.port'.");
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        logger().Info("Gateway init success.");
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
            auto user_context_id = ++user_context_id_;
            session->info().user_context_id = user_context_id;
            users_.emplace(session->info().user_context_id, session);
            logger().Debug("Gateway connection establishment: ip: {}, port: {}", ip, port);
        }
        else {
            users_.erase(session->info().user_context_id);
            logger().Debug("Gateway Disconnection: ip: {}, port: {}", ip, port);
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayTcpRecvPacketMsg, msg) {
        auto session = static_cast<UserSession*>(msg->connection.get());
        // auto session_handle = UserSessionHandle(session);
        auto user_context_id = session->info().user_context_id;

        logger().Trace("GatewayTcpRecvPacketMsg: {}, {}.", user_context_id, msg->packet.size());

        if (session->info().token == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            // session->info().token = info.token;

            // todo: 需要断开原先token指向的连接
        }
        // 没有token
        auto span = net::PacketSpan(msg->packet.begin() + kGatewayHeaderSize, msg->packet.end());
        auto iter = agent_services_.find(session->info().user_context_id);

        
        // if (session->info().token == kInvaildToken) {
        if (iter == agent_services_.end()) {
            logger().Trace("packet send to user service.");
            Send<GatewayRecvPacketMsg>(user_service_, user_context_id, std::move(msg->packet), span);
        }
        else {
            logger().Trace("packet send to agent service.");
            Send<GatewayRecvPacketMsg>(iter->second, user_context_id, std::move(msg->packet), span);
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayRegisterUserServiceMsg, msg) {
        logger().Trace("GatewayRegisterUserServiceMsg.");
        user_service_ = msg->user_service;
        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(GatewaySureAgentMsg, msg) {
        agent_services_.emplace(msg->context_id, msg->agent_service);
        co_return;
    }

    MILLION_MSG_HANDLE(GatewaySendPacketMsg, msg) {
        auto iter = users_.find(msg->context_id);
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
    std::atomic<UserContextId> user_context_id_ = 0;
    std::unordered_map<UserContextId, UserSession*> users_;
    std::unordered_map<UserContextId, ServiceHandle> agent_services_;

    static constexpr uint32_t kGatewayHeaderSize = 8;
};

} // namespace gateway
} // namespace million
