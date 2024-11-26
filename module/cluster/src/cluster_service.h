#pragma once

#include <million/iservice.h>

#include <protogen/ss/ss_cluster.pb.h>

#include <cluster/cluster_msg.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

namespace Ss = ::Million::Proto::Ss;

MILLION_MSG_DEFINE(, ClusterTcpConnectionMsg, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(, ClusterTcpRecvPacketMsg, (net::TcpConnectionShared) connection, (net::Packet) packet)

class ClusterService : public IService {
public:
    using Base = IService;
    ClusterService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](const net::TcpConnectionShared& connection) -> asio::awaitable<void> {
            Send<ClusterTcpConnectionMsg>(service_handle(), connection);
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<ClusterTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
            co_return;
        });

        const auto& config = imillion_->YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            LOG_ERR("cannot find 'cluster'.");
            return false;
        }

        const auto& name_config = cluster_config["name"];
        if (!name_config)
        {
            LOG_ERR("cannot find 'cluster.name'.");
            return false;
        }
        node_name_ = name_config.as<std::string>();

        const auto& port_config = cluster_config["port"];
        if (!port_config)
        {
            LOG_ERR("cannot find 'cluster.port'.");
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        return true;
    }

    MILLION_MSG_DISPATCH(ClusterService);

    MILLION_MSG_HANDLE(ClusterTcpConnectionMsg, msg) {
        auto& ep = msg->connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());
        if (msg->connection->Connected()) {
            LOG_DEBUG("Connection establishment: ip: {}, port: {}", ip, port);
        }
        else {
            LOG_DEBUG("Disconnection: ip: {}, port: {}", ip, port);
        }

        // 可能是主动发起，也可能是被动收到连接的，连接去重不在这里处理
        // 连接投递到定时任务，比如10s还没有握手成功就断开连接
        
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterTcpRecvPacketMsg, msg) {
        auto& ep = msg->connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        Ss::Cluster::ClusterMsg cluster_msg;
        if (!cluster_msg.ParseFromArray(msg->packet.data(), msg->packet.size())) {
            LOG_WARN("Invalid ClusterMsg: ip: {}, port: {}", ip, port);
            msg->connection->Close();
            co_return;
        }

        auto node_session = msg->connection->get_ptr<NodeSession>();

        switch (cluster_msg.body_case()) {
        case Ss::Cluster::ClusterMsg::BodyCase::kHandshakeReq: {
            // 收到握手请求，当前是被动连接方
            auto& req = cluster_msg.handshake_req();
            if (node_name_ != req.src_node()) {
                LOG_ERR("Src node name mismatch, ip: {}, port: {}, cur_src_node: {}, req_src_node: {}", ip, port, node_name_, req.src_node());
                msg->connection->Close();
                co_return;
            }

            Ss::Cluster::ClusterMsg cluster_msg;
            auto* res = cluster_msg.mutable_handshake_res();
            res->set_target_node(node_name_);
            auto packet = ProtoMsgToPacket(cluster_msg);
            msg->connection->Send(std::move(packet));

            // 创建节点

            break;
        }
        case Ss::Cluster::ClusterMsg::BodyCase::kHandshakeRes: {
            // 收到握手响应，当前是主动连接方
            auto& res = cluster_msg.handshake_res();
            if (node_session->info().node_name != res.target_node()) {
                LOG_ERR("Target node name mismatch, ip: {}, port: {}, cur_target_node: {}, res_target_node: {}", ip, port, node_session->info().node_name, res.target_node());
                msg->connection->Close();
                co_return;
            }
            // 完成连接，发送所有队列包

            for (auto iter = send_queue_.begin(); iter != send_queue_.end(); ) {
                auto& msg = *iter;
                if (msg->target_node == res.target_node()) {
                    ForwardPacket(std::move(msg), node_session);
                    send_queue_.erase(iter++);
                }
                else {
                    ++iter;
                }
            }
            wait_nodes_.erase(res.target_node());

            // 创建节点
            break;
        }
        case Ss::Cluster::ClusterMsg::BodyCase::kForwardHeader: {
            // 还需要判断下，连接状态没有完成握手，则不允许转发包

            auto& header = cluster_msg.forward_header();
            auto& src_service = header.src_service();
            auto& target_service = header.target_service();
            auto target_service_handle = imillion_->GetServiceByUniqueNum(target_service);
            if (target_service_handle) {
                // 还需要获取下源节点
                auto span = net::PacketSpan(msg->packet.begin() + header.ByteSize(), msg->packet.end());
                Send<ClusterRecvPacketMsg>(*target_service_handle, 
                    NodeSessionHandle{ .src_node = node_session->info().node_name, .src_service = src_service},
                    std::move(msg->packet), span);
            }
            break;
        }
        }
        //Reply<ClusterHandshakeResMsg>(service_handle(), session_id, msg->connection);

        //CreateNode(req.src_node(), std::move(msg->connection), false);
        

        //if (node_session->info().node_name.empty()) {
        //    
        //}
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterSendPacketMsg, msg) {
        EndPointRes end_point;
        auto node_session = FindNode(msg->target_node, &end_point);
        if (!node_session && end_point.ip.empty()) {
            LOG_ERR("Node cannot be found: {}.", msg->target_node);
            co_return;
        }
        if (node_session) {
            ForwardPacket(std::move(msg), node_session);
            co_return;
        }

        if (wait_nodes_.find(msg->target_node) == wait_nodes_.end()) {
            wait_nodes_.emplace(msg->target_node);
            auto& io_context = imillion_->NextIoContext();
            asio::co_spawn(io_context.get_executor(), [this, msg = std::move(msg), end_point = std::move(end_point)]() mutable -> asio::awaitable<void> {
                auto connection_opt = co_await server_.ConnectTo(end_point.ip, end_point.port);
                if (!connection_opt) {
                    LOG_ERR("server_.ConnectTo failed, ip: {}, port: {}.", end_point.ip, end_point.port);
                    co_return;
                }

                // 握手
                auto connection = *connection_opt;
                Ss::Cluster::ClusterMsg cluster_msg;
                auto* req = cluster_msg.mutable_handshake_req();
                req->set_src_node(node_name_);
                auto packet = ProtoMsgToPacket(cluster_msg);
                connection->Send(std::move(packet));

                auto node_session = connection->get_ptr<NodeSession>();
                node_session->info().node_name = std::move(msg->target_node);
                }, asio::detached);
        }

        // 把正处于握手状态的需要send的所有包放到队列里，在握手完成时统一发包
        // 使用一个全局队列，因为这种连接排队的包很少，直接扫描就行
        send_queue_.emplace_back(std::move(msg));

        co_return;
    }

