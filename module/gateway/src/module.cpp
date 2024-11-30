#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include "gateway_service.h"

namespace million {
namespace gateway {

MILLION_FUNC_API bool MillionModuleInit(IMillion* imillion) {
    g_agent_proto_codec = new ProtoCodec();
    g_agent_logic_handle_map = new std::unordered_map<MsgKey, MsgLogicHandleFunc>();

    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<GatewayService>();
    if (!handle) {
        return false;
    }
    imillion->SetServiceName(*handle, "GatewayService");
    return true;
}

} // namespace gateway
} // namespace million