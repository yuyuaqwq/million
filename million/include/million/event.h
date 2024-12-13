#pragma once

#include <cassert>

#include <unordered_map>

#include <million/api.h>
#include <million/imsg.h>
#include <million/iservice.h>
#include <million/service_handle.h>

namespace million {

MILLION_MSG_DEFINE(MILLION_API, EventRegister, (const std::type_info&) type)

class EventSubscriber {
public:
	EventSubscriber(IService* iservice)
		: iservice_(iservice) {}

	void Subscribe(const std::type_info& type, const ServiceHandle& handle) {
		auto iter = event_map_.find(&type);
		if (iter == event_map_.end()) {
			auto res = event_map_.emplace(&type, std::vector<ServiceHandle>());
			assert(res.second);
			iter = res.first;
		}
		iter->second.emplace_back(handle);
	}

	void Send(MsgUnique msg) {
		auto iter = event_map_.find(&msg->type());
		if (iter == event_map_.end()) {
			// 没有关注此事件的服务
			return;
		}
		auto& services = iter->second;
		for (auto service_iter = services.begin(); service_iter != services.end(); ) {
			// 这里的move还要改掉
			auto id = iservice_->Send(*service_iter, MsgUnique(msg->Copy()));
			if (id == kSessionIdInvalid) {
				// 已关闭的服务
				services.erase(service_iter++);
			}
			else {
				++service_iter;
			}
		}
	}

private:
	IService* iservice_;
	std::unordered_map<const std::type_info*, std::list<ServiceHandle>> event_map_;
};

} // namespace million