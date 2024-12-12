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
class MILLION_API ServiceHandle {
public:
    ServiceHandle() = default;
    explicit ServiceHandle(std::shared_ptr<Service> ptr)
        : shared_(ptr) {}
    ~ServiceHandle() = default;

    ServiceHandle(const ServiceHandle&) = default;
    void operator=(const ServiceHandle& v) {
        shared_ = v.shared_;
    }
    ServiceHandle(ServiceHandle&&) = default;
    void operator=(ServiceHandle&& v) noexcept {
        shared_ = std::move(v.shared_);
    }

    Service* service() const { return shared_.get(); }

private:
    std::shared_ptr<Service> shared_;
};

} // namespace million