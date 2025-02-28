#include <million/imillion.h>

#include <config/api.h>

namespace million {
namespace config {

using ConfigWeak = ProtoMsgWeak;
using ConfigShared = ProtoMsgShared;

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryMsg, (const std::string) module_name, (const google::protobuf::Descriptor&) config_desc, (std::optional<ConfigWeak>) config)
MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigUpdateMsg, (const google::protobuf::Descriptor&) config_desc)

static ConfigShared GetConfig(const google::protobuf::Descriptor& desc, ConfigWeak* config_weak) {
    auto config_shared = config_weak->lock();
    if (!config_shared) {
        // 提升失败，说明配置有更新
        // ConfigQueryMsg
        // *config_weak = ;
    }
    return config_shared;
}

} // namespace config
} // namespace million