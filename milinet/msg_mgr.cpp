#include "milinet/msg_mgr.h"

#include <cassert>

#include "milinet/milinet.h"

namespace milinet {

MsgMgr::MsgMgr(Milinet* milinet)
    : milinet_(milinet) {}

MsgMgr::~MsgMgr() = default;

SessionId MsgMgr::AllocSessionId() {
    auto id = ++session_id_;
    if (id == 0) {
        throw std::runtime_error("session id rolled back.");
    }
    return id;
}

} //namespace milinet