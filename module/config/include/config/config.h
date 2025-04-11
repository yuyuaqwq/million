#include <million/imillion.h>

#include <config/api.h>

namespace million {
namespace config {

constexpr const char* kConfigServiceName = "ConfigService";

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigQueryResp, (std::optional<ProtoMsgWeak>) config)

MILLION_MSG_DEFINE(MILLION_CONFIG_API, ConfigUpdateReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MSG_DEFINE_EMPTY(MILLION_CONFIG_API, ConfigUpdateResp)


template <typename ConfigTableMsgT, typename ConfigMsgT>
class ConfigWeak : public noncopyable {
public:
    using ConfigTableMsgType = ConfigTableMsgT;
    using ConfigMsgType = ConfigMsgT;

public:
    ConfigWeak() = default;
    ~ConfigWeak() = default;

    explicit ConfigWeak(ProtoMsgWeak&& msg_weak)
        : weak_(std::move(msg_weak)) {}

    ConfigWeak(ConfigWeak&& other)
        : weak_(std::move(other.weak_)) {}

    void operator=(ConfigWeak&& other) {
        weak_ = std::move(other.weak_);
    }

    auto lock() const {
        return weak_.lock();
    }

private:
    ProtoMsgWeak weak_;
};

template <typename ConfigTableMsgT, typename ConfigMsgT>
static Task<ConfigWeak<ConfigTableMsgT, ConfigMsgT>> QueryConfig(IService* this_service, ServiceHandle config_service) {
    auto desc = ConfigMsgT::GetDescriptor();
    if (!desc) {
        TaskAbort("Unable to obtain descriptor: {}.", typeid(ConfigMsgT).name());
    }

    auto msg = co_await this_service->Call<ConfigQueryReq, ConfigQueryResp>(config_service, *desc);
    if (!msg->config) {
        TaskAbort("Query config failed: {}.", desc->full_name());
    }
    co_return ConfigWeak<ConfigTableMsgT, ConfigMsgT>(std::move(*msg->config));
}

template <typename ConfigTableMsgT, typename ConfigMsgT>
class ConfigShared : public noncopyable {
public:
    explicit ConfigShared(ProtoMsgShared&& msg_shared)
        : shared_(std::move(msg_shared)) {}

    const ConfigTableMsgT* operator->() const {
        return static_cast<const ConfigTableMsgT*>(shared_.get());
    }

    ConfigShared(ConfigShared&& other)
        : shared_(std::move(other.shared_)) {}

    void operator=(ConfigShared&& other) {
        shared_ = std::move(other.shared_);
    }

private:
    ProtoMsgShared shared_;
};

template <typename ConfigWeakT, typename ConfigTableMsgT = ConfigWeakT::ConfigTableMsgType, typename ConfigMsgT = ConfigWeakT::ConfigMsgType>
static Task<ConfigShared<ConfigTableMsgT, ConfigMsgT>> MakeConfigLock(IService* this_service, ServiceHandle config_service, ConfigWeakT* config_weak) {
    auto config_lock = config_weak->lock();
    while (!config_lock) {
        // 提升失败，说明配置有更新，重新拉取
        auto weak = co_await QueryConfig<ConfigTableMsgT, ConfigMsgT>(this_service, config_service);
        config_lock = weak.lock();
    }
    co_return ConfigShared<ConfigTableMsgT, ConfigMsgT>(std::move(config_lock));
}

} // namespace config
} // namespace million