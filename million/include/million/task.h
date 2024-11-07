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

struct Task;

struct TaskPromise;

// 消息等待器，等待一条消息
template <typename MsgT>
struct Awaiter {
    explicit Awaiter(SessionId waiting_session_id)
        : waiting_session_id_(waiting_session_id) {}

    Awaiter(Awaiter&& rv) noexcept {
        operator=(std::move(rv));
    }
    void operator=(Awaiter&& rv) noexcept {
        waiting_session_id_ = std::move(rv.waiting_session_id_);
    }

    Awaiter(Awaiter&) = delete;
    Awaiter& operator=(Awaiter&) = delete;

    // 总是挂起
    constexpr bool await_ready() const noexcept {
        return false;
    }

    void set_result(std::unique_ptr<MsgT> result) {
        result_ = std::move(result);
    }
    SessionId waiting_session() const {
        return waiting_session_id_;
    }
    std::coroutine_handle<TaskPromise> waiting_coroutine() const {
        return waiting_coroutine_;
    }

    // 通过在协程中 co_await 当前类型的对象时调用
    void await_suspend(std::coroutine_handle<TaskPromise> parent_handle) noexcept;

    // co_await等待在当前对象中的协程被恢复时调用
    std::unique_ptr<MsgT> await_resume() noexcept;

private:
    SessionId waiting_session_id_;
    std::unique_ptr<MsgT> result_;
    std::coroutine_handle<TaskPromise> waiting_coroutine_;
};

// 协程的返回值类，这里不做返回值支持
struct MILLION_CLASS_API Task {
    using promise_type = TaskPromise;

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

    void await_suspend(std::coroutine_handle<promise_type> parent_coroutine) noexcept;

    void await_resume() noexcept;

    std::coroutine_handle<promise_type> coroutine;
};

// 管理协程运行、保存协程状态的类
struct MILLION_CLASS_API TaskPromise {
    TaskPromise() = default;
    ~TaskPromise() = default;

    TaskPromise(const TaskPromise&) = delete;
    TaskPromise& operator=(const TaskPromise&) = delete;

    // 协程开始时，立即执行
    std::suspend_never initial_suspend() {
        return {};
    };

    // 协程退出时，唤醒父协程
    auto final_suspend() noexcept {
        struct awaiter {
            bool await_ready() noexcept {
                return false;
            }
            void await_suspend(std::coroutine_handle<TaskPromise> coroutine) noexcept {
                // 该协程执行完毕了，需要恢复父协程的执行
                auto parent_coroutine = coroutine.promise().parent_coroutine();
                if (parent_coroutine) {
                    parent_coroutine.resume();
                }
            }
            void await_resume() noexcept {}
        };
        return awaiter{};
    }

    // 创建协程同时调用，提前创建返回值
    Task get_return_object() {
        return Task{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }

    // co_return时实际返回值
    void return_void() {
        
    }

    void unhandled_exception() {
        exception_ = std::current_exception();  // 捕获异常并存储
    }

    // 该协程内，可通过co_await进行等待的类型支持
    // 这里的T为了支持在协程里，可以等待不同类型的Awaiter
    template <typename T>
    Awaiter<T> await_transform(Awaiter<T>&& awaiter) {
        return Awaiter<T>(std::move(awaiter));
    }

    // 支持等待Task
    Task await_transform(Task&& task) {
        // 期望等待的Task可能是异常退出的，则继续上抛
        task.rethrow_if_exception();
        return Task(std::move(task));
    }

    void set_awaiter(Awaiter<IMsg>* awaiter) {
        awaiter_ = reinterpret_cast<Awaiter<IMsg>*>(awaiter);
    }
    Awaiter<IMsg>* awaiter() const {
        return reinterpret_cast<Awaiter<IMsg>*>(awaiter_);
    }

    std::coroutine_handle<TaskPromise> parent_coroutine() const { return parent_coroutine_; }
    void set_parent_coroutine(std::coroutine_handle<TaskPromise> parent_coroutine) { parent_coroutine_ = parent_coroutine; }

    std::exception_ptr exception() const {
        return exception_;
    }

    void set_exception(std::exception_ptr exception) {
        exception_ = exception;
    }

private:
    std::coroutine_handle<TaskPromise> parent_coroutine_;
    std::exception_ptr exception_ = nullptr;
    Awaiter<IMsg>* awaiter_ = nullptr;  // 最终需要唤醒的等待器
};

template <typename MsgT>
void Awaiter<MsgT>::await_suspend(std::coroutine_handle<TaskPromise> parent_coroutine) noexcept {
    // 何时resume交给调度器，设计上的等待是必定挂起的
    waiting_coroutine_ = parent_coroutine;

    // 向上设置awaiter，让service可以拿到正在等待的awaiter，来恢复等待awaiter的协程
    // 因为同一个协程，在第二次co_await时，不会走await_suspend了，需要在这里设置
    do {
        parent_coroutine.promise().set_awaiter(reinterpret_cast<Awaiter<IMsg>*>(this));
        parent_coroutine = parent_coroutine.promise().parent_coroutine();
    } while (parent_coroutine);
}

template <typename MsgT>
std::unique_ptr<MsgT> Awaiter<MsgT>::await_resume() noexcept {
    // 调度器恢复了等待当前awaiter的协程，说明已经等到结果了
    return std::unique_ptr<MsgT>(static_cast<MsgT*>(result_.release()));
}

inline bool Task::has_exception() const {
    return coroutine.promise().exception() != nullptr;
}

inline void Task::rethrow_if_exception() const {
    if (has_exception()) std::rethrow_exception(coroutine.promise().exception());
}

inline bool Task::await_ready() const noexcept {
    if (!coroutine.promise().awaiter()) {
        // 没有等待中的session，无需挂起，会直接调用await_resume，跳过await_suspend
        return true;
    }
    // 需要等待，会调用await_suspend
    return false;
}

inline void Task::await_suspend(std::coroutine_handle<TaskPromise> parent_coroutine) noexcept {
    // 向上设置awaiter，让service可以拿到正在等待的awaiter，来恢复等待awaiter的协程
    coroutine.promise().set_parent_coroutine(parent_coroutine);
    parent_coroutine.promise().set_awaiter(coroutine.promise().awaiter());
}

inline void Task::await_resume() noexcept {}

} // namespace million