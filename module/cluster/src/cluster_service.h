#pragma once

#include <million/iservice.h>

#include <protogen/ss/ss_cluster.pb.h>

#include <cluster/cluster_msg.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

namespace Ss = ::Million::Proto::Ss;

MILLION_MSG_DEFINE(, ClusterTcpConnectionMsg, (net::TcpConnectionShared)connection)
MILLION_MSG_DEFINE(, ClusterTcpRecvPacketMsg, (net::TcpConnectionShared)connection, (net::Packet)packet)

class ClusterService : public IService {
public:
    using Base = IService;
    ClusterService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) {
            Send<ClusterTcpConnectionMsg>(service_handle(), connection);
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) {
            Send<ClusterTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
        });

        const auto& config = imillion_->YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster'." << std::endl;
            return false;
        }

        const auto& name_config = cluster_config["name"];
        if (!name_config)
        {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster.name'." << std::endl;
            return false;
        }
        node_name_ = name_config.as<std::string>();

        const auto& port_config = cluster_config["port"];
        if (!port_config)
        {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster.port'." << std::endl;
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        return true;
    }

    MILLION_MSG_DISPATCH(ClusterService);

    MILLION_MSG_HANDLE(ClusterTcpConnectionMsg, msg) {
        // 有可能目标节点已经主动连接当前节点
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterTcpRecvPacketMsg, msg) {
        // 解析头部
        Ss::ClusterHeader header;
        header.ParseFromArray(msg->packet.data(), msg->packet.size());

        auto& src_service = header.src_service();
        auto& target_service = header.target_service();

        auto target_service_handle = imillion_->GetServiceByUniqueNum(target_service);
        if (target_service_handle) {
            // 还需要获取下源节点
            Send<ClusterRecvPacketMsg>(*target_service_handle, std::string(), std::move(src_service), std::move(msg->packet), net::PacketSpan(msg->packet.begin() + header.ByteSize(), msg->packet.end()));
        }
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterSendPacketMsg, msg) {
        EndPointRes end_point;
        auto connection_ptr = FindNode(msg->target_node, &end_point);
        if (!connection_ptr && end_point.ip.empty()) {
            co_return;
        }
        if (!connection_ptr) {
            auto& io_context = imillion_->NextIoContext();
            // 还需要标记该节点为连接中，连接过程中有其他发给该节点的消息需要放入队列等待
            asio::co_spawn(io_context.get_executor(), [this, msg = std::move(msg), end_point = std::move(end_point)]() mutable -> asio::awaitable<void> {
                auto res = co_await server_.ConnectTo(end_point.ip, end_point.port);
                if (!res) {
                    co_return;
                }
                // io线程执行，重新处理这条消息
                auto imsg = msg.get();
                Resend(service_handle(), std::move(msg));
            }, asio::detached);
            co_return;
        }
        auto& connection = *connection_ptr;
        // 追加集群头部
        Ss::ClusterHeader header;
        header.set_src_service(std::move(msg->src_service));
        header.set_target_service(std::move(msg->target_service));
        auto header_packet = net::Packet(header.ByteSize());
        connection->Send(std::move(header_packet), net::PacketSpan(header_packet.begin(), header_packet.end()), header_packet.size() + msg->packet.size());
        connection->Send(std::move(msg->packet), net::PacketSpan(msg->packet.begin(), msg->packet.end()), 0);
        co_return;
    }

private:
    struct EndPointRes {
        std::string ip;
        std::string port;
    };
    net::TcpConnectionShared* FindNode(NodeUniqueName node_name, EndPointRes* end_point) {
        auto iter = nodes_.find(node_name);
        if (iter != nodes_.end()) {
            return &iter->second;
        }

        // 尝试从配置文件中查找此节点
        const auto& config = imillion_->YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster'." << std::endl;
            return nullptr;
        }

        const auto& nodes = cluster_config["nodes"];
        if (!nodes) {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster.nodes'." << std::endl;
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

private:
    ClusterServer server_;
    NodeUniqueName node_name_;
    std::unordered_map<NodeUniqueName, net::TcpConnectionShared> nodes_;
};

} // namespace gateway
} // namespace million
