#pragma once

#include <cassert>

#include <memory>

#include <million/api.h>
#include <million/noncopyable.h>
#include <million/service_handle.h>
#include <million/imsg.h>
#include <million/task.h>

namespace million {

class IMillion;
class MILLION_CLASS_API IService : noncopyable {
public:
    IService(IMillion* imillion)
        : imillion_(imillion) {}
    virtual ~IService() = default;

    SessionId Send(const ServiceHandle& target, MsgUnique msg);
    template <typename MsgT, typename ...Args>
    SessionId Send(const ServiceHandle& target, Args&&... args) {
        return Send(target, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    template <typename MsgT>
    Awaiter<MsgT> Recv(SessionId session_id) {
        return Awaiter<MsgT>(session_id);
    }

    void Reply(MsgUnique msg);
    void Reply(const ServiceHandle& target, SessionId session_id);
    template <typename MsgT, typename ...Args>
    void Reply(const ServiceHandle& target, SessionId session_id, Args&&... args) {
        auto msg = std::make_unique<MsgT>(std::forward<Args>(args)...);
        msg->set_session_id(session_id);
        msg->set_sender(target);
        Reply(std::move(msg));
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    Awaiter<RecvMsgT> Call(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send(target, std::make_unique<SendMsgT>(std::forward<Args>(args)...));
        return Recv<RecvMsgT>(session_id);
    }
    template <typename MsgT, typename ...Args>
    Awaiter<MsgT> Call(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send(target, std::make_unique<MsgT>(std::forward<Args>(args)...));
        return Recv<MsgT>(session_id);
    }

    void Timeout(uint32_t tick, MsgUnique msg);
    template <typename MsgT, typename ...Args>
    void Timeout(uint32_t tick, Args&&... args) {
        Timeout(tick, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    void EnableSeparateWorker();

    virtual void OnInit() {};
    virtual Task OnMsg(MsgUnique msg) = 0;
    virtual void OnExit() {};

    const ServiceHandle& service_handle() const { return service_handle_; }
    void set_service_handle(const ServiceHandle& handle) { service_handle_ = handle; }

protected:
    IMillion* imillion_;
    ServiceHandle service_handle_;
};

#define MILLION_MSG_DISPATCH(MILLION_SERVICE_TYPE_) \
    using _MILLION_SERVICE_TYPE_ = MILLION_SERVICE_TYPE_; \
    virtual ::million::Task OnMsg(::million::MsgUnique msg) override { \
        auto iter = _MILLION_MSG_HANDLE_MAP_.find(msg->type()); \
        if (iter != _MILLION_MSG_HANDLE_MAP_.end()) { \
            auto task = (this->*iter->second)(std::move(msg)); \
            co_await task; \
            task.rethrow_if_exception(); \
        } \
        co_return; \
    } \
    ::std::unordered_map<std::string, ::million::Task(_MILLION_SERVICE_TYPE_::*)(::million::MsgUnique)> _MILLION_MSG_HANDLE_MAP_ \


#define MILLION_MSG_HANDLE(MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task _MILLION_MSG_HANDLE_##MSG_TYPE_##_I(::million::MsgUnique MILLION_MSG_) { \
        auto msg = ::std::unique_ptr<MSG_TYPE_>(static_cast<MSG_TYPE_*>(MILLION_MSG_.release())); \
        auto task = _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(std::move(msg)); \
        co_await task; \
        task.rethrow_if_exception(); \
        co_return; \
    } \
    const bool _MILLION_MSG_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [this] { \
            auto res = _MILLION_MSG_HANDLE_MAP_.insert(::std::make_pair(MSG_TYPE_::kType, \
                &_MILLION_SERVICE_TYPE_::_MILLION_MSG_HANDLE_##MSG_TYPE_##_I \
            )); \
            assert(res.second); \
            return true; \
        }(); \
    ::million::Task _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(::std::unique_ptr<MSG_TYPE_> MSG_PTR_NAME_)

} // namespace million