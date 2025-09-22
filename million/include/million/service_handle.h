#pragma once

#include <cstdint>

#include <string>
#include <list>
#include <memory>

#include <million/api.h>
#include <million/seata_snowflake.hpp>
#include <million/module_def.h>

namespace million {

using ServiceId = SnowId;
constexpr ServiceId kServiceIdInvalid = kSnowIdInvalid;

using ServiceNameId = ModuleSubCode;
constexpr ServiceNameId kServiceNameIdInvalid = kModuleSubCodeInvalid;

class IService;
class ServiceCore;
using ServiceShared = std::shared_ptr<ServiceCore>;
using ServiceWeak = std::weak_ptr<ServiceCore>;

class MILLION_API ServiceHandle {
public:
    ServiceHandle() = default;
    explicit ServiceHandle(ServiceShared shared)
        : weak_(shared) {}
    ~ServiceHandle() = default;

    ServiceHandle(const ServiceHandle&) = default;
    void operator=(const ServiceHandle& v) {
        weak_ = v.weak_;
    }
    ServiceHandle(ServiceHandle&&) = default;
    void operator=(ServiceHandle&& v) noexcept {
        weak_ = std::move(v.weak_);
    }

    ServiceShared lock() const {
        return weak_.lock();
    }

    IService* get_ptr(const ServiceShared& lock) const;

    template <typename IServiceT>
    IServiceT* get_ptr(const ServiceShared& lock) const { return static_cast<IServiceT*>(get_ptr(lock)); }

private:
    ServiceWeak weak_;
};

} // namespace million