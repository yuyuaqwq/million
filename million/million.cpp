#include "million/million.h"

#include <cassert>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include "million/service_mgr.h"
#include "million/msg_mgr.h"
#include "million/module_mgr.h"
#include "million/worker_mgr.h"
#include "million/io_context.h"
#include "million/io_context_mgr.h"

namespace million {

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

    if (!config["worker_num"]) {
        throw ConfigException("cannot find 'worker_num'.");
    }
    auto worker_num = config["worker_num"].as<size_t>();
    worker_mgr_ = std::make_unique<WorkerMgr>(this, worker_num);

    if (!config["io_context_num"]) {
        throw ConfigException("cannot find 'io_context_num'.");
    }
    auto io_context_num = config["io_context_num"].as<size_t>();
    io_context_mgr_ = std::make_unique<IoContextMgr>(this, io_context_num);

    if (!config["module_path"]) {
        throw ConfigException("cannot find 'module_path'.");
    }
    auto module_dir_path = config["module_path"].as<std::string>();
    module_mgr_ = std::make_unique<ModuleMgr>(this, module_dir_path);
    if (config["modules"]) {
        for (auto name_config : config["modules"]) {
            auto name = name_config.as<std::string>();
            module_mgr_->Load(name);
        }
    }
}

Million::~Million() = default;

void Million::Init() {
    assert(module_mgr_);
    module_mgr_->Init();
}

void Million::Start() {
    assert(worker_mgr_);
    worker_mgr_->Start();
}

ServiceHandle Million::AddService(std::unique_ptr<IService> iservice) {
    return service_mgr_->AddService(std::move(iservice));
}

SessionId Million::Send(ServiceHandle target, MsgUnique msg) {
    msg->set_session_id(msg_mgr_->AllocSessionId());
    return service_mgr_->Send(target, std::move(msg));
}

const YAML::Node& Million::config() const {
    return *config_;
}

asio::io_context& Million::NextIoContext() {
    return io_context_mgr_->NextIoContext().io_context();
}

} //namespace million