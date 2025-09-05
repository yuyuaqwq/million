#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>

#include <cluster/cluster.h>

namespace million {

namespace cluster {

class NodeSession : public net::TcpConnection {
public:
    using Base = net::TcpConnection;
    using Base::Base;

    NodeId node_id() const { return node_id_; }
    void set_node_id(NodeId node_id) { node_id_ = node_id; }

    const std::queue<MessageElementWithWeakSender>& message_queue() const { return message_queue_; }
    std::queue<MessageElementWithWeakSender>& message_queue() { return message_queue_; }

    bool CreateServiceVirutalSession(ServiceId src_service_id, ServiceId target_service_id, SessionId service_virtual_session_id) {
        auto res = service_virtual_session_map_.emplace(ServiceIdPair{ src_service_id, target_service_id }, service_virtual_session_id);
        return res.second;
    }

    std::optional<SessionId> FindServiceVirutalSession(ServiceId src_service_id, ServiceId target_service_id) {
        auto iter = service_virtual_session_map_.find(ServiceIdPair{ src_service_id, target_service_id });
        if (iter == service_virtual_session_map_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

    void AddTargetServiceId(ServiceName target_service_name, ServiceId target_service_id) {
        auto res = target_service_name_map_.emplace(std::move(target_service_name), target_service_id);
        assert(res.second);
    }

    std::optional<ServiceId> FindTargetServiceIdByName(const ServiceName& target_service_name) {
        auto iter = target_service_name_map_.find(target_service_name);
        if (iter == target_service_name_map_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

private:
    // 定义键类型
    struct ServiceIdPair {
        ServiceId first;
        ServiceId second;

        // 重载相等运算符
        bool operator==(const ServiceIdPair& other) const {
            return first == other.first && second == other.second;
        }
    };

    // 自定义哈希函数
    struct ServiceIdPairHash {
        std::size_t operator()(const ServiceIdPair& key) const {
            // 使用 std::hash 计算每个 uint64_t 的哈希值，然后组合它们
            std::size_t h1 = std::hash<ServiceId>{}(key.first);
            std::size_t h2 = std::hash<ServiceId>{}(key.second);

            // 常用的哈希组合技术（boost::hash_combine 类似）
            return h1 ^ (h2 << 1);
        }
    };

    NodeId node_id_ = kNodeIdInvalid;
    std::unordered_map<ServiceIdPair, SessionId, ServiceIdPairHash> service_virtual_session_map_;
    std::unordered_map<ServiceName, SessionId> target_service_name_map_;

    std::queue<MessageElementWithWeakSender> message_queue_;
};

} // namespace cluster

} // namespace million
