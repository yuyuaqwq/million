#pragma once

#include <yaml-cpp/yaml.h>

#include <ss/ss_cluster.pb.h>

#include <cluster/cluster.h>

#include "cluster_server.h"

namespace million {

namespace cluster {

MILLION_MESSAGE_DEFINE(, ClusterTcpConnectionMsg, (NodeSessionShared) node_session)
MILLION_MESSAGE_DEFINE(, ClusterTcpRecvPacketMsg, (NodeSessionShared) node_session, (net::Packet) packet)

MILLION_MESSAGE_DEFINE(, ClusterPersistentNodeServiceVirtualSessionMsg, (NodeSessionShared) node_session, (ServiceId) src_service_id, (ServiceName) target_service_name);

// SessionId部分，必须是a只能等待自己节点alloc的SessionId，想等待b的消息，必须是a把sessionid发给b，b再发回这个sessionid


using NodeServiceSessionId = uint64_t;

class WaitNodeSession {
public:
    struct MsgElement {
        ServiceHandle sender;
        SessionId session_id;
        MessagePointer message;
    };

public:
    WaitNodeSession() = default;
    ~WaitNodeSession() = default;

    NodeSession* node_session() const {
        return node_session_;
    }

    void set_node_session(NodeSession* node_session) {
        node_session_ = node_session;
    }

    const std::vector<MsgElement>& send_queue() const {
        return send_queue_;
    }

    void AddMessageToQueue(ServiceHandle sender, SessionId session_id, MessagePointer message) {
        send_queue_.emplace_back(MsgElement{ 
            .sender = std::move(sender),
            .session_id = session_id,
            .message = std::move(message)
        });
    }

private:
    NodeSession* node_session_ = nullptr;
    std::vector<MsgElement> send_queue_;
};


class ClusterService : public IService {
    MILLION_SERVICE_DEFINE(ClusterService);

public:
    using Base = IService;
    ClusterService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        imillion().SetServiceName(service_handle(), kClusterServiceName);

        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<ClusterTcpConnectionMsg>(service_handle(), std::move(std::static_pointer_cast<NodeSession>(connection)));
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<ClusterTcpRecvPacketMsg>(service_handle(), std::move(std::static_pointer_cast<NodeSession>(connection)), std::move(packet));
            co_return;
        });

        const auto& settings = imillion().YamlSettings();
        const auto& cluster_settings = settings["cluster"];
        if (!cluster_settings) {
            logger().LOG_ERROR("cannot find 'cluster'.");
            return false;
        }

        const auto& port_settings = cluster_settings["port"];
        if (!port_settings) {
            logger().LOG_ERROR("cannot find 'cluster.port'.");
            return false;
        }
        server_.Start(port_settings.as<uint16_t>());

        // 加载服务配置
        LoadServicesFromConfig(cluster_settings);

