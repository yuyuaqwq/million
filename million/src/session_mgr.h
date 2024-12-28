#pragma once

#include <atomic>

#include <million/noncopyable.h>
#include <million/session_def.h>

namespace million {

class Million;
class SessionMgr : noncopyable {
public:
    SessionMgr(Million* million);
    ~SessionMgr();

    SessionId NewSession();

private:
    Million* million_;
    std::atomic<SessionId> session_id_ = kSessionIdInvalid;

    // session_monitor
};

} // namespace million