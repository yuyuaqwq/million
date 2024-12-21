#pragma once

#include <cstdint>

#include <memory>

#include <million/imillion.h>

namespace million {

class ServiceMgr;
class SessionMgr;
class SessionMonitor;
class ModuleMgr;
class WorkerMgr;
class IoContextMgr;
class Timer;
class Logger;
class Million {
public:
    Million(IMillion* imillion);
    ~Million();

    bool Init(std::string_view config_path);
    void Start();
    void Stop();

    std::optional<ServiceShared> AddService(std::unique_ptr<IService> iservice, MsgUnique init_msg);

    SessionId StartService(const ServiceShared& service);
    SessionId StopService(const ServiceShared& service);

    bool SetServiceName(const ServiceShared& service, const ServiceName& name);
    std::optional<ServiceShared> GetServiceByName(const ServiceName& name);

    SessionId NewSession();

    bool SendTo(const ServiceShared& sender, const ServiceShared& target, SessionId session_id, MsgUnique msg);
    SessionId Send(const ServiceShared& sender, const ServiceShared& target, MsgUnique msg);

    const YAML::Node& YamlConfig() const;
    void Timeout(uint32_t tick, const ServiceShared& service, MsgUnique msg);
    asio::io_context& NextIoContext();
    void EnableSeparateWorker(const ServiceShared& service);

    auto& imillion() { assert(imillion_); return *imillion_; }
    auto& service_mgr() { assert(service_mgr_); return *service_mgr_; }
    auto& session_mgr() { assert(session_mgr_); return *session_mgr_; }
    auto& session_monitor() { assert(session_monitor_); return *session_monitor_; }
    auto& logger() { assert(logger_); return *logger_; }
    auto& module_mgr() { assert(module_mgr_); return *module_mgr_; }
    auto& worker_mgr() { assert(worker_mgr_); return *worker_mgr_; }
    auto& io_context_mgr() { assert(io_context_mgr_); return *io_context_mgr_; }
    auto& timer() { assert(timer_); return *timer_; }

private:
    IMillion* imillion_;

    std::unique_ptr<YAML::Node> config_;

    std::unique_ptr<ServiceMgr> service_mgr_;
    std::unique_ptr<SessionMgr> session_mgr_;
    std::unique_ptr<SessionMonitor> session_monitor_;
    std::unique_ptr<Logger> logger_;
    std::unique_ptr<ModuleMgr> module_mgr_;
    std::unique_ptr<WorkerMgr> worker_mgr_;
    std::unique_ptr<IoContextMgr> io_context_mgr_;
    std::unique_ptr<Timer> timer_;
};

} // namespace million