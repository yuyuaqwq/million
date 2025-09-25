#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>
#include <million/cluster/cluster.h>

namespace million {

namespace cluster {

class NodeSession : public net::TcpConnection {
public:
    using Base = net::TcpConnection;
    using Base::Base;

    NodeId node_id() const { return node_id_; }
    void set_node_id(NodeId node_id) { node_id_ = node_id; }

    const std::list<MessageElementWithWeakSender>& message_queue() const { return message_queue_; }
    std::list<MessageElementWithWeakSender>& message_queue() { return message_queue_; }

private:

    NodeId node_id_ = kNodeIdInvalid;

    std::list<MessageElementWithWeakSender> message_queue_;
};

} // namespace cluster

} // namespace million
