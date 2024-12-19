#include <million/service_handle.h>

#include "service.h"

namespace million {

IService* ServiceHandle::get_ptr(const ServiceShared& lock) const {
    if (!lock) return nullptr;
    return &lock->iservice();
}

} // namespace million