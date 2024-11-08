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

#include <million/api.h>

namespace million {

template <typename T = void>
struct Task;

struct TaskPromiseBase;

template <typename T = void>
struct TaskPromise;

// 会话等待器，等待一条消息
template <typename MsgT>
struct SessionAwaiter {
    explicit SessionAwaiter(SessionId waiting_session_id)
        : waiting_session_id_(waiting_session_id) {}

    SessionAwaiter(SessionAwaiter&& rv) noexcept {
        operator=(std::move(rv));
    }
    void operator=(SessionAwaiter&& rv) noexcept {
        waiting_session_id_ = std::move(rv.waiting_session_id_);
    }

    SessionAwaiter(SessionAwaiter&) = delete;
    SessionAwaiter& operator=(SessionAwaiter&) = delete;

    void set_result(std::unique_ptr<MsgT> result) {
        result_ = std::move(result);
    }
    SessionId waiting_session() const {
        return waiting_session_id_;
    }
    std::coroutine_handle<TaskPromiseBase> waiting_coroutine() const {
        return std::coroutine_handle<TaskPromiseBase>::from_address(waiting_coroutine_.address());
    }

    // 总是挂起
    constexpr bool await_ready() const noexcept {
        return false;
    }

    // 通过在协程中 co_await 当前类型的对象时调用
    template <typename U>
    void await_suspend(std::coroutine_handle<TaskPromise<U>> parent_coroutine) noexcept {
        // 何时resume交给调度器，设计上的等待是必定挂起的
        waiting_coroutine_ = parent_coroutine;

        // 向上设置awaiter，让service可以拿到正在等待的awaiter，来恢复等待awaiter的协程
        // 因为Task链都在挂起，需要在这里设置
        std::coroutine_handle<> parent_coroutine_void = parent_coroutine;
        do {
            auto parent_coroutine_base = std::coroutine_handle<TaskPromiseBase>::from_address(parent_coroutine_void.address());
            parent_coroutine_base.promise().set_session_awaiter(reinterpret_cast<SessionAwaiter<IMsg>*>(this));
            parent_coroutine_void = parent_coroutine_base.promise().parent_coroutine();
        } while (parent_coroutine_void);
    }

    // co_await等待在当前对象中的协程被恢复时调用
    std::unique_ptr<MsgT> await_resume() noexcept;

private:
    SessionId waiting_session_id_;
    std::coroutine_handle<> waiting_coroutine_;
    std::unique_ptr<MsgT> result_;
};

// 协程的返回值类
template <typename T = void>
struct Task {
    using promise_type = TaskPromise<T>;

    explicit Task(std::coroutine_handle<promise_type> coroutine) noexcept
        : coroutine(coroutine) {}

    ~Task() {
        if (coroutine) {
            coroutine.destroy();
        }
    }

    Task(Task&& co) noexcept
        : coroutine(std::exchange(co.coroutine, {})) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    
    bool has_exception() const;

    void rethrow_if_exception() const;

    bool await_ready() const noexcept;

    template <typename U>
    void await_suspend(std::coroutine_handle<TaskPromise<U>> parent_coroutine) noexcept {
        // 向上设置awaiter，让service可以拿到正在等待的awaiter，来恢复等待awaiter的协程
        coroutine.promise().set_parent_coroutine(parent_coroutine);
        // 可能新的co_await Task()，需要将当前Task等待的session设置给整条Task链
        std::coroutine_handle<> parent_coroutine_void = parent_coroutine;
        do {
            auto parent_coroutine_base = std::coroutine_handle<TaskPromiseBase>::from_address(parent_coroutine_void.address());
            parent_coroutine_base.promise().set_session_awaiter(coroutine.promise().session_awaiter());
            parent_coroutine_void = parent_coroutine_base.promise().parent_coroutine();
        } while (parent_coroutine_void);
    }

    T await_resume() noexcept;

    std::coroutine_handle<promise_type> coroutine;
};

struct TaskAwaiter {
    bool await_ready() noexcept {
        // 总是挂起，执行await_suspend
        return false;
    }

    template <typename U>
    void await_suspend(std::coroutine_handle<TaskPromise<U>> coroutine) noexcept;
    void await_resume() noexcept;
};

// 管理协程运行、保存协程状态的类
struct TaskPromiseBase {
    TaskPromiseBase() = default;
    ~TaskPromiseBase() = default;

    TaskPromiseBase(const TaskPromiseBase&) = delete;
    TaskPromiseBase& operator=(const TaskPromiseBase&) = delete;

    // Task协程总是立即执行
    std::suspend_never initial_suspend() {
        return {};
    };

    // Task协程退出时，唤醒父协程
    TaskAwaiter final_suspend() noexcept {
        return {};
    }


    void unhandled_exception() {
        exception_ = std::current_exception();  // 捕获异常并存储
    }

