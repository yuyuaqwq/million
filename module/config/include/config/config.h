#include <million/imillion.h>

#include <config/api.h>
#include <config/config_table.h>

namespace million {
namespace config {

constexpr const char* kConfigServiceName = "ConfigService";

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryResp, (const google::protobuf::Descriptor&) config_desc, (std::optional<ProtoMsgWeak>) config)

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigUpdateReq, (const google::protobuf::Descriptor&)config_desc)
MILLION_MSG_DEFINE_EMPTY(MILLION_CONFIG_API, ConfigUpdateResp)

class MILLION_CONFIG_API ConfigTableWeakBase : public noncopyable {
public:
    ConfigTableWeakBase() = default;
    explicit ConfigTableWeakBase(ProtoMsgWeak&& msg_weak)
        : weak_(std::move(msg_weak)) {}
    ~ConfigTableWeakBase() = default;

    ConfigTableWeakBase(ConfigTableWeakBase&& other) noexcept
        : weak_(std::move(other.weak_)) {}

    void operator=(ConfigTableWeakBase&& other) noexcept {
        weak_ = std::move(other.weak_);
    }

    std::optional<ConfigTableBase> TryLock(const google::protobuf::Descriptor* descriptor) {
        auto config_lock = weak_.lock();
        if (!config_lock) {
            return std::nullopt;
        }
        return ConfigTableBase(std::move(config_lock), descriptor);
    }

    Task<ConfigTableBase> Lock(IService* this_service, const ServiceHandle& config_service, const google::protobuf::Descriptor* descriptor) {
        TaskAssert(descriptor, "descriptor is nullptr.");
        
        auto config_lock = weak_.lock();
        while (!config_lock) {
            // 提升失败，说明配置有更新，重新拉取
            auto resp = co_await this_service->Call<ConfigQueryReq, ConfigQueryResp>(config_service, *descriptor);
            if (!resp->config) {
                TaskAbort("Query config failed: {}.", descriptor->full_name());
            }
            // 更新本地weak
            weak_ = *resp->config;

            auto new_weak = ConfigTableWeakBase(std::move(*resp->config));
            config_lock = new_weak.weak_.lock();
        }
        co_return ConfigTableBase(std::move(config_lock), descriptor);
    }

protected:
    ProtoMsgWeak weak_;
};

template <typename ConfigMsgT>
class ConfigTableWeak : public noncopyable {
public:
    ConfigTableWeak() = default;
    ~ConfigTableWeak() = default;

    explicit ConfigTableWeak(ProtoMsgWeak&& msg_weak)
        : base_(std::move(msg_weak)) {}

    ConfigTableWeak(ConfigTableWeak&& other) noexcept
        : base_(std::move(other.base_)) {}

    void operator=(ConfigTableWeak&& other) noexcept {
        base_ = std::move(other.base_);
    }

    Task<ConfigTable<ConfigMsgT>> Lock(IService* this_service, const ServiceHandle& config_service) {
        auto table_base = co_await base_.Lock(this_service, config_service, ConfigMsgT::GetDescriptor());
        co_return ConfigTable<ConfigMsgT>(std::move(table_base));
    }

private:
    ConfigTableWeakBase base_;
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
    co_return ConfigTableWeak<ConfigMsgT>(std::move(*msg->config));
}

} // namespace config
} // namespace million