private:
    NodeSession& CreateNode(const NodeUniqueName& target_node_name, net::TcpConnectionShared&& connection, bool active) {
        // 如果存在则需要插入失败
        auto res = nodes_.emplace(target_node_name, connection);
        if (res.second) {
            auto node_session = connection->get_ptr<NodeSession>();
            node_session->info().node_name = target_node_name;
            LOG_INFO("CreateNode: {}", target_node_name);
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
                res.first->second = connection;
                LOG_INFO("Duplicate connection, disconnect old connection: {}", target_node_name);
            }
            else {
                // 断开新连接
                connection->Close();
                LOG_INFO("Duplicate connection, disconnect new connection: {}", target_node_name);
            }
        }
        return *res.first->second->get_ptr<NodeSession>();
    }

    struct EndPointRes {
        std::string ip;
        std::string port;
    };
    NodeSession* FindNode(NodeUniqueName node_name, EndPointRes* end_point) {
        auto iter = nodes_.find(node_name);
        if (iter != nodes_.end()) {
            return iter->second->get_ptr<NodeSession>();
        }

        // 尝试从配置文件中查找此节点
        const auto& config = imillion_->YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            LOG_ERR("cannot find 'cluster'.");
            return nullptr;
        }

        const auto& nodes = cluster_config["nodes"];
        if (!nodes) {
            LOG_ERR("cannot find 'cluster.nodes'.");
            return nullptr;
        }

        for (auto node : nodes) {
            auto node_name = node.first.as<std::string>();
            if (node_name != node_name) continue;

            end_point->ip = node.second["ip"].as<std::string>();
            end_point->port = node.second["port"].as<std::string>();

            break;
        }
        return nullptr;
    }

    void ForwardPacket(std::unique_ptr<ClusterSendPacketMsg> msg, NodeSession* node_session) {
        // 追加集群头部
        Ss::Cluster::ClusterMsg cluster_msg;
        auto* header = cluster_msg.mutable_forward_header();
        header->set_src_service(std::move(msg->src_service));
        header->set_target_service(std::move(msg->target_service));
        auto packet = ProtoMsgToPacket(cluster_msg);
        auto span = net::PacketSpan(packet.begin(), packet.end());
        auto size = packet.size() + msg->packet.size();
        node_session->Send(std::move(packet), span, size);

        span = net::PacketSpan(msg->packet.begin(), msg->packet.end());
        node_session->Send(std::move(msg->packet), span, 0);
    }

private:
    ClusterServer server_;
    NodeUniqueName node_name_;
    std::unordered_map<NodeUniqueName, net::TcpConnectionShared> nodes_;

    std::set<std::string> wait_nodes_;
    std::list<std::unique_ptr<ClusterSendPacketMsg>> send_queue_;
};

} // namespace gateway
} // namespace million
