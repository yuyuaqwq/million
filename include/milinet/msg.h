#pragma once

#include <cstdint>

#include "milinet/msg_def.h"
#include "milinet/noncopyable.h"

namespace milinet {

class Msg : noncopyable {
public:
    Msg(SessionId session_id)
        : session_id_(session_id) {}
    virtual ~Msg() = default;

    SessionId session_id() { return session_id_; }
private:
    SessionId session_id_;
};

} // namespace milinet