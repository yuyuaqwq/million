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

#define MILLION_HANDLE_MSG_BEGIN(_MSG, _MSG_TYPE) \
    auto _MSG_UNIQUE = _MSG; \
    auto _MSG_PTR = _MSG_UNIQUE->get<_MSG_TYPE>(); \
    switch (_MSG_PTR->type()) {

#define MILLION_HANDLE_MSG(_MSG, _TYPE, _CODE_BLOCK) \
    case _TYPE::kTypeValue: { \
        auto _MSG = std::unique_ptr<_TYPE>(static_cast<_TYPE*>(_MSG_UNIQUE.release())); \
        _CODE_BLOCK \
        break; \
    }

#define MILLION_HANDLE_MSG_END() \
    }

} // namespace million