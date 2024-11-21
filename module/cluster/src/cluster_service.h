#pragma once

#include <million/iservice.h>

#include <cluster/cluster_msg.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

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
        server_.Start(8002);

        return true;
    }

    MILLION_MSG_DISPATCH(ClusterService);

    MILLION_MSG_HANDLE(ClusterTcpConnectionMsg, msg) {
        // 有可能目标节点已经主动连接当前节点
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterTcpRecvPacketMsg, msg) {
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
            asio::co_spawn(io_context.get_executor(), [this, msg = std::move(msg), end_point = std::move(end_point)]() mutable -> asio::awaitable<void> {
                auto res = co_await server_.ConnectTo(end_point.ip, end_point.port);
                if (!res) {
                    co_return;
                }
                // io线程执行，重新发这条消息
                auto imsg = msg.get();
                Resend(service_handle(), std::move(msg));
            }, asio::detached);
            co_return;
        }
        auto& connection = *connection_ptr;
        connection->Send(std::move(msg->packet));
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
        const auto& cluster_config = config["cluster_config"];
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

            end_point->ip = node["ip"].as<std::string>();
            end_point->port = node["port"].as<std::string>();

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
