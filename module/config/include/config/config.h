#include <million/imillion.h>

#include <config/api.h>
#include <config/config_table.h>

namespace million {
namespace config {

constexpr const char* kConfigServiceName = "ConfigService";

class MILLION_CONFIG_API ConfigTableWeakBase : public noncopyable {
public:
    ConfigTableWeakBase() = default;
    explicit ConfigTableWeakBase(std::weak_ptr<ConfigTableBase>&& weak)
        : weak_(std::move(weak)) {}
    ~ConfigTableWeakBase() = default;

    ConfigTableWeakBase(ConfigTableWeakBase&& other) noexcept
        : weak_(std::move(other.weak_)) {}

    void operator=(ConfigTableWeakBase&& other) noexcept {
        weak_ = std::move(other.weak_);
    }

    std::optional<ConfigTableShared> TryLock(const google::protobuf::Descriptor* descriptor) {
        auto config_lock = weak_.lock();
        if (!config_lock) {
            return std::nullopt;
        }
        return config_lock;
    }

    Task<ConfigTableShared> Lock(IService* this_service, const ServiceHandle& config_service, const google::protobuf::Descriptor* descriptor);

protected:
    std::weak_ptr<ConfigTableBase> weak_;
};

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MSG_DEFINE_NONCOPYABLE(MILLION_CONFIG_API, ConfigQueryResp, (const google::protobuf::Descriptor&) config_desc, (std::optional<ConfigTableWeakBase>) config)

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigUpdateReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MSG_DEFINE_EMPTY(MILLION_CONFIG_API, ConfigUpdateResp)

Task<ConfigTableShared> ConfigTableWeakBase::Lock(IService* this_service, const ServiceHandle& config_service, const google::protobuf::Descriptor* descriptor) {
    TaskAssert(descriptor, "descriptor is nullptr.");

    auto config_lock = weak_.lock();
    while (!config_lock) {
        // 提升失败，说明配置有更新，重新拉取
        auto resp = co_await this_service->Call<ConfigQueryReq, ConfigQueryResp>(config_service, *descriptor);
        if (!resp->config) {
            TaskAbort("Query config failed: {}.", descriptor->full_name());
        }
        // 更新本地weak
        weak_ = resp->config->weak_;
        config_lock = weak_.lock();
    }
    co_return config_lock;
}

template <typename ConfigMsgT>
class ConfigTableWeak : public ConfigTableWeakBase {
public:
    ConfigTableWeak() = default;
    ~ConfigTableWeak() = default;

    ConfigTableWeak(ConfigTableWeakBase&& base)
        : ConfigTableWeakBase(std::move(base)) {}

    ConfigTableWeak(ConfigTableWeak&& other) noexcept
        : ConfigTableWeakBase(std::move(other)) {}

    void operator=(ConfigTableWeak&& other) noexcept {
        ConfigTableWeakBase::operator=(std::move(other));
    }

    Task<std::shared_ptr<ConfigTable<ConfigMsgT>>> Lock(IService* this_service, const ServiceHandle& config_service) {
        auto table_base = co_await ConfigTableWeakBase::Lock(this_service, config_service, ConfigMsgT::GetDescriptor());
        co_return std::static_pointer_cast<ConfigTable<ConfigMsgT>>(std::move(table_base));
    }
};

template <typename ConfigMsgT>
static Task<ConfigTableWeak<ConfigMsgT>> QueryConfig(IService* this_service, const ServiceHandle& config_service) {
    auto descriptor = ConfigMsgT::GetDescriptor();
    if (!descriptor) {
        TaskAbort("Unable to obtain descriptor: {}.", typeid(ConfigMsgT).name());
    }

    auto msg = co_await this_service->Call<ConfigQueryReq, ConfigQueryResp>(config_service, *descriptor);
    if (!msg->config) {
        TaskAbort("Query config failed: {}.", descriptor->full_name());
    }
    co_return std::move(*msg->config);
}


} // namespace config
} // namespace million