        return true;
    }

    // 服务与服务之间的虚拟连接会话的处理协程
    MILLION_MESSAGE_HANDLE(const ClusterPersistentNodeServiceVirtualSessionMsg, msg) {
        auto& node_session = *msg->node_session;
        auto src_service_id = msg->src_service_id;
        // auto target_service_id = msg->target_service_id;
        auto target_service_name = msg->target_service_name;
        do {
            // 这里未来可以修改超时时间，来自动回收协程
            // 收到本节点其他服务的消息，转发给其他节点
            auto recv_msg = co_await RecvWithTimeout(session_id, ::million::kSessionNeverTimeout);
            if (recv_msg.IsProtoMessage()) {
                logger().LOG_TRACE("Cluster Recv ProtoMessage: {}.", session_id);

                auto proto_msg = std::move(recv_msg.GetProtoMessage());
                PacketForward(&node_session, src_service_id
                    , target_service_name, session_id, *proto_msg);
            }
            else if (recv_msg.IsType<ClusterPersistentNodeServiceVirtualSessionMsg>()) {
                break;
            }
        } while (true);
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(ClusterTcpConnectionMsg, msg) {
        auto& node_session = *msg->node_session;

        auto& ep = node_session.remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());
        if (node_session.Connected()) {
            logger().LOG_DEBUG("Connection establishment: ip: {}, port: {}", ip, port);
        }
        else {
            logger().LOG_DEBUG("Disconnection: ip: {}, port: {}, cur_node: {}, target_node : {}", ip, port, imillion().node_id(), msg->node_session->node_id());
            DeleteNodeSession(std::move(msg->node_session));

            // todo：wait node session的清理
        }

        // 可能是主动发起，也可能是被动收到连接的，连接去重不在这里处理
        // 连接投递到定时任务，比如10s还没有握手成功就断开连接
        
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(ClusterTcpRecvPacketMsg, msg) {
        auto& node_session = *msg->node_session;

        auto& ep = node_session.remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        uint32_t header_size = 0;
        if (msg->packet.size() < sizeof(header_size)) {
            logger().LOG_WARN("Invalid header size: ip: {}, port: {}", ip, port);
            node_session.Close();
            co_return nullptr;
        }
        header_size = *reinterpret_cast<const uint32_t*>(msg->packet.data());
        header_size = asio::detail::socket_ops::network_to_host_long(header_size);

        ss::cluster::MsgBody msg_body;
        if (!msg_body.ParseFromArray(msg->packet.data() + sizeof(header_size), header_size)) {
            logger().LOG_WARN("Invalid ClusterMsg: ip: {}, port: {}", ip, port);
            node_session.Close();
            co_return nullptr;
        }

        switch (msg_body.body_case()) {
        case ss::cluster::MsgBody::BodyCase::kHandshakeReq: {
            // 收到握手请求，当前是被动连接方
            auto& req = msg_body.handshake_req();
            node_session.set_node_id(req.src_node_id());

            // 还需要检查是否与目标节点存在连接，如果已存在，说明该连接已经失效，需要断开
            auto old_node_session = FindNodeSession(req.src_node_id());
            if (old_node_session) {
                old_node_session->Close();
                logger().LOG_INFO("old connection, ip: {}, port: {}, cur_node: {}, req_src_node: {}", ip, port, imillion().node_id(), req.src_node_id());
            }

            // 能否匹配都回包
            ss::cluster::MsgBody msg_body;
            auto* res = msg_body.mutable_handshake_res();
            res->set_target_node_id(imillion().node_id());

            auto packet = ProtoMsgToPacket(msg_body);
            PacketInit(&node_session, packet, net::Packet());
            msg->node_session->Send(std::move(packet), net::PacketSpan(packet), 0);

            // 创建节点
            CreateNodeSession(std::move(msg->node_session), false);
            break;
        }
        case ss::cluster::MsgBody::BodyCase::kHandshakeRes: {
            // 收到握手响应，当前是主动连接方
            auto& res = msg_body.handshake_res();

            // 创建节点
            msg->node_session->set_node_id(res.target_node_id());
            auto& node_session = CreateNodeSession(std::move(msg->node_session), true);

            // 完成连接，发送相关队列包
            decltype(wait_node_session_map_)::iterator wait_node_session_iter;
            {
                auto lock = std::lock_guard(wait_node_session_map_mutex_);
                for (wait_node_session_iter = wait_node_session_map_.begin(); wait_node_session_iter != wait_node_session_map_.end(); ) {
                    if (wait_node_session_iter->second.node_session() == &node_session) {
                        break;
                    }
                }
                assert(wait_node_session_iter == wait_node_session_map_.end());
            }

            for (auto iter = wait_node_session_iter->second.send_queue().begin(); iter != wait_node_session_iter->second.send_queue().end(); ) {
                auto msg = iter->message.GetMutableMessage<ClusterSend>();
                auto sender_lock = sender.lock();
                auto sender_ptr = sender.get_ptr(sender_lock);
                if (!sender_ptr) {
                    continue;
                }
                
                PacketForward(&node_session, kServiceInvalidId.
                    , std::move(msg->target_service_name), iter->session_id, *msg->msg);
            }
            
            {
                auto lock = std::lock_guard(wait_node_session_map_mutex_);
                wait_node_session_map_.erase(wait_node_session_iter);
            }
            break;
        }
        case ss::cluster::MsgBody::BodyCase::kForwardHeader: {
            // 还需要判断下，连接没有完成握手，则不允许转发包

            auto& header = msg_body.forward_header();
            // auto src_service_id = header.src_service_id();
            // auto target_service_id = header.target_service_id();
            auto target_service_handle = imillion().FindServiceByName(header.target_service_name());
            if (!target_service_handle) {
                // todo: 找不到时，有可能是当前节点重启了等情况导致的
                // 需要返回一个消息让源节点知道这个target_service_id是无效的
                // 这样源节点可以重新查询服务id并重试

                logger().LOG_WARN("The target service does not exist, ep:{}:{}, src_service_id:{}, target_service_id:{}",
                    ip, port, src_service_id, target_service_id);
                break;
            }

            auto span = net::PacketSpan(
                msg->packet.begin() + sizeof(header_size) + header_size,
                msg->packet.end());

            // 基于持久会话，建立两端的虚拟服务会话
            auto service_session_id = node_session.FindServiceVirutalSession(src_service_id, target_service_id);
            if (!service_session_id) {
                service_session_id = Send<ClusterPersistentNodeServiceVirtualSessionMsg>(service_handle()
                    , msg->node_session, src_service_id, target_service_id);
                node_session.CreateServiceVirutalSession(src_service_id, target_service_id, service_session_id.value());
            }
            // 将消息转发给本机节点的其他服务
            auto res = imillion().proto_mgr().codec().DecodeMessage(span);
            if (!res) {
                break;
            }
            imillion().SendTo(service_handle(), *target_service_handle,
                *service_session_id, std::move(res->msg));
            break;
        }
        }
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(ClusterSend, msg) {
        auto sender_lock = sender.lock();
        auto sender_ptr = sender.get_ptr(sender_lock);
        if (!sender_ptr) {
            co_return nullptr;
        }

        // 首先尝试通过服务名找到对应的节点会话
        auto node_session = FindNodeSessionByServiceName(msg->target_service_name);
        if (node_session) {
            // node_session->FindServiceVirutalSession();

            // 通过服务名查询
            // msg->target_service_name;
            PacketForward(node_session, sender_ptr->service_id(), msg->target_service_name, session_id, *msg->msg);
            co_return nullptr;
        }

        // 如果没有找到已连接的会话，查找服务端点配置
        auto endpoint_opt = FindServiceEndpoint(msg->target_service_name);
        if (!endpoint_opt) {
            logger().LOG_ERROR("Service cannot be found: {}.", msg->target_service_name);
            co_return nullptr;
        }

        auto endpoint_str = std::format("{}:{}", endpoint_opt->ip, endpoint_opt->port);
        auto wait_node_session_map_lock = std::lock_guard(wait_node_session_map_mutex_);
        auto res = wait_node_session_map_.emplace(endpoint_str, WaitNodeSession());
        if (res.second) {
            auto& io_context = imillion().NextIoContext();
            asio::co_spawn(io_context.get_executor(), [this, target_service = msg->target_service_name,
                wait_node_iter = res.first, &endpoint = *endpoint_opt]() mutable -> asio::awaitable<void>
            {
                auto connection = co_await server_.ConnectTo(endpoint.ip, endpoint.port);
                if (!connection) {
                    logger().LOG_ERROR("server_.ConnectTo failed for service {} at {}:{}.", target_service, endpoint.ip, endpoint.port);
                    
                    {
                        auto lock = std::lock_guard(wait_node_session_map_mutex_);
                        wait_node_session_map_.erase(wait_node_iter);
                    }
                    co_return;
                }

                // 发起握手请求
                auto node_session = std::move(std::static_pointer_cast<NodeSession>(*connection));

                auto res = service_name_to_node_session_.emplace(target_service, node_session);
                assert(res.second);
                wait_node_iter->second.set_node_session(node_session.get());

                // 与目标服务的连接未开始
                ss::cluster::MsgBody msg_body;
                auto* req = msg_body.mutable_handshake_req();
                req->set_src_node_id(imillion().node_id());

                auto packet = ProtoMsgToPacket(msg_body);
                PacketInit(node_session.get(), packet, net::Packet());
                node_session->Send(std::move(packet), net::PacketSpan(packet), 0);
            }, asio::detached);
        }
        // 把正处于握手状态的需要send的所有包放到队列里，在握手完成时统一发包
        res.first->second.AddMessageToQueue(std::move(sender), session_id, std::move(msg_));

        co_return nullptr;
    }

private:
    void LoadServicesFromConfig(const YAML::Node& cluster_settings) {
        const auto& services_settings = cluster_settings["services"];
        if (!services_settings) {
            logger().LOG_WARN("cannot find 'cluster.services', service-based calling will not be available.");
            return;
        }

        for (const auto& service_node : services_settings) {
            for (const auto& service_entry : service_node) {
                std::string service_name = service_entry.first.as<std::string>();
                const auto& endpoints = service_entry.second["endpoints"];
                
                if (endpoints && endpoints.IsSequence()) {
                    for (const auto& endpoint : endpoints) {
                        std::string endpoint_str = endpoint.as<std::string>();
                        size_t colon_pos = endpoint_str.find(':');
                        if (colon_pos != std::string::npos) {
                            std::string ip = endpoint_str.substr(0, colon_pos);
                            std::string port = endpoint_str.substr(colon_pos + 1);
                            
                            logger().LOG_DEBUG("Registered service '{}' at {}:{}", service_name, ip, port);
                            service_endpoints_[service_name].emplace_back(EndPoint{ std::move(ip), std::move(port) });
                        }
                    }
                }
            }
        }
    }

    NodeSession& CreateNodeSession(NodeSessionShared&& session, bool active) {
        auto target_node_id = session->node_id();
        auto res = nodes_.emplace(target_node_id, session);
        if (res.second) {
            logger().LOG_INFO("CreateNode: {}", target_node_id);
        }
        else {
            // 已存在，比较两边节点id，选择断开某一条连接
            bool disconnect_old;
            if (active) {
                // 主动建立
                disconnect_old = imillion().node_id() > target_node_id;
            }
            else {
                // 被动建立
                disconnect_old = target_node_id > imillion().node_id();
            }

            if (disconnect_old) {
                // 断开旧连接，关联新连接
                res.first->second->Close();
                res.first->second = std::move(session);
                logger().LOG_INFO("Duplicate connection, disconnect old connection: {}", target_node_id);
            }
            else {
                // 断开新连接
                session->Close();
                logger().LOG_INFO("Duplicate connection, disconnect new connection: {}", target_node_id);
            }
        }
        return *res.first->second;
    }

    NodeSession* FindNodeSession(NodeId node_id) {
        auto iter = nodes_.find(node_id);
        if (iter != nodes_.end()) {
            return iter->second.get();
        }
        return nullptr;
    }

    NodeSession* FindNodeSessionByServiceName(const ServiceName& service_name) {
        auto iter = service_name_to_node_session_.find(service_name);
        if (iter != service_name_to_node_session_.end()) {
            return iter->second;
        }
        return nullptr;
    }

    void DeleteNodeSession(NodeSessionShared&& node_session) {
        nodes_.erase(node_session->node_id());
    }

    struct EndPoint {
        std::string ip;
        std::string port;
    };

    EndPoint* FindServiceEndpoint(ServiceName service_name) {
        auto it = service_endpoints_.find(service_name);
        if (it == service_endpoints_.end() || it->second.empty()) {
            return nullptr;
        }
        
        // 简单实现：返回第一个可用的端点
        // 未来可以添加负载均衡策略
        return &it->second[0];
    }


    void PacketInit(NodeSession* node_session, const net::Packet& header_packet, const net::Packet& forward_packet) {
        uint32_t header_size = header_packet.size();
        header_size = asio::detail::socket_ops::host_to_network_long(header_size);

        auto size_packet = net::Packet(sizeof(header_size));
        std::memcpy(size_packet.data(), &header_size, sizeof(header_size));

        auto size_span = net::PacketSpan(size_packet);
        auto total_size = sizeof(header_size) + header_packet.size() + forward_packet.size();

        node_session->Send(std::move(size_packet), size_span, total_size);
    }

    void PacketForward(NodeSession* node_session, ServiceId src_service_id, const ServiceName& target_service_name, SessionId session_id, const ProtoMessage& msg) {
        auto packet_opt = imillion().proto_mgr().codec().EncodeMessage(msg);
        if (!packet_opt) {
            return;
        }
        auto packet = std::move(*packet_opt);

        // 追加集群头部
        ss::cluster::MsgBody msg_body;
        auto* header = msg_body.mutable_forward_header();
        //header->set_src_service_id(src_service_id);
        //header->set_target_service_id(target_service_id);
        header->set_target_service_name(target_service_name);
        header->set_session_id(session_id);
        auto header_packet = ProtoMsgToPacket(msg_body);
        
        PacketInit(node_session, header_packet, packet);

        auto header_span = net::PacketSpan(header_packet);
        node_session->Send(std::move(header_packet), header_span, 0);

        auto span = net::PacketSpan(packet);
        node_session->Send(std::move(packet), span, 0);
    }

private:
    ClusterServer server_;

    std::unordered_map<ServiceName, std::string> direct_service_registry_;
    std::unordered_map<ServiceName, std::vector<EndPoint>> service_endpoints_;

    std::unordered_map<NodeId, NodeSessionShared> nodes_;

    std::unordered_map<ServiceName, NodeSession*> service_name_to_node_session_;

    std::unordered_map<std::string, WaitNodeSession> wait_node_session_map_;
    std::mutex wait_node_session_map_mutex_;

};

} // namespace cluster

} // namespace million