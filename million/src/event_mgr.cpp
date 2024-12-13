#include <million/event_mgr.h>

#include "service.h"

namespace million {

EventMgr::EventMgr(IMillion* imillion)
	: imillion_(imillion) {}

void EventMgr::Subscribe(const std::type_info& type, const ServiceHandle& handle, uint32_t priority) {
	auto iter = services_.find(&type);
	if (iter == services_.end()) {
		auto res = services_.emplace(&type, EventMap());
		assert(res.second);
		iter = res.first;
	}
	iter->second.emplace(priority, handle);
}

bool EventMgr::Unsubscribe(const std::type_info& type, const ServiceHandle& subscriber) {
	auto iter = services_.find(&type);
	if (iter == services_.end()) {
		return false;
	}
	auto& services = iter->second;
	for (auto service_iter = services.begin(); service_iter != services.end(); ) {
		if (service_iter->second.service() == subscriber.service()) {
			services.erase(service_iter);
			return true;
		}
	}
	return false;
}

void EventMgr::Send(const ServiceHandle& sender, MsgUnique msg) {
	auto iter = services_.find(&msg->type());
	if (iter == services_.end()) {
		// 没有关注此事件的服务
		return;
	}
	
	auto sender_service = sender.service();
	if (!sender_service) {
		return;
	}
	auto& sender_iservice = sender_service->iservice();

	auto& services = iter->second;
	for (auto service_iter = services.begin(); service_iter != services.end(); ) {
		auto id = sender_iservice.Send(service_iter->second, MsgUnique(msg->Copy()));
		if (id == kSessionIdInvalid) {
			// 已关闭的服务
			services.erase(service_iter++);
		}
		else {
			++service_iter;
		}
	}
}

Task<> EventMgr::Call(const ServiceHandle& caller, MsgUnique msg, std::function<bool(MsgUnique)> res_handle) {
	auto iter = services_.find(&msg->type());
	if (iter == services_.end()) {
		// 没有关注此事件的服务
		co_return;
	}
	auto& services = iter->second;
	auto caller_service = caller.service();
	if (!caller_service) {
		co_return;
	}
	auto& caller_iservice = caller_service->iservice();
	for (auto service_iter = services.begin(); service_iter != services.end(); ) {
		auto id = caller_iservice.Send(service_iter->second, MsgUnique(msg->Copy()));
		if (id == kSessionIdInvalid) {
			// 已关闭的服务
			services.erase(service_iter++);
		}
		else {
			auto res = co_await caller_iservice.RecvOrNull<IMsg>(id);
			if (res && !res_handle(std::move(res))) {
				break;
			}
			++service_iter;
		}
	}
}

} // namespace million