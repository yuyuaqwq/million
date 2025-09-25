#pragma once

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>
#include <million/gateway/ss_gateway.pb.h>
#include <million/gateway/api.h>
#include <million/gateway/gateway.h>

#include "gateway_server.h"

namespace million {
namespace gateway {

namespace protobuf = google::protobuf; 

MILLION_MESSAGE_DEFINE(, GatewayTcpConnection, (UserSessionShared) user_session)
MILLION_MESSAGE_DEFINE(, GatewayTcpRecvPacket, (UserSessionShared) user_session, (net::Packet) packet)

MILLION_MESSAGE_DEFINE(, GatewayPersistentUserSession, (UserSessionShared) user_session);

// 跨节点：
    // 网关服务分为主服务及代理服务，主服务负责收包，部署在网关节点上
    // 其他需要接收网关转发的包的节点，需要部署一个代理服务，代理服务负责接收集群服务转发来的网关主服务发的包，再转发给节点内的其他服务
class GatewayService : public IService {
    MILLION_SERVICE_DEFINE(GatewayService);

public:
    using Base = IService;
    GatewayService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        return imillion().SetServiceNameId(service_handle(), module::module_id, ss::ServiceNameId_descriptor(), ss::SERVICE_NAME_ID_GATEWAY);
    }


    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override {
        const auto& settings = imillion().YamlSettings();
        const auto& gateway_settings = settings["gateway"];
        if (!gateway_settings) {
            TaskAbort("cannot find 'gateway'.");
            co_return nullptr;
        }
        const auto& port_settings = gateway_settings["port"];
        if (!port_settings) {
            TaskAbort("cannot find 'gateway.port'.");
            co_return nullptr;
        }
        const auto port = port_settings.as<uint16_t>();

        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<GatewayTcpConnection>(service_handle(), std::move(std::static_pointer_cast<UserSession>(connection)));
            co_return;
            });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<GatewayTcpRecvPacket>(service_handle(), std::move(std::static_pointer_cast<UserSession>(connection)), std::move(packet));
            co_return;
            });
        server_.Start(port);
        logger().LOG_INFO("Gateway start.");
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(GatewayPersistentUserSession, session_msg) {
        auto& user_session = *session_msg->user_session;
        do {
            auto recv_msg = co_await RecvWithTimeout(session_id, ::million::kSessionNeverTimeout);
            // imillion().SendTo(sender, service_handle(), session_id, std::move(recv_msg));
            if (recv_msg.IsProtoMessage()) {
                logger().LOG_TRACE("Gateway Recv ProtoMessage: {}.", session_id);
                auto header_packet = net::Packet(kGatewayHeaderSize);

                auto proto_msg = std::move(recv_msg.GetProtoMessage());
                auto packet = imillion().proto_mgr().codec().EncodeMessage(*proto_msg);
                if (!packet) {
                    logger().LOG_ERROR("Gateway Recv ProtoMessage EncodeMessage failed: {}.", session_id);
                    continue;
                }
                auto span = net::PacketSpan(header_packet);
                user_session.Send(std::move(header_packet), span, header_packet.size() + packet->size());
                span = net::PacketSpan(*packet);
                user_session.Send(std::move(*packet), span, 0);
            }
            else if (recv_msg.IsType<GatewaySendPacket>()) {
                logger().LOG_TRACE("GatewaySendPacket: {}.", session_id);
                auto header_packet = net::Packet(kGatewayHeaderSize);
                auto msg = recv_msg.GetMutableMessage<GatewaySendPacket>();
                auto span = net::PacketSpan(header_packet);
                user_session.Send(std::move(header_packet), span, header_packet.size() + msg->packet.size());
                span = net::PacketSpan(msg->packet);
                user_session.Send(std::move(msg->packet), span, 0);
            }
            else if (recv_msg.IsType<GatewaySureAgent>()) {
                auto msg = recv_msg.GetMutableMessage<GatewaySureAgent>();
                user_session.set_agent_handle(std::move(msg->agent_service));
            }
            else if (recv_msg.IsType<GatewayResetAgentId>()) {
                auto msg = recv_msg.GetMessage<GatewayResetAgentId>();
                user_session.set_agent_id(msg->agent_id);
                SendTo(service_handle(), msg->agent_id, std::move(msg_));
                break;
            }
            else if (recv_msg.IsType<GatewayPersistentUserSession>()) {
                break;
            }
        } while (true);
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(GatewayTcpConnection, msg) {
        auto& user_session = *msg->user_session;

        auto& ep = user_session.remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        if (user_session.Connected()) {
            // 开启持久会话，这里设计上，Send 创建的 session_id，则视作 agent_id
            auto agent_id = Send<GatewayPersistentUserSession>(service_handle(), std::move(msg->user_session));
            user_session.set_agent_id(agent_id.value());

            logger().LOG_DEBUG("Gateway connection establishment, agent_id:{}, ip: {}, port: {}", user_session.agent_id(), ip, port);
        }
        else {
            // 停止持久会话
            Reply<GatewayPersistentUserSession>(service_handle(), user_session.agent_id(), std::move(msg->user_session));

            logger().LOG_DEBUG("Gateway Disconnection: ip: {}, port: {}", ip, port);
        }
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(GatewayTcpRecvPacket, msg) {
        auto& user_session = *msg->user_session;
        auto agent_id = user_session.agent_id();

        logger().LOG_TRACE("GatewayTcpRecvPacketMsg: {}, {}.", agent_id, msg->packet.size());

        if (user_session.token() == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            // user_session.set_token(info.token);

            // todo: 需要断开原先token指向的连接
        }
        // 没有token

        if (msg->packet.size() < kGatewayHeaderSize){
            logger().LOG_WARN("GatewayTcpRecvPacketMsg size err: {}, {}.", agent_id, msg->packet.size());
            co_return nullptr;
        }

        auto span = net::PacketSpan(msg->packet.begin() + kGatewayHeaderSize, msg->packet.end());

        // 将消息转发给本机节点的其他服务
        auto res = imillion().proto_mgr().codec().DecodeMessage(span);
        if (!res) {
            logger().LOG_WARN("GatewayTcpRecvPacketMsg unresolvable message: {}, {}.", agent_id, msg->packet.size());
            co_return nullptr;
        }

        // if (session->token == kInvaildToken)

        // 指定user_session_id，目标在通过Reply回包时，会在GatewayPersistentUserSessionMsg中循环接收处理
        if (!user_session.agent_handle().lock()) {
            logger().LOG_TRACE("packet send to user service.");
            SendTo(user_service_, agent_id, std::move(res->msg));
        }
        else {
            logger().LOG_TRACE("packet send to agent service.");
            SendTo(user_session.agent_handle(), agent_id, std::move(res->msg));
        }
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(GatewayRegisterUserServiceReq, msg) {
        logger().LOG_TRACE("GatewayRegisterUserServiceReq.");
        user_service_ = msg->user_service;
        co_return make_message<GatewayRegisterUserServiceResp>();
    }


private:
    GatewayServer server_;
    TokenGenerator token_generator_;
    ServiceHandle user_service_;

    static constexpr uint32_t kGatewayHeaderSize = 8;
};

} // namespace gateway
} // namespace million
