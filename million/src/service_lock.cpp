#include <million/service_lock.h>

#include <protogen/ss/ss_service.pb.h>

#include "service.h"

namespace million {

ServiceLock::ServiceLock(IService* iservice)
	: iservice_(iservice)
	, locking_(false) {}

Task<> ServiceLock::Lock() {
	if (locking_ == false) {
		locking_ = true;
		co_return;
	}
	auto session_id = iservice_->NewSession();
	wait_sessions_.push(session_id);
	co_await iservice_->Recv<ss::service::ServiceUnlock>(session_id);
	locking_ = true;
}

void ServiceLock::Unlock() {
	locking_ = false;
	if (!wait_sessions_.empty()) {
		iservice_->SendTo<ss::service::ServiceUnlock>(iservice_->service_handle(), wait_sessions_.front());
		wait_sessions_.pop();
	}
}

} // namespace million