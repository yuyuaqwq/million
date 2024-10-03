#pragma once

#include <cstdint>

#include "milinet/msg_def.h"
#include "milinet/noncopyable.h"

namespace milinet {

class Msg : noncopyable {
public:
    Msg() {}
    virtual ~Msg() = default;

    SessionId session_id() const { return session_id_; }
    void set_session_id(SessionId session_id) { session_id_ = session_id; }

private:
    SessionId session_id_;
};

} // namespace milinet