#include "session_mgr.h"

#include <cassert>

#include "million.h"

namespace million {

SessionMgr::SessionMgr(Million* million)
: million_(million) {}

SessionMgr::~SessionMgr() = default;

SessionId SessionMgr::NewSession() {
    auto id = ++session_id_;
    if (id > 0x7fffffffffffffff) {
        throw std::overflow_error("Session ID overflow: no more IDs available.");
    }
    return id;
}

} //namespace million