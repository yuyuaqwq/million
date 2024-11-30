#include "gateway_service.h"

namespace million {
namespace gateway {

GATEWAY_OBJECT_API ProtoCodec* g_proto_codec = nullptr;
GATEWAY_OBJECT_API std::unordered_map<MsgKey, MsgLogicHandleFunc>* g_logic_handle_map = nullptr;

void AgentSend(AgentService* agent, const protobuf::Message& proto_msg) {
    auto res = g_proto_codec->EncodeMessage(proto_msg);
    if (!res) {
        MILLION_LOGGER_CALL(agent->imillion_, agent->service_handle(), ::million::logger::LogLevel::kErr, "EncodeMessage failed: type:{}.", typeid(proto_msg).name());
        return;
    }
    agent->Send<GatewaySendPacketMsg>(agent->gateway_, agent->gateway_context_id_, *res);
}

} // namespace gateway
} // namespace million