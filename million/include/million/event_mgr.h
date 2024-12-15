#pragma once

#include <cassert>

#include <map>
#include <unordered_map>

#include <million/imillion.h>

namespace million {

class MILLION_API EventMgr : noncopyable {
public:
	EventMgr(IMillion* imillion);

	void Subscribe(MsgTypeKey key, const ServiceHandle& subscriber, uint32_t priority = 0);
	bool Unsubscribe(MsgTypeKey key, const ServiceHandle& subscriber);

	void Send(const ServiceHandle& sender, MsgUnique msg);
	Task<> Call(const ServiceHandle& caller, MsgUnique msg, std::function<bool(MsgUnique)> res_handle);

private:
	IMillion* imillion_;
	using EventMap = std::multimap<uint32_t, ServiceHandle, std::greater<uint32_t>>;
	std::unordered_map<MsgTypeKey, EventMap> services_;
};

} // namespace million