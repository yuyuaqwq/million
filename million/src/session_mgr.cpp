#include "session_mgr.h"

#include <cassert>

#include "million.h"

namespace million {

SessionManager::SessionManager(Million* million)
: million_(million) {}

SessionManager::~SessionManager() = default;

SessionId SessionManager::NewSession() {
    return million_->NextSequenceId();
}

} //namespace million