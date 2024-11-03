#pragma once

#include <atomic>

#include <million/detail/noncopyable.h>
#include <million/msg_def.h>

namespace million {

class Million;
class SessionMgr : noncopyable {
public:
    SessionMgr(Million* million);
    ~SessionMgr();

    SessionId AllocSessionId();

private:
    Million* million_;
    std::atomic<SessionId> session_id_ = 0;

    // session_monitor
};

} // namespace million