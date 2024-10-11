#pragma once

#include <cstdint>

#include <milinet/detail/dl_export.hpp>
#include <milinet/msg_def.h>
#include <milinet/detail/noncopyable.h>

namespace milinet {

class MILINET_CLASS_EXPORT IMsg : noncopyable {
public:
    IMsg() {}
    virtual ~IMsg() = default;

    SessionId session_id() const { return session_id_; }
    void set_session_id(SessionId session_id) { session_id_ = session_id; }

private:
    SessionId session_id_{ kSessionIdInvalid };
};

} // namespace milinet