#pragma once

#include <cassert>

#include <unordered_map>

#include <million/imillion.h>

namespace million {

MILLION_MSG_DEFINE(MILLION_API, EventRegister, (const std::type_info&) type)

class MILLION_API EventManager {
public:
	EventManager(IMillion* imillion)
		: imillion_(imillion) {}

	void Subscribe(const std::type_info& type, const ServiceHandle& handle) {
		auto iter = event_map_.find(&type);
		if (iter == event_map_.end()) {
			auto res = event_map_.emplace(&type, std::vector<ServiceHandle>());
			assert(res.second);
			iter = res.first;
		}
		iter->second.emplace_back(handle);
	}

	void Send(const ServiceHandle& sender, MsgUnique msg) {
		auto iter = event_map_.find(&msg->type());
		if (iter == event_map_.end()) {
			// 没有关注此事件的服务
			return;
		}
		auto& services = iter->second;
		for (auto service_iter = services.begin(); service_iter != services.end(); ) {
			auto id = imillion_->Send(sender, *service_iter, MsgUnique(msg->Copy()));
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
	IMillion* imillion_;
	std::unordered_map<const std::type_info*, std::list<ServiceHandle>> event_map_;
};

} // namespace million