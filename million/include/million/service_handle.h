#pragma once

#include <cstdint>

#include <string>
#include <list>
#include <memory>

#include <million/api.h>

namespace million {

using ServiceId = uint64_t;
using ServiceName = std::string;

class IService;
class Service;
using ServiceShared = std::shared_ptr<Service>;
using ServiceWeak = std::weak_ptr<Service>;

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
        auto shared = weak_.lock();
        return shared;
    }

private:
    ServiceWeak weak_;
};

} // namespace million