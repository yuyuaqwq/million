#include <config/config.h>

namespace million {
namespace config {

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


} // namespace db
} // namespace million