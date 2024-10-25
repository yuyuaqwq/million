#include "million.h"

#include <cassert>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include "service_mgr.h"
#include "msg_mgr.h"
#include "module_mgr.h"
#include "worker_mgr.h"
#include "io_context.h"
#include "io_context_mgr.h"
#include "timer.h"

namespace million {

MILLION_FUNC_EXPORT void InitMillion() {
    // 初始化 Protobuf 库
    // GOOGLE_PROTOBUF_VERIFY_VERSION;

    // 清理 Protobuf 资源
    // google::protobuf::ShutdownProtobufLibrary();
}

MILLION_FUNC_EXPORT IMillion* NewMillion(std::string_view config_path) {
    auto mili = new Million(config_path);
    mili->Init();
    mili->Start();
    return mili;
}

MILLION_FUNC_EXPORT void DeleteMillion(IMillion* mili) {
    delete mili;
}

Million::Million(std::string_view config_path) {
    config_ = std::make_unique<YAML::Node>(YAML::LoadFile(std::string(config_path)));
    auto config = *config_;

    service_mgr_ = std::make_unique<ServiceMgr>(this);
    msg_mgr_ = std::make_unique<MsgMgr>(this);

    auto worker_mgr_config = config["worker_mgr"];
    if (!worker_mgr_config) {
        throw ConfigException("cannot find 'worker_mgr'.");
    }
    if (!worker_mgr_config["num"]) {
        throw ConfigException("cannot find 'worker_mgr.num'.");
    }
    auto worker_num = worker_mgr_config["num"].as<size_t>();
    worker_mgr_ = std::make_unique<WorkerMgr>(this, worker_num);

    auto io_context_mgr_config = config["io_context_mgr"];
    if (!io_context_mgr_config) {
        throw ConfigException("cannot find 'io_context_mgr'.");
    }
    if (!io_context_mgr_config["num"]) {
        throw ConfigException("cannot find 'io_context_mgr.num'.");
    }
    auto io_context_num = io_context_mgr_config["num"].as<size_t>();
    io_context_mgr_ = std::make_unique<IoContextMgr>(this, io_context_num);

    auto module_config = config["module"];
    if (!module_config) {
        throw ConfigException("cannot find 'module'.");
    }
    if (!module_config["dirs"]) {
        throw ConfigException("cannot find 'module.dirs'.");
    }
    auto module_dirs = module_config["dirs"].as<std::vector<std::string>>();
    module_mgr_ = std::make_unique<ModuleMgr>(this, module_dirs);
    if (module_config["loads"]) {
        for (auto name_config : module_config["loads"]) {
            auto name = name_config.as<std::string>();
            module_mgr_->Load(name);
        }
    }

    auto timer_config = config["timer"];
    if (!timer_config) {
        throw ConfigException("cannot find 'timer'.");
    }
    if (!timer_config["ms_per_tick"]) {
        throw ConfigException("cannot find 'timer.ms_per_tick'.");
    }
    auto ms_per_tick = timer_config["ms_per_tick"].as<uint32_t>();
    timer_ = std::make_unique<Timer>(this, ms_per_tick);
}

Million::~Million() = default;

void Million::Init() {
    assert(module_mgr_);
    module_mgr_->Init();
}

void Million::Start() {
    worker_mgr_->Start();
    io_context_mgr_->Start();
    timer_->Start();
}

void Million::Stop() {
    worker_mgr_->Stop();
    io_context_mgr_->Stop();
    timer_->Stop();
}

ServiceHandle Million::AddService(std::unique_ptr<IService> iservice) {
    return service_mgr_->AddService(std::move(iservice));
}

SessionId Million::Send(ServiceHandle sender, ServiceHandle target, MsgUnique msg) {
    msg->set_session_id(msg_mgr_->AllocSessionId());
    return service_mgr_->Send(sender, target, std::move(msg));
}


asio::io_context& Million::NextIoContext() {
    return io_context_mgr_->NextIoContext().io_context();
}

void Million::AddDelayTask(detail::DelayTask&& task) {
    timer_->AddTask(std::move(task));
}

const YAML::Node& Million::YamlConfig() const {
    return *config_;
}


} //namespace million