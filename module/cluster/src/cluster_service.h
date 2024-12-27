#pragma once

#include <million/iservice.h>
#include <million/proto_codec.h>

#include <protogen/ss/ss_cluster.pb.h>

#include <cluster/cluster.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

MILLION_MSG_DEFINE(, ClusterTcpConnectionMsg, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(, ClusterTcpRecvPacketMsg, (net::TcpConnectionShared) connection, (net::Packet) packet)

// SessionId部分，必须是a只能等待自己节点alloc的SessionId，想等待b的消息，必须是a把sessionid发给b，b再发回这个sessionid


using NodeServiceSessionId = uint64_t;

class ClusterService : public IService {
public:
    using Base = IService;
    ClusterService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit(million::MsgUnique msg) override {
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<ClusterTcpConnectionMsg>(service_handle(), std::move(connection));
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<ClusterTcpRecvPacketMsg>(service_handle(), std::move(connection), std::move(packet));
            co_return;
        });

        const auto& config = imillion().YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            logger().Err("cannot find 'cluster'.");
            return false;
        }

        const auto& name_config = cluster_config["name"];
        if (!name_config)
        {
            logger().Err("cannot find 'cluster.name'.");
            return false;
        }
        node_name_ = name_config.as<std::string>();

        const auto& port_config = cluster_config["port"];
        if (!port_config)
        {
            logger().Err("cannot find 'cluster.port'.");
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        return true;
    }

    MILLION_MSG_DISPATCH(ClusterService);

    MILLION_PERSISTENT_SESSION_MSG_LOOP(CPP, GatewayServiceSessionCreateMsg, &GatewayServiceSessionCreateMsg::type_static());


    MILLION_CPP_MSG_HANDLE(ClusterTcpConnectionMsg, msg) {
        auto& ep = msg->connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());
        if (msg->connection->Connected()) {
            logger().Debug("Connection establishment: ip: {}, port: {}", ip, port);
        }
        else {
            auto node_session = msg->connection->get_ptr<NodeSession>();
            logger().Debug("Disconnection: ip: {}, port: {}, cur_node: {}, target_node : {}", ip, port, node_name_, node_session->info().node_name);
            DeleteNodeSession(node_session);
        }

        // 可能是主动发起，也可能是被动收到连接的，连接去重不在这里处理
        // 连接投递到定时任务，比如10s还没有握手成功就断开连接
        
        co_return;
    }

    MILLION_CPP_MSG_HANDLE(ClusterTcpRecvPacketMsg, msg) {
        auto& ep = msg->connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        uint32_t header_size = 0;
        if (msg->packet.size() < sizeof(header_size)) {
            logger().Warn("Invalid header size: ip: {}, port: {}", ip, port);
            msg->connection->Close();
            co_return;
        }
        header_size = *reinterpret_cast<uint32_t*>(msg->packet.data());
        header_size = asio::detail::socket_ops::network_to_host_long(header_size);

        ss::cluster::MsgBody msg_body;
        if (!msg_body.ParseFromArray(msg->packet.data() + sizeof(header_size), header_size)) {
            logger().Warn("Invalid ClusterMsg: ip: {}, port: {}", ip, port);
            msg->connection->Close();
            co_return;
        }

        auto node_session = msg->connection->get_ptr<NodeSession>();

        switch (msg_body.body_case()) {
        case ss::cluster::MsgBody::BodyCase::kHandshakeReq: {
            // 收到握手请求，当前是被动连接方
            auto& req = msg_body.handshake_req();
            node_session->info().node_name = req.src_node();

            // 还需要检查是否与目标节点存在连接，如果已存在，说明该连接已经失效，需要断开
            auto old_node_session = FindNodeSession(req.src_node(), nullptr);
            if (old_node_session) {
                old_node_session->Close();
                logger().Info("old connection, ip: {}, port: {}, cur_node: {}, req_src_node: {}", ip, port, node_name_, req.src_node());
            }

            // 能否匹配都回包
            ss::cluster::MsgBody msg_body;
            auto* res = msg_body.mutable_handshake_res();
            res->set_target_node(node_name_);
            auto packet = ProtoMsgToPacket(msg_body);

            SendInit(node_session, packet, net::Packet());

            auto span = net::PacketSpan(packet.begin(), packet.end());
            node_session->Send(std::move(packet), span, 0);

            if (node_name_ != req.target_node()) {
                logger().Err("Src node name mismatch, ip: {}, port: {}, cur_node: {}, req_target_node: {}", ip, port, node_name_, req.target_node());
                node_session->Close();
                co_return;
            }

            // 创建节点
            CreateNodeSession(std::move(msg->connection), false);
            break;
        }
        case ss::cluster::MsgBody::BodyCase::kHandshakeRes: {
            // 收到握手响应，当前是主动连接方
            auto& res = msg_body.handshake_res();
            if (node_session->info().node_name != res.target_node()) {
                logger().Err("Target node name mismatch, ip: {}, port: {}, target_node: {}, res_target_node: {}", ip, port, node_session->info().node_name, res.target_node());
                msg->connection->Close();
                co_return;
            }
            
            // 创建节点
            auto& node_session_ref = CreateNodeSession(std::move(msg->connection), true);

            // 完成连接，发送所有队列包
            for (auto iter = send_queue_.begin(); iter != send_queue_.end(); ) {
                auto& msg = *iter;
                if (msg->target_node == res.target_node()) {
                    ForwardPacket(&node_session_ref, std::move(msg));
                    send_queue_.erase(iter++);
                }
                else {
                    ++iter;
                }
            }
            wait_nodes_.erase(res.target_node());
            break;
        }
        case ss::cluster::MsgBody::BodyCase::kForwardHeader: {
            // 还需要判断下，连接没有完成握手，则不允许转发包

            auto& header = msg_body.forward_header();
            auto& src_service = header.src_service();
            auto& target_service = header.target_service();
            auto target_service_handle = imillion().GetServiceByName(target_service);
            if (target_service_handle) {
                // 还需要获取下源节点
                auto span = net::PacketSpan(msg->packet.begin() + sizeof(header_size) + header_size, msg->packet.end());
                Send<ClusterRecvPacketMsg>(*target_service_handle, 
                    NodeSessionHandle{ .src_node = node_session->info().node_name, .src_service = src_service},
                    std::move(msg->packet), span);
            }
            break;
        }
        }
        co_return;
    }

    MILLION_CPP_MSG_HANDLE(ClusterSendPacketMsg, msg) {
        EndPointRes end_point;
        auto node_session = FindNodeSession(msg->target_node, &end_point);
        if (!node_session && end_point.ip.empty()) {
            logger().Err("Node cannot be found: {}.", msg->target_node);
            co_return;
        }
        if (node_session) {
            ForwardPacket(node_session, std::move(msg));
            co_return;
        }

        if (wait_nodes_.find(msg->target_node) == wait_nodes_.end()) {
            wait_nodes_.emplace(msg->target_node);
            auto& io_context = imillion().NextIoContext();
            asio::co_spawn(io_context.get_executor(), [this, target_node = msg->target_node, end_point = std::move(end_point)]() mutable -> asio::awaitable<void> {
                auto connection_opt = co_await server_.ConnectTo(end_point.ip, end_point.port);
                if (!connection_opt) {
                    logger().Err("server_.ConnectTo failed, ip: {}, port: {}.", end_point.ip, end_point.port);
                    co_return;
                }

                // 握手
                auto connection = *connection_opt;
                auto node_session = connection->get_ptr<NodeSession>();

                ss::cluster::MsgBody msg_body;
                auto* req = msg_body.mutable_handshake_req();
                req->set_src_node(node_name_);
                req->set_target_node(target_node);
                auto packet = ProtoMsgToPacket(msg_body);

                SendInit(node_session, packet, net::Packet());

                auto span = net::PacketSpan(packet.begin(), packet.end());
                node_session->Send(std::move(packet), span, 0);

                node_session->info().node_name = std::move(target_node);
                }, asio::detached);
        }

        // 把正处于握手状态的需要send的所有包放到队列里，在握手完成时统一发包
        // 使用一个全局队列，因为这种连接排队的包很少，直接扫描就行
        send_queue_.emplace_back(std::move(msg));

        co_return;
    }

