#pragma once

#include <cstdint>

#include <memory>

#include <million/imillion.h>

namespace million {

class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error("config error: " + message) {}
};

class ServiceMgr;
class SessionMgr;
class SessionMonitor;
class ModuleMgr;
class WorkerMgr;
class IoContextMgr;
class Timer;
class Million : public IMillion {
public:
    Million(std::string_view config_path);
    ~Million();

    void Init();
    void Start();
    void Stop();

    virtual ServiceHandle AddService(std::unique_ptr<IService> iservice) override;
    using IMillion::NewService;

    virtual SessionId Send(SessionId session_id, ServiceHandle sender, ServiceHandle target, MsgUnique msg);
    virtual SessionId Send(ServiceHandle sender, ServiceHandle target, MsgUnique msg) override;
    using IMillion::Send;

    virtual void Timeout(uint32_t tick, ServiceHandle service, MsgUnique msg) override;

    virtual asio::io_context& NextIoContext() override;

    virtual const YAML::Node& YamlConfig() const override;

    auto& service_mgr() { assert(service_mgr_); return *service_mgr_; }
    auto& session_mgr() { assert(session_mgr_); return *session_mgr_; }
    auto& session_monitor() { assert(session_monitor_); return *session_monitor_; }
    auto& module_mgr() { assert(module_mgr_); return *module_mgr_; }
    auto& worker_mgr() { assert(worker_mgr_); return *worker_mgr_; }
    auto& io_context_mgr() { assert(io_context_mgr_); return *io_context_mgr_; }

private:
    std::unique_ptr<YAML::Node> config_;

    std::unique_ptr<ServiceMgr> service_mgr_;
    std::unique_ptr<SessionMgr> session_mgr_;
    std::unique_ptr<SessionMonitor> session_monitor_;
    std::unique_ptr<ModuleMgr> module_mgr_;
    std::unique_ptr<WorkerMgr> worker_mgr_;
    std::unique_ptr<IoContextMgr> io_context_mgr_;
    std::unique_ptr<Timer> timer_;
};

} // namespace million