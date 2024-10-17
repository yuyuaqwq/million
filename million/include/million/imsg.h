#pragma once

#include <cstdint>

#include <million/detail/dl_export.h>
#include <million/msg_def.h>
#include <million/detail/noncopyable.h>

namespace million {

class MILLION_CLASS_EXPORT IMsg : noncopyable {
public:
    IMsg() = default;
    virtual ~IMsg() = default;

    SessionId session_id() const { return session_id_; }
    void set_session_id(SessionId session_id) { session_id_ = session_id; }

    template<typename MsgT>
    MsgT* get() { return static_cast<MsgT*>(this); }

private:
    SessionId session_id_{ kSessionIdInvalid };
};

template<typename TypeT>
class MsgBaseT : public IMsg {
public:
    MsgBaseT(TypeT type) : type_(type) {}
    ~MsgBaseT() = default;

    using IMsg::get;

    TypeT type() const { return type_; }

private:
    const TypeT type_;
};

template<auto kType>
class MsgT : public MsgBaseT<decltype(kType)> {
public:
    using TypeT = decltype(kType);
    static constexpr TypeT TypeValue = kType;

    using Base = MsgBaseT<TypeT>;

    MsgT() : Base(kType) {};
    ~MsgT() = default;
};

} // namespace million