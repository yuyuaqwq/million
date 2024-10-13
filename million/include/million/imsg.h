#pragma once

#include <cstdint>

#include <million/detail/dl_export.h>
#include <million/msg_def.h>
#include <million/detail/noncopyable.h>

namespace million {

class MILLION_CLASS_EXPORT IMsg : noncopyable {
public:
    IMsg() {}
    virtual ~IMsg() = default;

    SessionId session_id() const { return session_id_; }
    void set_session_id(SessionId session_id) { session_id_ = session_id; }

private:
    SessionId session_id_{ kSessionIdInvalid };
};

} // namespace million