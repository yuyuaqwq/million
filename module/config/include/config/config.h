#include <million/imillion.h>

#include <config/api.h>

namespace million {
namespace config {

constexpr const char* kConfigServiceName = "ConfigService";

using ConfigWeak = ProtoMsgWeak;
using ConfigShared = ProtoMsgShared;

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryMsg, (const google::protobuf::Descriptor&) config_desc, (std::optional<ConfigWeak>) config)
MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigUpdateMsg, (const google::protobuf::Descriptor&) config_desc)


template <typename ConfigMsgT>
class ConfigLock {
public:
    explicit ConfigLock(ConfigShared&& config_shared)
        : config_shared_(std::move(config_shared)) {}

    const ConfigMsgT* operator->() const {
        return static_cast<const ConfigMsgT*>(config_shared_.get());
    }

private:
    ConfigShared config_shared_;
};

template <typename ConfigMsgT>
static Task<ConfigLock<ConfigMsgT>> MakeConfigLock(ServiceHandle config_service, ConfigWeak* config_weak) {
    auto config_lock = config_weak->lock();
    while (!config_lock) {
        // 提升失败，说明配置有更新，重新拉取
        auto service_lock = config_service.lock();
        auto iservice = config_service.get_ptr(service_lock);

        auto desc = ConfigMsgT::GetDescriptor();
        if (!desc) {
            TaskAbort("unable to obtain descriptor: {}.", typeid(ConfigMsgT).name());
        }

        auto msg = co_await iservice->Call<ConfigQueryMsg>(config_service, *desc, std::nullopt);
        if (!msg->config) {
            TaskAbort("query config failed: {}.", desc->full_name());
        }
        config_lock = msg->config->lock();
    }
    co_return ConfigLock<ConfigMsgT>(std::move(config_lock));
}

} // namespace config
} // namespace million