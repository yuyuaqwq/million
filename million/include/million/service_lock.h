#pragma once

#include <queue>
#include <million/api.h>
#include <million/imsg.h>
#include <million/task.h>

namespace million {

class IService;
class MILLION_API ServiceLock {
public:
	ServiceLock(IService* iservice);

	Task<> Lock();
	void Unlock();

private:
	IService* iservice_;
	bool locking_;
	std::queue<SessionId> wait_sessions_;
};

} // namespace million