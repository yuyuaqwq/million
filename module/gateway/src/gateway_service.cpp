#include "gateway_service.h"

namespace million {
namespace gateway {

GATEWAY_OBJECT_API ProtoCodec* g_agent_proto_codec = nullptr;
GATEWAY_OBJECT_API std::unordered_map<MsgKey, MsgLogicHandleFunc>* g_agent_logic_handle_map = nullptr;

} // namespace gateway
} // namespace million