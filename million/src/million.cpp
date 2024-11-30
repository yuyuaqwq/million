#include "million.h"

#include <cassert>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include "service_mgr.h"
#include "session_mgr.h"
#include "session_monitor.h"
#include "module_mgr.h"
#include "worker_mgr.h"
#include "io_context.h"
#include "io_context_mgr.h"
#include "timer.h"
#include "logger.h"

namespace million {

MILLION_FUNC_API void InitMillion() {
    // GOOGLE_PROTOBUF_VERIFY_VERSION;

    // google::protobuf::ShutdownProtobufLibrary();
}

Million::Million(IMillion* imillion)
    : imillion_(imillion) {
}

Million::~Million() = default;

bool Million::Init(std::string_view config_path) {
    config_ = std::make_unique<YAML::Node>(YAML::LoadFile(std::string(config_path)));
    const auto& config = *config_;

    service_mgr_ = std::make_unique<ServiceMgr>(this);
    session_mgr_ = std::make_unique<SessionMgr>(this);
    logger_ = std::make_unique<Logger>(this);

    const auto& worker_mgr_config = config["worker_mgr"];
    if (!worker_mgr_config) {
        std::cerr << "[config][error]cannot find 'worker_mgr'." << std::endl;
        return false;
    }
    if (!worker_mgr_config["num"]) {
        std::cerr << "[config][error]cannot find 'worker_mgr.num'." << std::endl;
        return false;
    }
    auto worker_num = worker_mgr_config["num"].as<size_t>();
    worker_mgr_ = std::make_unique<WorkerMgr>(this, worker_num);

    const auto& io_context_mgr_config = config["io_context_mgr"];
    if (!io_context_mgr_config) {
        std::cerr << "[config][error]cannot find 'io_context_mgr'." << std::endl;
        return false;
    }
    if (!io_context_mgr_config["num"]) {
        std::cerr << "[config][error]cannot find 'io_context_mgr.num'." << std::endl;
        return false;
    }
    auto io_context_num = io_context_mgr_config["num"].as<size_t>();
    io_context_mgr_ = std::make_unique<IoContextMgr>(this, io_context_num);

    const auto& module_config = config["module"];
    if (!module_config) {
        std::cerr << "[config][error]cannot find 'module'." << std::endl;
        return false;
    }
    if (!module_config["dirs"]) {
        std::cerr << "[config][error]cannot find 'module.dirs'." << std::endl;
        return false;
    }
    auto module_dirs = module_config["dirs"].as<std::vector<std::string>>();
    module_mgr_ = std::make_unique<ModuleMgr>(this, module_dirs);
    const auto& loads = module_config["loads"];
    if (loads) {
        for (auto name_config : loads) {
            auto name = name_config.as<std::string>();
            module_mgr_->Load(name);
        }
    }

    const auto& session_monitor_config = config["session_monitor"];
    if (!session_monitor_config) {
        std::cerr << "[config][error]cannot find 'session_monitor_config'." << std::endl;
        return false;
    }
    if (!session_monitor_config["s_per_tick"]) {
        std::cerr << "[config][error]cannot find 'session_monitor_config.s_per_tick'." << std::endl;
        return false;
    }
    auto tick_s = session_monitor_config["s_per_tick"].as<uint32_t>();
    if (!session_monitor_config["timeout_tick"]) {
        std::cerr << "[config][error]cannot find 'session_monitor_config.timeout_tick'." << std::endl;
        return false;
    }
    auto timeout_s = session_monitor_config["timeout_tick"].as<uint32_t>();
    session_monitor_ = std::make_unique<SessionMonitor>(this, tick_s, timeout_s);

    const auto& timer_config = config["timer"];
    if (!timer_config) {
        std::cerr << "[config][error]cannot find 'timer'." << std::endl;
        return false;
    }
    if (!timer_config["ms_per_tick"]) {
        std::cerr << "[config][error]cannot find 'timer.ms_per_tick'." << std::endl;
        return false;
    }
    auto ms_per_tick = timer_config["ms_per_tick"].as<uint32_t>();
    timer_ = std::make_unique<Timer>(this, ms_per_tick);

    if (!logger_->Init()) {
        return false;
    }
    module_mgr_->Init();

    return true;
}

void Million::Start() {
    worker_mgr_->Start();
    io_context_mgr_->Start();
    session_monitor_->Start();
    timer_->Start();
}

void Million::Stop() {
    worker_mgr_->Stop();
    io_context_mgr_->Stop();
    timer_->Stop();
}

std::optional<ServiceHandle> Million::AddService(std::unique_ptr<IService> iservice) {
    return service_mgr_->AddService(std::move(iservice));
}

void Million::DeleteService(ServiceHandle&& service_handle) {
    service_mgr_->DeleteService(std::move(service_handle));
}

bool Million::SetServiceName(const ServiceHandle& handle, const ServiceName& name) {
    return service_mgr_->SetServiceName(handle, name);
}

std::optional<ServiceHandle> Million::GetServiceByName(const ServiceName& name) {
    return service_mgr_->GetServiceByName(name);
}

SessionId Million::AllocSessionId() {
    return session_mgr_->AllocSessionId();
}

SessionId Million::Send(SessionId session_id, const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) {
    msg->set_session_id(session_id);
    return service_mgr_->Send(sender, target, std::move(msg));
}

SessionId Million::Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) {
    return Send(session_mgr_->AllocSessionId(), sender, target, std::move(msg));
}

const YAML::Node& Million::YamlConfig() const {
    return *config_;
}

void Million::Timeout(uint32_t tick, const ServiceHandle& service, MsgUnique msg) {
    timer_->AddTask(tick, service, std::move(msg));
}

asio::io_context& Million::NextIoContext() {
    return io_context_mgr_->NextIoContext().io_context();
}

void Million::Log(const ServiceHandle& sender, logger::LogLevel level, const char* file, int line, const char* function, const std::string& str) {
    logger_->Log(sender, level, file, line, function, str);
}

void Million::EnableSeparateWorker(const ServiceHandle& service) {
    assert(service.service());
    service.service()->EnableSeparateWorker();
}

} //namespace million