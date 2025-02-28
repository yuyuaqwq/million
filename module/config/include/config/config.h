#include <million/imillion.h>

#include <config/api.h>

namespace million {
namespace config {

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryMsg, (const std::string) module_name, (const google::protobuf::Descriptor&) config_desc, (std::optional<ProtoMsgWeak>) config)
MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigUpdateMsg, (const google::protobuf::Descriptor&) config_desc)


static ProtoMsgShared GetConfig(const google::protobuf::Descriptor& desc, ProtoMsgWeak* weak_config) {
    auto shared_config = weak_config->lock();
    if (!shared_config) {
        // 提升失败，说明配置有更新
        // ConfigQueryMsg
        // *weak_config = ;
    }
    return shared_config;
}

} // namespace config
} // namespace million