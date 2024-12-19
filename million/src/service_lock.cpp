#include <million/service_lock.h>

#include "million.h"
#include "service.h"

namespace million {

MILLION_MSG_DEFINE_EMPTY(, ServiceLockMsg);

ServiceLock::ServiceLock(IService* iservice)
	: iservice_(iservice)
	, locking_(false) {}

Task<> ServiceLock::Lock() {
	if (locking_ == false) {
		locking_ = true;
		co_return;
	}
	
	auto& imillion = iservice_->imillion();
	auto session_id = imillion.NewSession();
	wait_sessions_.push(session_id);
	co_await imillion.Recv<ServiceLockMsg>(session_id);
	locking_ = true;
}

void ServiceLock::Unlock() {
	locking_ = false;
	if (!wait_sessions_.empty()) {
		auto& imillion = iservice_->imillion();
		imillion.SendTo<ServiceLockMsg>(iservice_->service_handle(), iservice_->service_handle(), wait_sessions_.front());
		wait_sessions_.pop();
	}
}

} // namespace million