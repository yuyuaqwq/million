#include "million/msg_mgr.h"

#include <cassert>

#include "million/million.h"

namespace million {

MsgMgr::MsgMgr(Million* million)
    : million_(million) {}

MsgMgr::~MsgMgr() = default;

SessionId MsgMgr::AllocSessionId() {
    auto id = ++session_id_;
    if (id == 0) {
        throw std::runtime_error("session id rolled back.");
    }
    return id;
}

} //namespace million