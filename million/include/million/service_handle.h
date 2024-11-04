#pragma once

#include <cstdint>

#include <list>
#include <memory>

#include <million/detail/dl_export.h>

namespace million {

using ServiceCodeName = std::string;

class Service;
class MILLION_CLASS_EXPORT ServiceHandle {
public:
    ServiceHandle() = default;
    explicit ServiceHandle(std::list<std::unique_ptr<Service>>::iterator iter)
        : iter_(iter) {}
    ~ServiceHandle() = default;

    ServiceHandle(const ServiceHandle&) = default;
    void operator=(const ServiceHandle& v) {
        iter_ = v.iter_;
    }

    Service& service() const { return *iter_->get(); }
    auto iter() const { return iter_; }

private:
    std::list<std::unique_ptr<Service>>::iterator iter_;
};

} // namespace million