private:
    NodeSession& CreateNodeSession(net::TcpConnectionShared&& connection, bool active) {
        // 如果存在则需要插入失败
        auto node_session = connection->get_ptr<NodeSession>();
        auto& target_node_name = node_session->info().node_name;
        auto res = nodes_.emplace(target_node_name, connection);
        if (res.second) {
            logger().Info("CreateNode: {}", target_node_name);
        }
        else {
            // 已存在，比较两边节点名的字典序，选择断开某一条连接
            bool disconnect_old;
            if (active) {
                // 主动建立
                disconnect_old = node_name_ > target_node_name;
            }
            else {
                // 被动建立
                disconnect_old = target_node_name > node_name_;
            }

            if (disconnect_old) {
                // 断开旧连接，关联新连接
                res.first->second->Close();
                res.first->second = std::move(std::static_pointer_cast<NodeSession>(connection));
                logger().Info("Duplicate connection, disconnect old connection: {}", target_node_name);
            }
            else {
                // 断开新连接
                connection->Close();
                logger().Info("Duplicate connection, disconnect new connection: {}", target_node_name);
            }
        }
        return *res.first->second->get_ptr<NodeSession>();
    }

    struct EndPointRes {
        std::string ip;
        std::string port;
    };
    NodeSession* FindNodeSession(NodeName node_name, EndPointRes* end_point) {
        auto iter = nodes_.find(node_name);
        if (iter != nodes_.end()) {
            return iter->second->get_ptr<NodeSession>();
        }

        // 尝试从配置文件中查找此节点
        const auto& config = imillion().YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            logger().Err("cannot find 'cluster'.");
            return nullptr;
        }

        const auto& nodes = cluster_config["nodes"];
        if (!nodes) {
            logger().Err("cannot find 'cluster.nodes'.");
            return nullptr;
        }

        if (end_point) {
            for (auto node : nodes) {
                auto node_name = node.first.as<std::string>();
                if (node_name != node_name) continue;

                end_point->ip = node.second["ip"].as<std::string>();
                end_point->port = node.second["port"].as<std::string>();

                break;
            }
        }
        return nullptr;
    }

    void DeleteNodeSession(NodeSession* node_session) {
        nodes_.erase(node_session->info().node_name);
    }

    void SendInit(NodeSession* node_session, const net::Packet& header_packet, const net::Packet& forward_packet) {
        uint32_t header_size = header_packet.size();
        header_size = asio::detail::socket_ops::host_to_network_long(header_size);
        auto size_packet = net::Packet(sizeof(header_size));
        std::memcpy(size_packet.data(), &header_size, sizeof(header_size));
        auto size_span = net::PacketSpan(size_packet.begin(), size_packet.end());
        auto total_size = sizeof(header_size) + header_packet.size() + forward_packet.size();
        node_session->Send(std::move(size_packet), size_span, total_size);
    }

    void ForwardPacket(NodeSession* node_session, std::unique_ptr<ClusterSendPacketMsg> msg) {
        // 追加集群头部
        ss::cluster::MsgBody msg_body;
        auto* header = msg_body.mutable_forward_header();
        header->set_src_service(std::move(msg->src_service));
        header->set_target_service(std::move(msg->target_service));
        auto header_packet = ProtoMsgToPacket(msg_body);
        
        SendInit(node_session, header_packet, msg->packet);

        auto header_span = net::PacketSpan(header_packet.begin(), header_packet.end());
        node_session->Send(std::move(header_packet), header_span, 0);

        auto span = net::PacketSpan(msg->packet.begin(), msg->packet.end());
        node_session->Send(std::move(msg->packet), span, 0);
    }

private:
    ClusterServer server_;
    NodeName node_name_;

    std::unordered_map<NodeName, NodeSessionShared> nodes_;

    std::set<NodeName> wait_nodes_;
    std::list<std::unique_ptr<ClusterSendPacketMsg>> send_queue_;
};

} // namespace cluster
} // namespace million
