#include <million/event_mgr.h>

#include "million.h"
#include "service.h"

namespace million {

EventMgr::EventMgr(IMillion* imillion)
	: imillion_(imillion) {}

void EventMgr::Subscribe(MsgTypeKey key, const ServiceHandle& handle, uint32_t priority) {
	auto iter = services_.find(key);
	if (iter == services_.end()) {
		auto res = services_.emplace(key, EventMap());
		assert(res.second);
		iter = res.first;
	}
	iter->second.emplace(priority, handle);
}

bool EventMgr::Unsubscribe(MsgTypeKey key, const ServiceHandle& subscriber) {
	auto iter = services_.find(key);
	if (iter == services_.end()) {
		return false;
	}
	auto& services = iter->second;
	auto lock = subscriber.lock();
	for (auto service_iter = services.begin(); service_iter != services.end(); ) {
		if (service_iter->second.lock() == lock) {
			services.erase(service_iter);
			return true;
		}
	}
	return false;
}

void EventMgr::Send(const ServiceHandle& sender, MsgUnique msg) {
	auto iter = services_.find(msg.GetTypeKey());
	if (iter == services_.end()) {
		// 没有关注此事件的服务
		return;
	}
	
	auto sender_lock = sender.lock();
	if (!sender_lock) {
		return;
	}

	auto& services = iter->second;
	for (auto service_iter = services.begin(); service_iter != services.end(); ) {
		auto target_lock = service_iter->second.lock();
		if (!target_lock) {
			// 已关闭的服务
			services.erase(service_iter++);
			continue;
		}
		imillion_->impl().Send(sender_lock, target_lock, MsgUnique(msg.Copy()));
		++service_iter;
	}
}

Task<> EventMgr::Call(const ServiceHandle& caller, MsgUnique msg, std::function<bool(MsgUnique)> res_handle) {
	auto iter = services_.find(msg.GetTypeKey());
	if (iter == services_.end()) {
		// 没有关注此事件的服务
		co_return;
	}
	auto& services = iter->second;
	auto caller_lock = caller.lock();
	if (!caller_lock) {
		co_return;
	}
	for (auto service_iter = services.begin(); service_iter != services.end(); ) {
		auto target_lock = service_iter->second.lock();
		if (!target_lock) {
			// 已关闭的服务
			services.erase(service_iter++);
			continue;
		}

		auto session_id = imillion_->impl().Send(caller_lock, target_lock, MsgUnique(msg.Copy()));

		if (msg.IsProtoMessage()) {
			auto res = co_await imillion_->RecvOrNull<ProtoMessage>(session_id);
			if (res && !res_handle(MsgUnique(res.release()))) {
				break;
			}
		}
		else if (msg.IsCppMessage()) {
			auto res = co_await imillion_->RecvOrNull<CppMessage>(session_id);
			if (res && !res_handle(MsgUnique(res.release()))) {
				break;
			}
		}
		++service_iter;
		
	}
}

} // namespace million