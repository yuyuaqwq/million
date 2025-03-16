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

MILLION_MODULE_INIT();

namespace million {

extern "C" MILLION_API void MillionInit() {
    // GOOGLE_PROTOBUF_VERIFY_VERSION;
    // google::protobuf::ShutdownProtobufLibrary();
}

extern "C" MILLION_API void* MillionMemAlloc(size_t size) {
    // std::cout << "MillionMemAlloc: " << size << " bytes\n";
    return std::malloc(size);
}

extern "C" MILLION_API void MillionMemFree(void* ptr) {
    // std::cout << "MillionMemFree: " << ptr << "\n";
    std::free(ptr);
}


Million::Million(IMillion* imillion)
    : imillion_(imillion) {
}

Million::~Million() {
    Stop();
}

bool Million::Init(std::string_view settings_path) {
    if (stage_ != kUninitialized) {
        throw std::runtime_error("Repeat initialization.");
    }

    settings_ = std::make_unique<YAML::Node>(YAML::LoadFile(std::string(settings_path)));
    const auto& settings = *settings_;

    do {
        logger_ = std::make_unique<Logger>(this);
        if (!logger_->Init()) {
            std::cerr << "[million] logger init failed." << std::endl;
            break;
        }

        logger_->Info("init start.");

        service_mgr_ = std::make_unique<ServiceMgr>(this);
        session_mgr_ = std::make_unique<SessionMgr>(this);

        logger_->Info("load 'worker_mgr' settings.");

        const auto& worker_mgr_settings = settings["worker_mgr"];
        if (!worker_mgr_settings) {
            logger_->Err("cannot find 'worker_mgr'.");
            return false;
        }
        if (!worker_mgr_settings["num"]) {
            logger_->Err("cannot find 'worker_mgr.num'.");
            return false;
        }
        auto worker_num = worker_mgr_settings["num"].as<size_t>();
        worker_mgr_ = std::make_unique<WorkerMgr>(this, worker_num);


        logger_->Info("load 'io_context_mgr' settings.");

        const auto& io_context_mgr_settings = settings["io_context_mgr"];
        if (!io_context_mgr_settings) {
            logger_->Err("cannot find 'io_context_mgr'.");
            break;
        }
        if (!io_context_mgr_settings["num"]) {
            logger_->Err("cannot find 'io_context_mgr.num'.");
            break;
        }
        auto io_context_num = io_context_mgr_settings["num"].as<size_t>();
        io_context_mgr_ = std::make_unique<IoContextMgr>(this, io_context_num);


        logger_->Info("load 'session_monitor' settings.");

        const auto& session_monitor_settings = settings["session_monitor"];
        if (!session_monitor_settings) {
            logger_->Err("cannot find 'session_monitor_settings'.");
            break;
        }
        if (!session_monitor_settings["s_per_tick"]) {
            logger_->Err("cannot find 'session_monitor_settings.s_per_tick'.");
            break;
        }
        auto tick_s = session_monitor_settings["s_per_tick"].as<uint32_t>();
        if (!session_monitor_settings["timeout_tick"]) {
            logger_->Err("cannot find 'session_monitor_settings.timeout_tick'.");
            break;
        }
        auto timeout_s = session_monitor_settings["timeout_tick"].as<uint32_t>();
        session_monitor_ = std::make_unique<SessionMonitor>(this, tick_s, timeout_s);


        logger_->Info("load 'proto_mgr' settings.");

        proto_mgr_ = std::make_unique<ProtoMgr>();
        proto_mgr_->Init();


        logger_->Info("load 'timer' settings.");

        const auto& timer_settings = settings["timer"];
        if (!timer_settings) {
            logger_->Err("cannot find 'timer'.");
            break;
        }
        if (!timer_settings["ms_per_tick"]) {
            logger_->Err("cannot find 'timer.ms_per_tick'.");
            break;
        }
        auto ms_per_tick = timer_settings["ms_per_tick"].as<uint32_t>();
        timer_ = std::make_unique<Timer>(this, ms_per_tick);

        logger_->Info("load 'module_mgr' settings.");

        const auto& module_mgr_settings = settings["module_mgr"];
        if (!module_mgr_settings) {
            logger_->Err("cannot find 'module_mgr'.");
            break;
        }

        module_mgr_ = std::make_unique<ModuleMgr>(this);

        bool success = true;
        for (const auto& module_settings : module_mgr_settings) {
            if (!module_settings["dir"]) {
                logger_->Err("cannot find 'dir' in module settings.");
                continue; // 跳过当前模块，继续下一个
            }

            std::string module_dir = module_settings["dir"].as<std::string>();

            const auto& loads = module_settings["loads"];
            if (!loads) {
                continue;
            }
            for (const auto& name_settings : loads) {
                auto name = name_settings.as<std::string>();
                if (!module_mgr_->Load(module_dir, name)) {
                    logger_->Err("load module '{} -> {}' failed.", module_dir, name);
                    success = false;
                    break;
                }
                else {
                    logger_->Info("load module '{} -> {}' success.", module_dir, name);
                }
            }
            if (!success) {
                break;
            }
        }
        if (!success) {
            break;
        }
        if (!module_mgr_->Init()) {
            logger_->Err("module mgr init failed.");
            break;
        }

        logger_->Info("init success.");

        stage_ = kReady;
        return true;

    } while (false);

    logger_->Err("init failed.");
    return false;
}

void Million::Start() {
    if (stage_ != kReady) {
        throw std::runtime_error("Not initialized.");
    }
    worker_mgr_->Start();
    io_context_mgr_->Start();
    session_monitor_->Start();
    timer_->Start();
    module_mgr_->Start();
}

void Million::Stop() {
    if (module_mgr_) module_mgr_->Stop();
    if (timer_) timer_->Stop();
    if (service_mgr_) service_mgr_->Stop();
    if (session_monitor_) session_monitor_->Stop();
    if (io_context_mgr_) io_context_mgr_->Stop();
    if (worker_mgr_) worker_mgr_->Stop();
}


std::optional<ServiceShared> Million::AddService(std::unique_ptr<IService> iservice) {
    return service_mgr_->AddService(std::move(iservice));
}


#undef StartService
std::optional<SessionId> Million::StartService(const ServiceShared& service) {
    return service_mgr_->StartService(service);
}

std::optional<SessionId> Million::StopService(const ServiceShared& service) {
    return service_mgr_->StopService(service);
}


bool Million::SetServiceName(const ServiceShared& service, const ServiceName& name) {
    return service_mgr_->SetServiceName(service, name);
}

std::optional<ServiceShared> Million::GetServiceByName(const ServiceName& name) {
    return service_mgr_->GetServiceByName(name);
}


SessionId Million::NewSession() {
    return session_mgr_->NewSession();
}


bool Million::SendTo(const ServiceShared& sender, const ServiceShared& target, SessionId session_id, MsgPtr msg) {
    return service_mgr_->Send(sender, target, session_id, std::move(msg));
}

std::optional<SessionId> Million::Send(const ServiceShared& sender, const ServiceShared& target, MsgPtr msg) {
    auto session_id = session_mgr_->NewSession();
    if (SendTo(sender, target, session_id, std::move(msg))) {
        return session_id;
    }
    return std::nullopt;
}


const YAML::Node& Million::YamlSettings() const {
    return *settings_;
}

void Million::Timeout(uint32_t tick, const ServiceShared& service, MsgPtr msg) {
    timer_->AddTask(tick, service, std::move(msg));
}

asio::io_context& Million::NextIoContext() {
    return io_context_mgr_->NextIoContext().io_context();
}

void Million::EnableSeparateWorker(const ServiceShared& service) {
    service->EnableSeparateWorker();
}

} //namespace million