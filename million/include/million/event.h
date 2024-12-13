#pragma once

#include <cassert>

#include <unordered_map>

#include <million/api.h>
#include <million/imsg.h>
#include <million/iservice.h>
#include <million/service_handle.h>

namespace million {

MILLION_MSG_DEFINE(MILLION_API, EventRegister, (const std::type_info&) type)

class EventRegistry {
public:
	EventRegistry(IService* iservice)
		: iservice_(iservice) {}

	void AddService(const std::type_info& type, const ServiceHandle& handle) {
		auto iter = map_.find(&type);
		if (iter == map_.end()) {
			auto res = map_.emplace(&type, std::vector<ServiceHandle>());
			assert(res.second);
			iter = res.first;
		}
		iter->second.emplace_back(handle);
	}

	bool Send(MsgUnique msg) {
		auto iter = map_.find(&msg->type());
		if (iter == map_.end()) {
			return false;
		}
		auto& services = iter->second;
		for (auto service_iter = services.begin(); service_iter != services.end(); ) {
			// �����move��Ҫ�ĵ�
			auto id = iservice_->Send(*service_iter, MsgUnique(msg->Copy()));
			if (id == kSessionIdInvalid) {
				// �ѹرյķ���
				services.erase(service_iter++);
			}
			else {
				++service_iter;
			}
		}
		return true;
	}

private:
	IService* iservice_;
	std::unordered_map<const std::type_info*, std::list<ServiceHandle>> map_;
};

} // namespace million