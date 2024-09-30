#pragma once

#include <coroutine>
#include <optional>
#include <memory>
#include <mutex>
#include <list>
#include <functional>
#include <condition_variable>
#include <utility>

namespace milinet {

class SessionCoroutine;
struct SessionPromise;

struct SessionAwaiter {
    explicit SessionAwaiter(SessionId session_id)
        : session_id_(session_id) {}

    SessionAwaiter(SessionAwaiter&& rv) {
        operator=(std::move(rv));
    }
    void operator=(SessionAwaiter&& rv) {
        handle_ = std::move(rv.handle_);
        session_id_ = std::move(rv.session_id_);
    }

    SessionAwaiter(SessionAwaiter&) = delete;
    SessionAwaiter& operator=(SessionAwaiter&) = delete;

    constexpr bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<SessionPromise> handle) noexcept;

    MsgUnique await_resume() noexcept;

private:
    std::coroutine_handle<SessionPromise> handle_;
    SessionId session_id_;
};

struct SessionCoroutine {
    using promise_type = SessionPromise;

    explicit SessionCoroutine(std::coroutine_handle<SessionPromise> handle_pra) noexcept
        : handle(handle_pra) {}

    SessionCoroutine(SessionCoroutine&& co) noexcept
        : handle(std::exchange(co.handle, {})) {}

    ~SessionCoroutine() {
        if (handle) {
            handle.destroy();
        }
    }

    SessionCoroutine(SessionCoroutine&) = delete;
    SessionCoroutine &operator=(SessionCoroutine&) = delete;
    
    std::coroutine_handle<SessionPromise> handle;
};

struct SessionPromise {
    std::suspend_never initial_suspend() {
        return {};
    };

    std::suspend_always final_suspend() noexcept {
        return {};
    }

    SessionCoroutine get_return_object() {
        return SessionCoroutine{ std::coroutine_handle<SessionPromise>::from_promise(*this) };
    }

    void return_void() {
        
    }

    void unhandled_exception() {

    }

    SessionAwaiter await_transform(SessionAwaiter awaiter) {
        return std::move(awaiter);
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

    MsgUnique result_;
    SessionId waiting_session_id_;
};

inline void SessionAwaiter::await_suspend(std::coroutine_handle<SessionPromise> handle) noexcept {
    // resume交给调度器，Recv等待是必定挂起的
    handle_ = handle;
    handle_.promise().set_waiting(session_id_);
}

inline MsgUnique SessionAwaiter::await_resume() noexcept {
    // 调度器恢复了当前协程的执行，说明已经等到结果了
    return handle_.promise().get_result();
}

} // namespace milinet