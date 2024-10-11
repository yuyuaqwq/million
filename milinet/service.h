#pragma once

#include <cstdint>

#include <memory>
#include <mutex>
#include <queue>

#include "milinet/detail/noncopyable.h"
#include "milinet/iservice.hpp"
#include "milinet/service_handle.h"
#include "milinet/imsg.hpp"
#include "milinet/msg_executor.h"

namespace milinet {

class ServiceMgr;
class Service : noncopyable {
public:
    Service(std::unique_ptr<IService> iservice);
    ~Service();

    void PushMsg(MsgUnique msg);
    MsgUnique PopMsg();
    bool MsgQueueEmpty();

    bool ProcessMsg();
    void ProcessMsgs(size_t count);

    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }

    IService& iservice() const { assert(iservice_); return *iservice_; }
    ServiceHandle service_handle() const { return iservice_->service_handle(); }

private:
    std::mutex msgs_mutex_;
    std::queue<MsgUnique> msgs_;

    MsgExecutor excutor_;

    bool in_queue_ = false;

    std::unique_ptr<IService> iservice_;
};

} // namespace milinet