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

bool Million::Init(std::string_view config_path) {
    config_ = std::make_unique<YAML::Node>(YAML::LoadFile(std::string(config_path)));
    const auto& config = *config_;

    std::cout << "[million] [info] init start." << std::endl;

    do {
        service_mgr_ = std::make_unique<ServiceMgr>(this);
        session_mgr_ = std::make_unique<SessionMgr>(this);
        logger_ = std::make_unique<Logger>(this);


        std::cout << "[worker_mgr] [info] load 'worker_mgr' config." << std::endl;

        const auto& worker_mgr_config = config["worker_mgr"];
        if (!worker_mgr_config) {
            std::cerr << "[worker_mgr] [error] cannot find 'worker_mgr'." << std::endl;
            return false;
        }
        if (!worker_mgr_config["num"]) {
            std::cerr << "[worker_mgr] [error] cannot find 'worker_mgr.num'." << std::endl;
            return false;
        }
        auto worker_num = worker_mgr_config["num"].as<size_t>();
        worker_mgr_ = std::make_unique<WorkerMgr>(this, worker_num);


        std::cout << "[io_context_mgr] [info] load 'io_context_mgr' config." << std::endl;

        const auto& io_context_mgr_config = config["io_context_mgr"];
        if (!io_context_mgr_config) {
            std::cerr << "[io_context_mgr] [error] cannot find 'io_context_mgr'." << std::endl;
            break;
        }
        if (!io_context_mgr_config["num"]) {
            std::cerr << "[io_context_mgr] [error] cannot find 'io_context_mgr.num'." << std::endl;
            break;
        }
        auto io_context_num = io_context_mgr_config["num"].as<size_t>();
        io_context_mgr_ = std::make_unique<IoContextMgr>(this, io_context_num);


        std::cout << "[module] [info] load 'module' config." << std::endl;

        const auto& module_config = config["module"];
        if (!module_config) {
            std::cerr << "[module] [error] cannot find 'module'." << std::endl;
            break;
        }
        if (!module_config["dirs"]) {
            std::cerr << "[module] [error] cannot find 'module.dirs'." << std::endl;
            break;
        }
        auto module_dirs = module_config["dirs"].as<std::vector<std::string>>();
        module_mgr_ = std::make_unique<ModuleMgr>(this, module_dirs);
        const auto& loads = module_config["loads"];
        if (loads) {
            bool success = true;
            for (auto name_config : loads) {
                auto name = name_config.as<std::string>();
                if (!module_mgr_->Load(name)) {
                    std::cerr << "[module] [error] load module '" << name << "' failed." << std::endl;
                    success = false;
                    break;
                }
                else {
                    std::cout << "[module] [info] load module '" << name << "' success." << std::endl;
                }
            }
            if (!success) {
                break;
            }
        }


        std::cout << "[session_monitor] [info] load 'session_monitor' config." << std::endl;

        const auto& session_monitor_config = config["session_monitor"];
        if (!session_monitor_config) {
            std::cerr << "[session_monitor] [error] cannot find 'session_monitor_config'." << std::endl;
            break;
        }
        if (!session_monitor_config["s_per_tick"]) {
            std::cerr << "[session_monitor] [error] cannot find 'session_monitor_config.s_per_tick'." << std::endl;
            break;
        }
        auto tick_s = session_monitor_config["s_per_tick"].as<uint32_t>();
        if (!session_monitor_config["timeout_tick"]) {
            std::cerr << "[session_monitor] [error] cannot find 'session_monitor_config.timeout_tick'." << std::endl;
            break;
        }
        auto timeout_s = session_monitor_config["timeout_tick"].as<uint32_t>();
        session_monitor_ = std::make_unique<SessionMonitor>(this, tick_s, timeout_s);


        std::cout << "[timer] [info] load 'timer' config." << std::endl;

        const auto& timer_config = config["timer"];
        if (!timer_config) {
            std::cerr << "[timer] [error] cannot find 'timer'." << std::endl;
            break;
        }
        if (!timer_config["ms_per_tick"]) {
            std::cerr << "[timer] [error] cannot find 'timer.ms_per_tick'." << std::endl;
            break;
        }
        auto ms_per_tick = timer_config["ms_per_tick"].as<uint32_t>();
        timer_ = std::make_unique<Timer>(this, ms_per_tick);

        if (!logger_->Init()) {
            break;
        }
        module_mgr_->Init();

        std::cout << "[million] [error] init success." << std::endl;
        return true;

    } while (false);

    std::cout << "[million] [info] init failed." << std::endl;

    return false;
}

void Million::Start() {
    worker_mgr_->Start();
    io_context_mgr_->Start();
    session_monitor_->Start();
    timer_->Start();
}

void Million::Stop() {
    if (timer_) timer_->Stop();
    if (service_mgr_) service_mgr_->Stop();
    if (worker_mgr_) worker_mgr_->Stop();
    if (io_context_mgr_) io_context_mgr_->Stop();
    if (session_monitor_) session_monitor_->Stop();
}


std::optional<ServiceShared> Million::AddService(std::unique_ptr<IService> iservice, bool start) {
    return service_mgr_->AddService(std::move(iservice), start);
}


#undef StartService
SessionId Million::StartService(const ServiceShared& service) {
    return service_mgr_->StartService(service);
}

SessionId Million::StopService(const ServiceShared& service) {
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


bool Million::SendTo(const ServiceShared& sender, const ServiceShared& target, SessionId session_id, MsgUnique msg) {
    return service_mgr_->Send(sender, target, session_id, std::move(msg));
}

SessionId Million::Send(const ServiceShared& sender, const ServiceShared& target, MsgUnique msg) {
    auto session_id = session_mgr_->NewSession();
    if (SendTo(sender, target, session_id, std::move(msg))) {
        return session_id;
    }
    return kSessionIdInvalid;
}


const YAML::Node& Million::YamlConfig() const {
    return *config_;
}

void Million::Timeout(uint32_t tick, const ServiceShared& service, MsgUnique msg) {
    timer_->AddTask(tick, service, std::move(msg));
}

asio::io_context& Million::NextIoContext() {
    return io_context_mgr_->NextIoContext().io_context();
}

void Million::EnableSeparateWorker(const ServiceShared& service) {
    service->EnableSeparateWorker();
}

} //namespace million