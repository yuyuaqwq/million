#pragma once

#include <cstdint>

#include <string>
#include <list>
#include <memory>

#include <million/api.h>

namespace million {

using ServiceId = uint64_t;
using ServiceName = std::string;

class Service;
class IService;
class MILLION_API ServiceHandle {
public:
    ServiceHandle() = default;
    explicit ServiceHandle(std::shared_ptr<Service> ptr)
        : impl_(ptr) {}
    ~ServiceHandle() = default;

    ServiceHandle(const ServiceHandle&) = default;
    void operator=(const ServiceHandle& v) {
        impl_ = v.impl_;
    }
    ServiceHandle(ServiceHandle&&) = default;
    void operator=(ServiceHandle&& v) noexcept {
        impl_ = std::move(v.impl_);
    }

    Service* impl() const { return impl_.get(); }

    template<typename ServiceT>
    ServiceT* get() const {
        return static_cast<ServiceT*>(get());
    }

    IService* get() const;

private:
    std::shared_ptr<Service> impl_;
};

} // namespace million