#include <million/service_lock.h>

#include "service.h"

namespace million {

MILLION_MSG_DEFINE_EMPTY(, MillionServiceUnlockMsg);

ServiceLock::ServiceLock(IService* iservice)
	: iservice_(iservice)
	, locking_(false) {}

Task<> ServiceLock::Lock() {
	if (locking_ == false) {
		locking_ = true;
		co_return;
	}
	auto session_id = iservice_->AllocSessionId();
	wait_sessions_.push(session_id);
	co_await iservice_->Recv<MillionServiceUnlockMsg>(session_id);
	locking_ = true;
}

void ServiceLock::Unlock() {
	locking_ = false;
	if (!wait_sessions_.empty()) {
		iservice_->SendTo<MillionServiceUnlockMsg>(iservice_->service_handle(), wait_sessions_.front());
		wait_sessions_.pop();
	}
}

} // namespace million