#pragma once

#include <cstdint>

#include <memory>
#include <mutex>
#include <queue>

#include "noncopyable.h"

#include "milinet/msg.h"

namespace milinet {

class Service : noncopyable {
public:
    Service(uint32_t id);
    virtual ~Service();

    void PushMsg(std::unique_ptr<Msg> msg);
    std::unique_ptr<Msg> PopMsg();
    bool MsgQueueEmpty();

    bool ProcessMsg();
    void ProcessMsgs(size_t count);

    virtual void OnMsg(std::unique_ptr<Msg> msg);

    uint32_t id() const { return id_; }
    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }

private:
    uint32_t id_;

    std::mutex msgs_mutex_;
    std::queue<std::unique_ptr<Msg>> msgs_;

    bool in_queue_ = false;
};

} // namespace milinet