#pragma once

#include <cassert>

#include <memory>

#include <million/detail/dl_export.h>
#include <million/detail/noncopyable.h>
#include <million/service_handle.h>
#include <million/imsg.h>
#include <million/task.hpp>

namespace million {

class IMillion;
class MILLION_CLASS_EXPORT IService : noncopyable {
public:
    IService(IMillion* imillion)
        : imillion_(imillion) {}
    virtual ~IService() = default;

    SessionId Send(ServiceHandle target, MsgUnique msg);
    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceHandle target, Args&&... args) {
        return Send(target, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    template <typename MsgT>
    Awaiter<MsgT> Recv(SessionId session_id) {
        return Awaiter<MsgT>(session_id);
    }

    void Reply(MsgUnique msg);
    void Reply(ServiceHandle target, SessionId session_id);
    template <typename MsgT, typename ...Args>
    void Reply(ServiceHandle target, SessionId session_id, Args&&... args) {
        auto msg = std::make_unique<MsgT>(std::forward<Args>(args)...);
        msg->set_session_id(session_id);
        Send(target, std::move(msg));
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    Awaiter<RecvMsgT> Call(ServiceHandle target, Args&&... args) {
        auto session_id = Send(target, std::make_unique<SendMsgT>(std::forward<Args>(args)...));
        return Recv<RecvMsgT>(session_id);
    }
    template <typename SendMsgT, typename ...Args>
    Awaiter<IMsg> Call(ServiceHandle target, Args&&... args) {
        auto session_id = Send(target, std::make_unique<SendMsgT>(std::forward<Args>(args)...));
        return Recv<IMsg>(session_id);
    }

    virtual void OnInit() {};
    virtual Task OnMsg(MsgUnique msg) = 0;
    virtual void OnExit() {};

    ServiceHandle service_handle() const { return service_handle_; }
    void set_service_handle(ServiceHandle handle) { service_handle_ = handle; }

protected:
    IMillion* imillion_;
    ServiceHandle service_handle_;
};

#define MILLION_MSG_DISPATCH(MILLION_SERVICE_TYPE_, MILLION_MSG_BASE_TYPE_) \
    static_assert(std::is_same<std::underlying_type_t<MILLION_MSG_BASE_TYPE_::MsgType>, uint32_t>::value, "type is not based on uint32_t.");; \
    using _MILLION_SERVICE_TYPE_ = MILLION_SERVICE_TYPE_; \
    ::million::Task OnMsgDispatch(::million::MsgUnique msg) { \
        auto msg_ptr = msg->get<##MILLION_MSG_BASE_TYPE_##>(); \
        auto iter = _MILLION_MSG_HANDLE_MAP_.find(static_cast<uint32_t>(msg_ptr->type())); \
        if (iter != _MILLION_MSG_HANDLE_MAP_.end()) { \
            co_await(this->*iter->second)(std::move(msg)); \
        } \
        co_return; \
    } \
    ::std::unordered_map<uint32_t, ::million::Task(_MILLION_SERVICE_TYPE_::*)(::million::MsgUnique)> _MILLION_MSG_HANDLE_MAP_ \


#define MILLION_MSG_HANDLE(MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task _MILLION_MSG_HANDLE_##MSG_TYPE_##_I(::million::MsgUnique MILLION_MSG_) { \
        auto msg = ::std::unique_ptr<MSG_TYPE_>(static_cast<MSG_TYPE_*>(MILLION_MSG_.release())); \
        co_await _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(std::move(msg)); \
        co_return; \
    } \
    const bool _MILLION_MSG_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [this] { \
            auto res = _MILLION_MSG_HANDLE_MAP_.insert(::std::make_pair(static_cast<uint32_t>(MSG_TYPE_::kTypeValue), \
                &_MILLION_SERVICE_TYPE_::_MILLION_MSG_HANDLE_##MSG_TYPE_##_I \
            )); \
            assert(res.second); \
            return true; \
        }(); \
    ::million::Task _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(::std::unique_ptr<MSG_TYPE_> MSG_PTR_NAME_)

} // namespace million