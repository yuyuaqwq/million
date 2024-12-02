#include "gateway_service.h"

namespace million {
namespace gateway {

GATEWAY_OBJECT_API ProtoCodec* g_agent_proto_codec = nullptr;
GATEWAY_OBJECT_API std::unordered_map<MsgKey, MsgLogicHandleFunc>* g_agent_logic_handle_map = nullptr;
GATEWAY_OBJECT_API std::vector<std::function<void()>>* g_agent_logic_init = nullptr;

} // namespace gateway
} // namespace million