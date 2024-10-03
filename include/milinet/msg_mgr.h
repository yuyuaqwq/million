#pragma once

#include <atomic>

#include "milinet/noncopyable.h"
#include "milinet/msg_def.h"

namespace milinet {

class Milinet;
class MsgMgr : noncopyable {
public:
    MsgMgr(Milinet* milinet);
    ~MsgMgr();

    SessionId AllocSessionId();

private:
    Milinet* milinet_;
    std::atomic<SessionId> session_id_ = 0;
};

} // namespace milinet