#pragma once

#include <cstdint>

#include <memory>

#include <million/imillion.h>

namespace million {

class SeataSnowflake;
class ServiceManager;
class SessionManager;
class SessionMonitor;
class ProtoManager;
class ModuleManager;
class WorkerManager;
class IoContextManager;
class Timer;
class Logger;
class Million {
public:
    Million(IMillion* imillion);
    ~Million();

    bool Init(std::string_view settings_path);
    void Start();
    void Stop();

    std::optional<ServiceShared> AddService(std::unique_ptr<IService> iservice);

    std::optional<SessionId> StartService(const ServiceShared& service, MessagePointer with_msg);
    std::optional<SessionId> StopService(const ServiceShared& service, MessagePointer with_msg);

    std::optional<ServiceShared> FindServiceById(ServiceId id);

    bool SetServiceNameId(const ServiceShared& service, ModuleCode name_id);
    std::optional<ServiceShared> FindServiceByNameId(ModuleCode name_id);

    SessionId NewSession();

    SnowId NextSequenceId();

    bool SendTo(const ServiceShared& sender, const ServiceShared& target, SessionId session_id, MessagePointer msg);
    std::optional<SessionId> Send(const ServiceShared& sender, const ServiceShared& target, MessagePointer msg);

    const YAML::Node& YamlSettings() const;
    void Timeout(uint32_t tick, const ServiceShared& service, MessagePointer msg);
    asio::io_context& NextIoContext();
    void EnableSeparateWorker(const ServiceShared& service);

    auto& imillion() { assert(imillion_); return *imillion_; }
    auto& node_id() { return node_id_; }
    auto& sequence_id_manager() { assert(sequence_id_manager_); return *sequence_id_manager_; }
    auto& service_manager() { assert(service_manager_); return *service_manager_; }
    auto& session_manager() { assert(session_manager_); return *session_manager_; }
    auto& session_monitor() { assert(session_monitor_); return *session_monitor_; }
    auto& logger() { assert(logger_); return *logger_; }
    auto& proto_manager() { assert(proto_manager_); return *proto_manager_; }
    auto& module_manager() { assert(module_manager_); return *module_manager_; }
    auto& worker_manager() { assert(worker_manager_); return *worker_manager_; }
    auto& io_context_manager() { assert(io_context_manager_); return *io_context_manager_; }
    auto& timer() { assert(timer_); return *timer_; }

private:
    IMillion* imillion_;

    enum Stage {
        kUninitialized,
        kReady,
        kRunning,
    };
    Stage stage_ = kUninitialized;

    NodeId node_id_;

    std::unique_ptr<YAML::Node> settings_;

    std::unique_ptr<ServiceManager> service_manager_;
    std::unique_ptr<SessionManager> session_manager_;
    std::unique_ptr<SessionMonitor> session_monitor_;
    std::unique_ptr<Logger> logger_;
    std::unique_ptr<ProtoManager> proto_manager_;
    std::unique_ptr<ModuleManager> module_manager_;
    std::unique_ptr<WorkerManager> worker_manager_;
    std::unique_ptr<IoContextManager> io_context_manager_;
    std::unique_ptr<Timer> timer_;
    std::unique_ptr<SeataSnowflake> sequence_id_manager_;
};

} // namespace million