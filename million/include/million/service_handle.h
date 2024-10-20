#pragma once

#include <cstdint>

#include <memory>

#include <million/detail/list.hpp>

namespace million {

using ServiceCodeName = std::string;

class Service;
class ServiceHandle {
public:
    ServiceHandle() = default;
    explicit ServiceHandle(million::list<std::unique_ptr<Service>>::iterator iter)
        : iter_(iter) {}
    ~ServiceHandle() = default;

    ServiceHandle(const ServiceHandle&) = default;
    void operator=(const ServiceHandle& v) {
        iter_ = v.iter_;
    }

    Service& service() const { return *iter_->get(); }
    auto iter() const { return iter_; }
    bool is_value() const { return iter_.is_valid(); }

private:
    million::list<std::unique_ptr<Service>>::iterator iter_;
};

} // namespace million