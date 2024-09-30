#pragma once

#include <cassert>

#include <coroutine>
#include <optional>
#include <memory>
#include <mutex>
#include <list>
#include <functional>
#include <condition_variable>
#include <utility>

namespace milinet {


class Task;
struct TaskPromise;

template <typename MsgT>
struct Awaiter {
    explicit Awaiter(SessionId session_id)
        : session_id_(session_id) {}

    Awaiter(Awaiter&& rv) {
        operator=(std::move(rv));
    }
    void operator=(Awaiter&& rv) {
        parent_handle_ = std::move(rv.parent_handle_);
        session_id_ = std::move(rv.session_id_);
    }

    Awaiter(Awaiter&) = delete;
    Awaiter& operator=(Awaiter&) = delete;

    constexpr bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<TaskPromise> parent_handle) noexcept {
        // resume交给调度器，等待是必定挂起的
        parent_handle_ = parent_handle;
        parent_handle_.promise().set_waiting(session_id_);
    }

    std::unique_ptr<MsgT> await_resume() noexcept {
        // 调度器恢复了当前Awaiter的执行，说明已经等到结果了
        return parent_handle_.promise().get_result();
    }

private:
    std::coroutine_handle<TaskPromise> parent_handle_;
    SessionId session_id_;
};

struct Task {
    using promise_type = TaskPromise;

    explicit Task(std::coroutine_handle<TaskPromise> handle) noexcept
        : handle(handle) {}

    Task(Task&& co) noexcept
        : handle(std::exchange(co.handle, {})) {}

    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }

    Task(Task&) = delete;
    Task& operator=(Task&) = delete;
    
    constexpr bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<TaskPromise> parent_handle) noexcept;

    void await_resume() noexcept;

    std::coroutine_handle<TaskPromise> handle;
    std::coroutine_handle<TaskPromise> parent_handle_;
};

struct TaskPromise {
    // 协程立即执行
    std::suspend_never initial_suspend() {
        return {};
    };

    // 执行结束后挂起
    std::suspend_always final_suspend() noexcept {
        return {};
    }

    Task get_return_object() {
        return Task{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }

    // co_return;
    void return_void() {
        
    }

    void unhandled_exception() {

    }

    template <typename MsgT>
    Awaiter<MsgT> await_transform(Awaiter<MsgT>&& awaiter) {
        return Awaiter<MsgT>(std::move(awaiter));
    }

    Task await_transform(Task&& task) {
        return Task(std::move(task));
    }


    void set_result(MsgUnique msg) {
        result_ = std::move(msg);
    }

    MsgUnique get_result() {
        return std::move(result_);
    }

    void set_waiting(SessionId session_id) {
        waiting_session_id_ = session_id;
    }

    SessionId get_waiting() {
        return waiting_session_id_;
    }

    void set_waiting_handle(std::coroutine_handle<TaskPromise> waiting_handle) {
        waiting_handle_ = waiting_handle;
    }

    std::coroutine_handle<TaskPromise> get_waiting_handle() {
        return waiting_handle_;
    }

private:
    MsgUnique result_;
    SessionId waiting_session_id_;
    std::coroutine_handle<TaskPromise> waiting_handle_;
};

inline void Task::await_suspend(std::coroutine_handle<TaskPromise> parent_handle) noexcept {
    // 这里的handle是parent coroutine的handle
    // resume交给调度器，等待是必定挂起的
    parent_handle_ = parent_handle;
    parent_handle.promise().set_waiting_handle(handle);
    parent_handle.promise().set_waiting(handle.promise().get_waiting());
}

inline void Task::await_resume() noexcept {
    // 调度器恢复了当前Task的执行，传递msg，继续向下唤醒，直到传递给Awaiter
    auto waiting_handle = parent_handle_.promise().get_waiting_handle();
    assert(waiting_handle);
    assert(waiting_handle == handle);
    handle.promise().set_result(parent_handle_.promise().get_result());
    waiting_handle.resume();
}

} // namespace milinet