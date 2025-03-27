#include "session_mgr.h"

#include <cassert>

#include <million/seata_snowflake.hpp>

#include "million.h"

namespace million {

SessionMgr::SessionMgr(Million* million)
: million_(million) {}

SessionMgr::~SessionMgr() = default;

SessionId SessionMgr::NewSession() {
    return million_->seata_snowflake().NextId();
}

} //namespace million