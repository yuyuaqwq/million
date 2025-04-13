#include <million/imillion.h>

#include <config/api.h>
#include <config/config_table.h>

namespace million {
namespace config {

constexpr const char* kConfigServiceName = "ConfigService";

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryResp, (std::optional<ProtoMsgWeak>) config)

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigUpdateReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MSG_DEFINE_EMPTY(MILLION_CONFIG_API, ConfigUpdateResp)

template <typename ConfigMsgT>
class ConfigTableWeak : public noncopyable {
public:
    ConfigTableWeak() = default;
    ~ConfigTableWeak() = default;

    explicit ConfigTableWeak(ProtoMsgWeak&& msg_weak)
        : weak_(std::move(msg_weak)) {}

    ConfigTableWeak(ConfigTableWeak&& other) noexcept
        : weak_(std::move(other.weak_)) {}

    void operator=(ConfigTableWeak&& other) noexcept {
        weak_ = std::move(other.weak_);
    }

    Task<ConfigTable<ConfigMsgT>> Lock(IService* this_service, ServiceHandle config_service) const {
        auto config_lock = weak_.lock();
        while (!config_lock) {
            // 提升失败，说明配置有更新，重新拉取
            auto new_weak = co_await QueryConfig<ConfigMsgT>(this_service, config_service);
            config_lock = new_weak.weak_.lock();
        }
        co_return ConfigTable<ConfigMsgT>(std::move(config_lock));
    }

private:
    ProtoMsgWeak weak_;
};

template <typename ConfigMsgT>
static Task<ConfigTableWeak<ConfigMsgT>> QueryConfig(IService* this_service, ServiceHandle config_service) {
    auto desc = ConfigMsgT::GetDescriptor();
    if (!desc) {
        TaskAbort("Unable to obtain descriptor: {}.", typeid(ConfigMsgT).name());
    }

    auto msg = co_await this_service->Call<ConfigQueryReq, ConfigQueryResp>(config_service, *desc);
    if (!msg->config) {
        TaskAbort("Query config failed: {}.", desc->full_name());
    }
    co_return ConfigTableWeak<ConfigMsgT>(std::move(*msg->config));
}

} // namespace config
} // namespace million