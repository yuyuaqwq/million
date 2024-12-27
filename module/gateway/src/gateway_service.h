#pragma once

#include <yaml-cpp/yaml.h>

#include <million/iservice.h>

#include <gateway/api.h>
#include <gateway/gateway.h>

#include "gateway_server.h"

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

    virtual bool OnInit(million::MsgUnique msg) override {
        logger().Info("Gateway init start.");
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<GatewayTcpConnectionMsg>(service_handle(), std::move(connection));
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<GatewayTcpRecvPacketMsg>(service_handle(), std::move(connection), std::move(packet));
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

    //MILLION_PERSISTENT_SESSION_MSG_LOOP(CPP, GatewayServiceSessionCreateMsg, &GatewayServiceSessionCreateMsg::type_static());


    MILLION_CPP_MSG_HANDLE(GatewayTcpConnectionMsg, msg) {
        auto& connection = msg->connection;
        auto session = static_cast<UserSession*>(msg->connection.get());

        auto& ep = connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        if (connection->Connected()) {
            session->info().user_session_id = NewSession();
            users_.emplace(session->info().user_session_id, std::move(std::static_pointer_cast<UserSession>(connection)));
            logger().Debug("Gateway connection establishment: ip: {}, port: {}", ip, port);
        }
        else {
            users_.erase(session->info().user_session_id);
            logger().Debug("Gateway Disconnection: ip: {}, port: {}", ip, port);
        }
        co_return;
    }

    MILLION_CPP_MSG_HANDLE(GatewayTcpRecvPacketMsg, msg) {
        auto session = static_cast<UserSession*>(msg->connection.get());
        // auto session_handle = UserSessionHandle(session);
        auto persistent_session_id = session->info().user_session_id;

        logger().Trace("GatewayTcpRecvPacketMsg: {}, {}.", persistent_session_id, msg->packet.size());

        if (session->info().token == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            // session->info().token = info.token;

            // todo: 需要断开原先token指向的连接
        }
        // 没有token
        auto span = net::PacketSpan(msg->packet.begin() + kGatewayHeaderSize, msg->packet.end());
        auto iter = users_.find(session->info().user_session_id);
        if (iter == users_.end()) {
            logger().Err("Session does not exist.");
            co_return;
        }

        // if (session->info().token == kInvaildToken) {
        auto agent = iter->second->info().agent_.lock();
        if (!agent) {
            logger().Trace("packet send to user service.");
            SendTo<GatewayRecvPacketMsg>(user_service_, persistent_session_id, std::move(msg->packet), span);
        }
        else {
            logger().Trace("packet send to agent service.");
            Send<GatewayRecvPacketMsg>(iter->second->info().agent_, std::move(msg->packet), span);
        }
        co_return;
    }

    MILLION_CPP_MSG_HANDLE(GatewayRegisterUserServiceMsg, msg) {
        logger().Trace("GatewayRegisterUserServiceMsg.");
        user_service_ = msg->user_service;
        Reply(sender, session_id, std::move(msg));
        co_return;
    }

    MILLION_CPP_MSG_HANDLE(GatewaySureAgentMsg, msg) {
        auto iter = users_.find(session_id);
        if (iter == users_.end()) {
            logger().Err("Session does not exist.");
            co_return;
        }
        iter->second->info().agent_ = std::move(msg->agent_service);
        co_return;
    }

    MILLION_CPP_MSG_HANDLE(GatewaySendPacketMsg, msg) {
        auto iter = users_.find(session_id);
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

    std::unordered_map<SessionId, UserSessionShared> users_;

    static constexpr uint32_t kGatewayHeaderSize = 8;
};

} // namespace gateway
} // namespace million
