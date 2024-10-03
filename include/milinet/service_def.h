#pragma once

#include <cstdint>

#include <memory>
#include <list>

namespace milinet {

class Service;
class ServiceHandle {
public:
    ServiceHandle() = default;
    explicit ServiceHandle(std::list<std::unique_ptr<Service>>::iterator iter)
        : iter_(iter) {}
    ~ServiceHandle() = default;

    ServiceHandle(const ServiceHandle&) = default;
    void operator=(const ServiceHandle& v) {
        iter_ = v.iter_;
    }

    Service* service_ptr() const { return iter_->get(); }
    auto iter() const { return iter_; }
private:
    std::list<std::unique_ptr<Service>>::iterator iter_;
};

} // namespace milinet