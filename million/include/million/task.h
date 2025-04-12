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

#ifdef MILLION_STACK_TRACE
//#include <stacktrace>
//#include <iostream>
#endif

#include <million/api.h>
#include <million/exception.h>
#include <million/session_def.h>
#include <million/msg.h>

namespace million {

template <typename T = void>
struct Task;

struct TaskPromiseBase;

template <typename T = void>
struct TaskPromise;

struct SessionAwaiterBase {
    SessionAwaiterBase(SessionId waiting_session_id, uint32_t timeout_s, bool or_null)
        : waiting_session_id_(waiting_session_id)
        , timeout_s_(timeout_s)
        , or_null_(or_null) {}

    SessionAwaiterBase(SessionAwaiterBase&& rv) noexcept {
        operator=(std::move(rv));
    }
    void operator=(SessionAwaiterBase&& rv) noexcept {
        waiting_session_id_ = std::move(rv.waiting_session_id_);
        timeout_s_ = rv.timeout_s_;
        or_null_ = rv.or_null_;
    }

    SessionAwaiterBase(SessionAwaiterBase&) = delete;
    SessionAwaiterBase& operator=(SessionAwaiterBase&) = delete;

    void set_result(MsgPtr result) {
        result_ = std::move(result);
    }

    SessionId waiting_session_id() const {
        return waiting_session_id_;
    }

    uint32_t timeout_s() const {
        return timeout_s_;
    }

    uint32_t or_null() const {
        return or_null_;
    }

    std::coroutine_handle<TaskPromiseBase> waiting_coroutine() const {
        return std::coroutine_handle<TaskPromiseBase>::from_address(waiting_coroutine_.address());
    }


    // co_await SessionAwaiterBase相关
    // co_await 开始
    constexpr bool await_ready() const noexcept {
        return false;
    }

    template <typename U>
    // co_await 挂起
    void await_suspend(std::coroutine_handle<TaskPromise<U>> parent_coroutine) noexcept {
        // 何时resume交给调度器，设计上的等待是必定挂起的
        waiting_coroutine_ = parent_coroutine;

        // 向上设置awaiter，让service可以拿到正在等待的awaiter，来恢复等待awaiter的协程
        // 因为Task链都在挂起，需要在这里设置
        std::coroutine_handle<> parent_coroutine_void = parent_coroutine;
        do {
            auto parent_coroutine_base = std::coroutine_handle<TaskPromiseBase>::from_address(parent_coroutine_void.address());
            parent_coroutine_base.promise().set_session_awaiter(this);
            parent_coroutine_void = parent_coroutine_base.promise().parent_coroutine();
        } while (parent_coroutine_void);
    }

    // co_await 恢复
    MsgPtr await_resume() {
        // 调度器恢复了等待当前awaiter的协程，说明已经等到结果/超时了
        if (!result_ && or_null_ == false) {
            // 超时，不返回nullptr
            TaskAbort("Session timeout: {}.", waiting_session_id_);
        }
        return std::move(result_);
    }

protected:
    SessionId waiting_session_id_;
    uint32_t timeout_s_;
    bool or_null_;
    std::coroutine_handle<> waiting_coroutine_;
    MsgPtr result_;
};

// 会话等待器，等待一条消息
template <typename MsgT>
struct SessionAwaiter : public SessionAwaiterBase {
    using SessionAwaiterBase::SessionAwaiterBase;

    std::unique_ptr<MsgT> await_resume() {
        auto msg = SessionAwaiterBase::await_resume();
        // 如果是基类，说明外部希望自己转换，不做类型检查
        if (msg) {
            if constexpr (std::is_same_v<MsgT, ProtoMsg>) {
                TaskAssert(msg.IsProtoMsg(), "Not ProtoMessage type: {}.", typeid(MsgT).name());
            }
            else if constexpr (std::is_same_v<MsgT, CppMsg>) {
                TaskAssert(msg.IsCppMsg(), "Not CppMessage type: {}.", typeid(MsgT).name());
            }
            else {
                TaskAssert(msg.IsType<MsgT>(), "Mismatched type: {}.", typeid(MsgT).name());
            }
        }
        return std::unique_ptr<MsgT>(static_cast<MsgT*>(msg.Release()));
    }
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

    Task& operator=(Task&& co) noexcept {
        coroutine = std::exchange(co.coroutine, {});
        return *this;
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    
    bool has_exception() const {
        return coroutine.promise().exception() != nullptr;
    }

    void rethrow_if_exception() const {
        if (has_exception()) std::rethrow_exception(coroutine.promise().exception());
    }


    // co_await Task 相关
    // co_await 开始
    bool await_ready() const noexcept {
        // co_await Task() 时调用，检查下Task内是否co_await
        if (!coroutine.promise().session_awaiter()) {
            // 没有等在一个会话中，无需挂起，会直接调用await_resume，跳过await_suspend
            return true;
        }
        // 等待会话，会调用await_suspend
        return false;
    }

    // co_await 挂起
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

    // co_await 恢复
    T await_resume() {
        rethrow_if_exception();
        if constexpr (!std::is_void_v<T>) {
            TaskAssert(coroutine.promise().result_value, "Task has no return value.");
            return std::move(*coroutine.promise().result_value);
        }
    }


    std::coroutine_handle<promise_type> coroutine;
};

struct TaskAwaiter {
    // Task 协程退出

    bool await_ready() noexcept {
        // 退出时总是挂起，执行await_suspend，不会调用await_resume
        return false;
    }

    template <typename U>
    void await_suspend(std::coroutine_handle<TaskPromise<U>> coroutine) noexcept {
        // 该协程执行完毕了，需要恢复父协程的执行
        auto parent_coroutine = coroutine.promise().parent_coroutine();
        if (parent_coroutine) {
            // 当前协程可能是异常退出，Task的await_resume需要rethrow_if_exception
            parent_coroutine.resume();
        }
    }

    void await_resume() noexcept {}
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
#ifdef MILLION_STACK_TRACE
        // std::stacktrace stacktrace = std::stacktrace::current();
        // std::cout << stacktrace << std::endl;
#endif
    }

    // 可等待对象类型支持
    SessionAwaiterBase await_transform(SessionAwaiterBase&& awaiter) {
        return SessionAwaiterBase(std::move(awaiter));
    }

    template <typename U>
    SessionAwaiter<U> await_transform(SessionAwaiter<U>&& awaiter) {
        return SessionAwaiter<U>(std::move(awaiter));
    }

    template <typename U>
    Task<U> await_transform(Task<U>&& task) {
        // 期望等待的Task可能是异常退出的，则继续上抛
        task.rethrow_if_exception();
        return Task<U>(std::move(task));
    }


    void set_session_awaiter(SessionAwaiterBase* awaiter) {
        session_awaiter_ = awaiter;
    }
    SessionAwaiterBase* session_awaiter() const {
        return session_awaiter_;
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
    SessionAwaiterBase* session_awaiter_ = nullptr;  // 最终需要唤醒的等待器
};

template <typename T>
struct TaskPromise : public TaskPromiseBase {
    Task<T> get_return_object() {
        return Task<T>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }

    template <typename U>
    void return_value(U&& value) noexcept {
        result_value.emplace(std::forward<U>(value));
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

} // namespace million