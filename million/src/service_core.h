#pragma once

#include <cstdint>

#include <functional>
#include <memory>
#include <mutex>
#include <queue>

#include <million/noncopyable.h>
#include <million/iservice.h>
#include <million/service_handle.h>
#include <million/message.h>

#include "task_executor.h"

namespace million {

class ServiceMgr;
    /** \class ServiceCore
     * \brief 服务核心实现类，管理单个服务实例的生命周期和消息处理
     * 
     * 负责服务的启动、停止、消息队列管理和任务执行，
     * 维护服务在不同生命周期阶段的状态转换。
     */
class ServiceCore : public noncopyable {
public:
    /** \brief 构造函数
     * \param service_mgr 服务管理器指针
     * \param iservice 服务接口的唯一指针
     */
ServiceCore(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice);
    /** \brief 析构函数
     * 清理服务资源并释放相关内存
     */
~ServiceCore();

    /** \brief 推送消息到服务的消息队列
     * \param sender 发送方服务共享指针
     * \param session_id 会话ID
     * \param msg 消息指针
     * \return 成功推送返回true，否则返回false
     */
bool PushMsg(const ServiceShared& sender, SessionId session_id, MessagePointer msg);
    /** \brief 从消息队列弹出一条消息
     * \return 包含消息元素的optional，如果队列为空则返回nullopt
     */
std::optional<MessageElementWithStrongSender> PopMsg();
    /** \brief 检查消息队列是否为空
     * \return 队列为空返回true，否则返回false
     */
bool MsgQueueIsEmpty();

    /** \brief 处理单条消息
     * \param msg 要处理的消息元素
     */
void ProcessMsg(MessageElementWithStrongSender msg);
    /** \brief 处理多条消息
     * \param count 最多处理的消息数量
     */
void ProcessMsgs(size_t count);
    
    /** \brief 检查任务执行器是否为空
     * \return 为空返回true，否则返回false
     */
bool TaskExecutorIsEmpty() const;

    /** \brief 启用独立工作线程
     * 
     * 为服务创建独立的工作线程来处理任务，提高并发处理能力
     */
    void EnableSeparateWorker();
    /** \brief 检查是否启用了独立工作线程
     * \return 若启用独立工作线程则返回true
     */
    bool HasSeparateWorker() const;

    /** \brief 获取服务管理器指针
     * \return 服务管理器指针
     */
    ServiceMgr* service_mgr() const { return service_mgr_; }
    
    /** \brief 获取服务共享指针
     * \return 服务的共享指针
     */
    ServiceShared shared() const { return *iter_; }

    /** \brief 获取服务迭代器
     * \return 服务在列表中的迭代器
     */
    auto iter() const { return iter_; }
    /** \brief 设置服务迭代器
     * \param iter 新的服务迭代器
     */
    void set_iter(std::list<std::shared_ptr<ServiceCore>>::iterator iter) { iter_ = iter; }

    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }

    /** \brief 获取服务接口引用
     * \return IService接口引用
     */
    IService& iservice() const { assert(iservice_); return *iservice_; }

    /** \brief 启动服务
     * \param msg 启动消息指针
     * \return 成功启动返回会话ID，失败返回nullopt
     */
    std::optional<SessionId> Start(MessagePointer msg);
    /** \brief 停止服务
     * \param msg 停止消息指针
     * \return 成功停止返回会话ID，失败返回nullopt
     */
    std::optional<SessionId> Stop(MessagePointer msg);
    /** \brief 退出服务
     * \return 成功退出返回会话ID，失败返回nullopt
     */
    std::optional<SessionId> Exit();

    /** \brief 检查服务是否就绪
     * \return 服务就绪返回true
     */
    bool IsReady() const;
    bool IsStarting() const;
    /** \brief 检查服务是否正在运行
     * \return 服务运行中返回true
     */
    bool IsRunning() const;
    bool IsStopping() const;
    /** \brief 检查服务是否已停止
     * \return 服务已停止返回true
     */
    bool IsStopped() const;
    bool IsExited() const;

private:
    /** \brief 独立线程处理函数
     * 运行在独立工作线程中的消息循环和任务处理逻辑
     */
    void SeparateThreadHandle();
    /** \brief 带锁弹出消息
     * \return 包含消息元素的optional，队列为空时返回nullopt
     */
    std::optional<MessageElementWithStrongSender> PopMsgWithLock();

    /** \brief 回复消息给发送方
     * \param ele 任务元素指针，包含回复相关信息
     */
    void ReplyMsg(TaskElement* ele);

private:
    ServiceMgr* service_mgr_;
    std::list<ServiceShared>::iterator iter_; ///< 服务迭代器

    // std::optional<Task<>> on_start_task_;
    enum class ServiceStage {
        kReady,      ///< 已就绪，可以开启服务
        kStarting,   ///< 启动中，只能处理OnStart相关的消息及调度OnStart协程
        kRunning,    ///< 运行中，可以接收及处理任何消息、调度已有协程
        kStopping,   ///< 停止中，只能处理OnStart相关的消息及调度OnStart协程
        kStopped,    ///< 已停止，不会开启新协程，不会调度已有协程，会触发已有协程的超时
        kExited,     ///< 退出后，不会开启新协程，不会调度已有协程，不会触发已有协程的超时(希望执行完所有协程再退出，则需要在Stop状态等待所有协程处理完毕，即使用TaskExecutorIsEmpty)
    };
    ServiceStage stage_ = ServiceStage::kReady;

    bool in_queue_ = false; ///< 标记服务是否在消息处理队列中

    std::mutex msgs_mutex_;  ///< 消息队列互斥锁，保护msgs_的线程安全访问
    std::queue<MessageElementWithStrongSender> msgs_; ///< 消息队列，存储待处理的消息元素

    // 允许指定某个任务执行完成之前，其他任务不允许并发执行
    // SessionId lock_task_ = kSessionIdInvalid;

    TaskExecutor excutor_;   ///< 任务执行器，负责调度和执行服务任务

    std::unique_ptr<IService> iservice_; ///< 服务接口实现的唯一指针

    /** \struct SeparateWorker
     * \brief 独立工作线程结构体
     * 
     * 封装独立工作线程及其条件变量，用于异步处理服务任务
     */
    struct SeparateWorker {
        std::thread thread;       ///< 工作线程实例
        std::condition_variable cv; ///< 线程条件变量
        /** \brief 独立工作线程构造函数
         * \param func 线程执行函数
         */
        SeparateWorker(const std::function<void()>& func) : thread(func) { thread.detach(); }
    };
    std::unique_ptr<SeparateWorker> separate_worker_; ///< 独立工作线程实例指针
};

} // namespace million