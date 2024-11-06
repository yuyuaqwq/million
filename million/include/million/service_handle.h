#pragma once

#include <cstdint>

#include <string>
#include <list>
#include <memory>

#include <million/api.h>

namespace million {

using ServiceCodeName = std::string;

class Service;
class MILLION_CLASS_API ServiceHandle {
public:
    ServiceHandle() = default;
    explicit ServiceHandle(std::shared_ptr<Service> ptr)
        : ptr_(ptr) {}
    ~ServiceHandle() = default;

    ServiceHandle(const ServiceHandle&) = default;
    void operator=(const ServiceHandle& v) {
        ptr_ = v.ptr_;
    }

    Service* service() const { return ptr_.get(); }

private:
    std::shared_ptr<Service> ptr_;
};

} // namespace million