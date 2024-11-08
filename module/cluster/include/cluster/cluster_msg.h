#pragma once

#include <cassert>

#include <utility>

#include <protogen/cs/cs_msgid.pb.h>

#include <million/imillion.h>
#include <million/proto_msg.h>

#include <cluster/api.h>

#include "cluster_service.h"

// #include <gateway/user_session_handle.h>

namespace million {
namespace cluster {

MILLION_MSG_DEFINE(CLUSTER_CLASS_API, ClusterProtoMsg, (ProtoMsgUnique) proto_msg)
//// MILLION_MSG_DEFINE(GATEWAY_CLASS_API, UnRegisterServiceMsg, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
//MILLION_MSG_DEFINE(GATEWAY_CLASS_API, RecvProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)
//MILLION_MSG_DEFINE(GATEWAY_CLASS_API, SendProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)


// Task支持定义返回值，Cluster.Call返回一个Task<ProtoMsgUnique>，通过co_return 将proto_msg返回回来
// Cluster.Call内部：
// msg = co_await service.Call<ClusterPacketMsg, ClusterSendMsg>(Cluster.Service, proto_msg);
// co_return ProtoMsgUnique转换为GoogleProtoMsgUnique;
// 
// Cluster服务OnMsg，接收到ClusterProtoMsg，将proto_msg序列化，头部再带session_id、src_unique_name、target_unique_name，发送给其他集群
// Cluster服务OnMsg，接收到ClusterNodeMsg，解析拿到session_id、src_unique_name、target_unique_name，以及反序列化google_proto_msg，打包为ClusterProtoMsg并发送给src_unique_name服务
// 即Cluster.Call的co_await等到结果

// 其他集群收到包，向Cluster服务发送ClusterRecvMsg

class Cluster {
public:
	Task<ProtoMsgUnique> Call(const ServiceHandle& target, ProtoMsgUnique req_msg) {
		auto res_msg = co_await service_->Call<ClusterProtoMsg>(service_->service_handle(), req_msg);
		co_return std::move(res_msg->proto_msg);
	}

	// IMillion* imillion;
	ClusterService* service_;
};

} // namespace gateway
} // namespace million
