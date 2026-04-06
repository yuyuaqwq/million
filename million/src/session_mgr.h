#pragma once

#include <atomic>

#include <million/noncopyable.h>
#include <million/session_def.h>

namespace million {

class Million;
class SessionManager : noncopyable {
public:
    SessionManager(Million* million);
    ~SessionManager();

    SessionId NewSession();

private:
    Million* million_;

    // session_monitor
};

} // namespace million