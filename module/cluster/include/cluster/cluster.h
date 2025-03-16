#pragma once

#include <cassert>

#include <utility>

#include <million/imillion.h>
#include <million/msg.h>
#include <million/net/packet.h>

#include <cluster/api.h>

namespace million {
namespace cluster {

#define MILLION_CLUSTER_SERVICE_NAME "ClusterService"

using NodeName = std::string;

MILLION_MSG_DEFINE_NONCOPYABLE(MILLION_CLUSTER_API, ClusterSendMsg, (ServiceName) src_service, (NodeName) target_node, (ServiceName) target_service, (ProtoMsgUnique) msg)

// Cluster.Call返回一个Task<ProtoMsgUnique>，通过co_return 将proto_msg返回回来
// Cluster.Call内部：
// msg = co_await service.Call<ClusterPacketMsg, ClusterSendMsg>(Cluster.Service, proto_msg);
// co_return ProtoMsgUnique转换为GoogleProtoMsgUnique;
// 
// Cluster服务OnMsg，接收到ClusterProtoMsg，将proto_msg序列化，头部再带session_id、src_unique_name、target_unique_name，发送给其他集群
// Cluster服务OnMsg，接收到ClusterNodeMsg，解析拿到session_id、src_unique_name、target_unique_name，以及反序列化google_proto_msg，打包为ClusterProtoMsg并发送给src_unique_name服务
// 即Cluster.Call的co_await等到结果

// 其他集群收到包，向Cluster服务发送ClusterRecvMsg

// 1. Cluster.Call
	// 1. 参数必须是net::Packet，通过ProtoMgr进行编码
	// 2. 集群服务通过tcp发送ClusterPacket，将net::Packet带出去
	// 3. 目标节点收包，反序列化为ClusterPacket
	// 4. 目标节点通过ProtoMgr解析net::Packet，分发proto消息
	// 5. 目标处理完成，再通过ProtoMgr进行编码，调用Cluster.Reply，发回给当前节点
	// 6. 当前节点收包，返回net::Packet
// 2. 优化点
	// ClusterPacket可以不通过proto，而是直接组包send，避免多次拷贝

//class Cluster {
//public:
//
//	// node name, src_unique_name, target_unique_name, 
//
//	Task<ProtoMsgUnique> Call(const ServiceHandle& target, ProtoMsgUnique req_msg) {
//		auto res_msg = co_await service_->Call<ClusterProtoMsg>(service_->service_handle(), req_msg);
//		co_return std::move(res_msg->proto_msg);
//	}
//
//	// IMillion* imillion;
//	ClusterService* service_;
//};

} // namespace cluster
} // namespace million
