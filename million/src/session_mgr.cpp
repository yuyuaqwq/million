#include "session_mgr.h"

#include <cassert>

#include "million.h"

namespace million {

SessionMgr::SessionMgr(Million* million)
: million_(million) {}

SessionMgr::~SessionMgr() = default;

SessionId SessionMgr::AllocSessionId() {
    auto id = ++session_id_;
    if (id == 0) {
        throw std::runtime_error("session id rolled back.");
    }
    return id;
}

} //namespace million