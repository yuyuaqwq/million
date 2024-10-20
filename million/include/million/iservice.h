#pragma once

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

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    Awaiter<RecvMsgT> Call(ServiceHandle target, Args&&... args) {
        auto session_id = Send(target, std::make_unique<SendMsgT>(std::forward<Args>(args)...));
        return Recv<RecvMsgT>(session_id);
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

#define MILLION_MSG_DISPATCH(_MSG_BASE_TYPE, _MSG_UNIQUE) \
    auto _MSG_PTR = _MSG_UNIQUE->get<_MSG_BASE_TYPE>(); \
    auto iter = MSG_HANDLE_MAP_.find(_MSG_PTR->type()); \
    if (iter != MSG_HANDLE_MAP_.end()) { \
        iter->second(_MSG_UNIQUE); \
    }

#define MILLION_MSG_HANDLE_INIT(_MSG_BASE_TYPE) \
    ::std::unordered_map<_MSG_BASE_TYPE::MsgType, ::std::function<void(::million::MsgUnique)>> MSG_HANDLE_MAP_

#define MILLION_MSG_HANDLE_BEGIN(_MSG_TYPE, _MSG_PTR_NAME) \
    ::million::Task MILLION_HANDLE_##_MSG_TYPE(::std::unique_ptr<_MSG_TYPE> _MSG_PTR_NAME)

#define MILLION_MSG_HANDLE_END(_MSG_TYPE) \
    const bool MILLION_HANDLE_REGISTER_##_MSG_TYPE = [this] { \
    MSG_HANDLE_MAP_.insert(::std::make_pair(_MSG_TYPE::kTypeValue, [this](::million::MsgUnique msg) { \
            MILLION_HANDLE_##_MSG_TYPE(::std::move(::std::unique_ptr<_MSG_TYPE>(static_cast<_MSG_TYPE*>(msg.release())))); \
        }) \
    ); \
    return true; \
    }();


} // namespace million