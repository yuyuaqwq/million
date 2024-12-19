#include <million/service_handle.h>

#include "service.h"

namespace million {

IService* ServiceHandle::get() const {
	if (!impl()) return nullptr;
	return &impl()->iservice();
}

} // namespace million