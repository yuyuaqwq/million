#include "milinet/milinet.h"

#include <cassert>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include "milinet/service_mgr.h"
#include "milinet/msg_mgr.h"
#include "milinet/module_mgr.h"
#include "milinet/worker_mgr.h"

namespace milinet {

Milinet::Milinet(std::string_view config_path) {
    auto config = YAML::LoadFile(std::string(config_path));

    service_mgr_ = std::make_unique<ServiceMgr>(this);
    msg_mgr_ = std::make_unique<MsgMgr>(this);

    if (!config["module_path"]) {
        throw ConfigException("cannot find 'module_path'.");
    }
    auto module_dir_path = config["module_path"].as<std::string>();
    module_mgr_ = std::make_unique<ModuleMgr>(this, module_dir_path);

    if (!config["worker_num"]) {
        throw ConfigException("cannot find 'worker_num'.");
    }
    auto worker_num = config["worker_num"].as<size_t>();
    worker_mgr_ = std::make_unique<WorkerMgr>(this, worker_num);

    if (config["modules"]) {
        for (auto name_config : config["modules"]) {
            auto name = name_config.as<std::string>();
            module_mgr_->Load(name);
        }
    }
}

Milinet::~Milinet() = default;

void Milinet::Init() {
    assert(module_mgr_);
    module_mgr_->Init();
}

void Milinet::Start() {
    assert(worker_mgr_);
    worker_mgr_->Start();
}

ServiceId Milinet::CreateService(std::unique_ptr<IService> iservice) {
    return service_mgr_->AddService(std::move(iservice));
}

SessionId Milinet::Send(ServiceId service_id, MsgUnique msg) {
    msg->set_session_id(msg_mgr_->AllocSessionId());
    return service_mgr_->Send(service_id, std::move(msg));
}

} //namespace milinet