    // 该协程内，可通过co_await进行等待的类型支持
    template <typename U>
    SessionAwaiter<U> await_transform(SessionAwaiter<U>&& awaiter) {
        return SessionAwaiter<U>(std::move(awaiter));
    }

    // Task等待支持
    template <typename U>
    Task<U> await_transform(Task<U>&& task) {
        // 期望等待的Task可能是异常退出的，则继续上抛
        task.rethrow_if_exception();
        return Task<U>(std::move(task));
    }

    void set_session_awaiter(SessionAwaiter<IMsg>* awaiter) {
        session_awaiter_ = reinterpret_cast<SessionAwaiter<IMsg>*>(awaiter);
    }
    SessionAwaiter<IMsg>* session_awaiter() const {
        return reinterpret_cast<SessionAwaiter<IMsg>*>(session_awaiter_);
    }

    std::coroutine_handle<> parent_coroutine() const { return parent_coroutine_; }
    void set_parent_coroutine(std::coroutine_handle<> parent_coroutine) { parent_coroutine_ = parent_coroutine; }

    std::exception_ptr exception() const {
        return exception_;
    }
    void set_exception(std::exception_ptr exception) {
        exception_ = exception;
    }

private:
    std::coroutine_handle<> parent_coroutine_;
    std::exception_ptr exception_ = nullptr;
    SessionAwaiter<IMsg>* session_awaiter_ = nullptr;  // 最终需要唤醒的等待器
};

template <typename T>
struct TaskPromise : public TaskPromiseBase {
    Task<T> get_return_object() {
        return Task<T>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }

    void return_value(T&& value) noexcept {
        result_value = std::forward<T>(value);
    }

    std::optional<T> result_value;
};

// 针对 void 类型的特化，只处理 void 特有的部分
template <>
struct TaskPromise<void> : public TaskPromiseBase {
    Task<void> get_return_object() {
        return Task<void>{ std::coroutine_handle<TaskPromise<void>>::from_promise(*this) };
    }

    void return_void() noexcept {}
};

//template <typename MsgT>
//void SessionAwaiter<MsgT>::await_suspend(std::coroutine_handle<TaskPromise<>> parent_coroutine) noexcept {
//    // 何时resume交给调度器，设计上的等待是必定挂起的
//    waiting_coroutine_ = parent_coroutine;
//
//    // 向上设置awaiter，让service可以拿到正在等待的awaiter，来恢复等待awaiter的协程
//    // 因为Task链都在挂起，需要在这里设置
//    do {
//        parent_coroutine.promise().set_session_awaiter(reinterpret_cast<SessionAwaiter<IMsg>*>(this));
//        parent_coroutine = parent_coroutine.promise().parent_coroutine();
//    } while (parent_coroutine);
//}

template <typename MsgT>
std::unique_ptr<MsgT> SessionAwaiter<MsgT>::await_resume() noexcept {
    // 调度器恢复了等待当前awaiter的协程，说明已经等到结果了
    return std::unique_ptr<MsgT>(static_cast<MsgT*>(result_.release()));
}

template <typename T>
inline bool Task<T>::has_exception() const {
    return coroutine.promise().exception() != nullptr;
}

template <typename T>
inline void Task<T>::rethrow_if_exception() const {
    if (has_exception()) std::rethrow_exception(coroutine.promise().exception());
}

template <typename T>
inline bool Task<T>::await_ready() const noexcept {
    // co_await Task() 时调用，检查下Task内是否co_await
    if (!coroutine.promise().session_awaiter()) {
        // 没有等在一个会话中，无需挂起，会直接调用await_resume，跳过await_suspend
        return true;
    }
    // 等待会话，会调用await_suspend
    return false;
}

//template <typename T, typename U>
//inline void Task<T>::await_suspend(std::coroutine_handle<TaskPromise<U>> parent_coroutine) noexcept {
//    // 向上设置awaiter，让service可以拿到正在等待的awaiter，来恢复等待awaiter的协程
//    coroutine.promise().set_parent_coroutine(parent_coroutine);
//    // 可能新的co_await Task()，需要将当前Task等待的session设置给整条Task链
//    do {
//        parent_coroutine.promise().set_session_awaiter(coroutine.promise().session_awaiter());
//        parent_coroutine = parent_coroutine.promise().parent_coroutine();
//    } while (parent_coroutine);
//}

template <typename T>
inline T Task<T>::await_resume() noexcept {
    if constexpr (!std::is_void_v<T>) {
        return std::move(*coroutine.promise().result_value);
    }
}

template <typename U>
inline void TaskAwaiter::await_suspend(std::coroutine_handle<TaskPromise<U>> coroutine) noexcept {
    // 该协程执行完毕了，需要恢复父协程的执行
    auto parent_coroutine = coroutine.promise().parent_coroutine();
    if (parent_coroutine) {
        parent_coroutine.resume();
    }
}

inline void TaskAwaiter::await_resume() noexcept {}

} // namespace million