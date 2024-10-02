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

    template <typename MsgT, typename ...Args>
    std::unique_ptr<MsgT> MakeMsg(Args&&... args) {
        auto id = AllocSessionId();
        return std::make_unique<MsgT>(id, std::forward<Args>(args)...);
    }

private:
    Milinet* milinet_;
    std::atomic<SessionId> session_id_ = 0;
};

} // namespace milinet