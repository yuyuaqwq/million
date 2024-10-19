#pragma once

#include <atomic>

#include <million/detail/noncopyable.h>
#include <million/msg_def.h>

namespace million {

class Million;
class MsgMgr : noncopyable {
public:
    MsgMgr(Million* million);
    ~MsgMgr();

    SessionId AllocSessionId();

private:
    Million* million_;
    std::atomic<SessionId> session_id_ = 0;
};

} // namespace million