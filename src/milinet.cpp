#include "milinet/milinet.h"

#include <cassert>

#include <yaml-cpp/yaml.h>

#include "milinet/worker.h"

namespace milinet {

Milinet::Milinet(std::string_view config_path) {
    auto config = YAML::LoadFile(std::string(config_path));

    service_mgr_ = std::make_unique<ServiceMgr>(this);
    msg_mgr_ = std::make_unique<MsgMgr>(this);

    auto module_dir_path = config["module_path"].as<std::string>();
    module_mgr_ = std::make_unique<ModuleMgr>(this, module_dir_path);

    auto worker_num = config["worker_num"].as<size_t>();
    worker_mgr_ = std::make_unique<WorkerMgr>(this, worker_num);
}

Milinet::~Milinet() = default;

void Milinet::Start() {
    worker_mgr_->Start();
}

} //namespace milinet