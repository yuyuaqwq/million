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

namespace million {

struct Task;

struct TaskPromise;

// 消息等待器，等待一条消息
template <typename MsgT>
struct Awaiter {
    explicit Awaiter(SessionId waiting_session_id)
        : waiting_session_id_(waiting_session_id) {}

    Awaiter(Awaiter&& rv) {
        operator=(std::move(rv));
    }
    void operator=(Awaiter&& rv) {
        waiting_session_id_ = std::move(rv.waiting_session_id_);
    }

    Awaiter(Awaiter&) = delete;
    Awaiter& operator=(Awaiter&) = delete;

    constexpr bool await_ready() const noexcept {
        return false;
    }

    void set_result(std::unique_ptr<MsgT> result) {
        reslut_ = std::move(result);
    }
    SessionId get_waiting() {
        return waiting_session_id_;
    }

    // 通过在协程中 co_await 当前类型的对象时调用
    void await_suspend(std::coroutine_handle<TaskPromise> parent_handle) noexcept;

    // co_await等待在当前对象中的协程被恢复时调用
    std::unique_ptr<MsgT> await_resume() noexcept;

private:
    SessionId waiting_session_id_;
    std::unique_ptr<MsgT> reslut_;
};

// 协程的返回值类，这里不做返回值支持
struct Task {
    using promise_type = TaskPromise;

    explicit Task(std::coroutine_handle<promise_type> handle) noexcept
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

    void await_suspend(std::coroutine_handle<promise_type> parent_handle) noexcept;

    void await_resume() noexcept;

    std::coroutine_handle<promise_type> handle;
};

// 管理协程运行、保存协程状态的类
struct TaskPromise {

    // 协程立即执行
    std::suspend_never initial_suspend() {
        return {};
    };

    // 执行结束后挂起
    std::suspend_always final_suspend() noexcept {
        return {};
    }

    // 创建协程同时调用，提前创建返回值
    Task get_return_object() {
        return Task{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }

    // co_return时实际返回值
    void return_void() {
        
    }

    void unhandled_exception() {

    }

    // 该协程内，可通过co_await进行等待的类型支持
    // 这里的T为了支持在协程里，可以等待不同类型的Awaiter
    template <typename T>
    Awaiter<T> await_transform(Awaiter<T>&& awaiter) {
        return Awaiter<T>(std::move(awaiter));
    }

    // 顺便支持一下等待Task
    Task await_transform(Task&& task) {
        return Task(std::move(task));
    }

    void set_awaiter(Awaiter<IMsg>* awaiter) {
        awaiter_ = reinterpret_cast<Awaiter<IMsg>*>(awaiter);
    }

    Awaiter<IMsg>* get_awaiter() {
        return reinterpret_cast<Awaiter<IMsg>*>(awaiter_);
    }

private:
    Awaiter<IMsg>* awaiter_;  // 最终需要唤醒的等待器
};

template <typename MsgT>
void Awaiter<MsgT>::await_suspend(std::coroutine_handle<TaskPromise> parent_handle) noexcept {
    // 何时resume交给调度器，设计上的等待是必定挂起的
    parent_handle.promise().set_awaiter(this);
}

template <typename MsgT>
std::unique_ptr<MsgT> Awaiter<MsgT>::await_resume() noexcept {
    // 调度器恢复了当前awaiter的执行，说明已经等到结果了
    return std::unique_ptr<MsgT>(static_cast<MsgT*>(reslut_.release()));
}

inline void Task::await_suspend(std::coroutine_handle<TaskPromise> parent_handle) noexcept {
    // 这里的参数是parent coroutine的handle
    // 向上设置awaiter，让service可以拿到正在等待的awaiter，选择是否调度当前协程
    parent_handle.promise().set_awaiter(handle.promise().get_awaiter());
}

inline void Task::await_resume() noexcept {
    // parent_handle被调度器恢复，继续向下唤醒，直到唤醒Awaiter
    handle.resume();
}

} // namespace million