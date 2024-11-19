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

MILLION_MSG_DEFINE(CLUSTER_CLASS_API, ClusterProtoMsg, (ProtoMsgUnique)proto_msg)
//// MILLION_MSG_DEFINE(GATEWAY_CLASS_API, UnRegisterServiceMsg, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
//MILLION_MSG_DEFINE(GATEWAY_CLASS_API, RecvProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)
//MILLION_MSG_DEFINE(GATEWAY_CLASS_API, SendProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)


// Cluster.Call����һ��Task<ProtoMsgUnique>��ͨ��co_return ��proto_msg���ػ���
// Cluster.Call�ڲ���
// msg = co_await service.Call<ClusterPacketMsg, ClusterSendMsg>(Cluster.Service, proto_msg);
// co_return ProtoMsgUniqueת��ΪGoogleProtoMsgUnique;
// 
// Cluster����OnMsg�����յ�ClusterProtoMsg����proto_msg���л���ͷ���ٴ�session_id��src_unique_name��target_unique_name�����͸�������Ⱥ
// Cluster����OnMsg�����յ�ClusterNodeMsg�������õ�session_id��src_unique_name��target_unique_name���Լ������л�google_proto_msg�����ΪClusterProtoMsg�����͸�src_unique_name����
// ��Cluster.Call��co_await�ȵ����

// ������Ⱥ�յ�������Cluster������ClusterRecvMsg

// 1. Cluster.Call
	// 1. ����������net::Packet��ͨ��ProtoMgr���б���
	// 2. tcp����ClusterPacket
	// 3. Ŀ��ڵ��հ��������л�ΪClusterPacket
	// 4. Ŀ��ڵ�ͨ��ProtoMgr����net::Packet���ַ�proto��Ϣ
	// 5. Ŀ�괦����ɣ�����Cluster.Reply����ClusterPacket���ظ���ǰ�ڵ�
	// 6. ��ǰ�ڵ��հ�������net::Packet

class Cluster {
public:

	// node name, src_unique_name, target_unique_name, 

	Task<ProtoMsgUnique> Call(const ServiceHandle& target, ProtoMsgUnique req_msg) {
		auto res_msg = co_await service_->Call<ClusterProtoMsg>(service_->service_handle(), req_msg);
		co_return std::move(res_msg->proto_msg);
	}

	// IMillion* imillion;
	ClusterService* service_;
};

} // namespace cluster
} // namespace million
