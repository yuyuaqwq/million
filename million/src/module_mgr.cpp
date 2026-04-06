#include "module_mgr.h"

#include "million.h"

namespace million {

ModuleManager::ModuleManager(Million* million)
    : million_(million) {}

bool ModuleManager::Load(const std::string& module_dir, const std::string& module_name) {
    std::filesystem::path path = module_dir;
#ifdef __linux__
    path /= module_name + ".so";
#elif WIN32
    path /= module_name + ".dll";
#endif

    if (modules_.find(path.string()) != modules_.end()) {
        return false;
    }

    std::unique_ptr<Module> module;
    try {
        module = std::make_unique<Module>(million_, path);
    }
    catch (std::system_error ec) {
        million_->logger().LOG_ERROR("load module '{}' err: {}", path.string(), ec.what());
    }
    if (!module || !module->Loaded()) {
        return false;
    }
    modules_.emplace(std::make_pair(path.string(), std::move(module)));
    return true;
}

bool ModuleManager::Init() {
    for (auto& module : modules_) {
        if (!module.second->Init()) {
            million_->logger().LOG_ERROR("module init '{}' failed.", module.first);
            return false;
        }
    }
    return true;
}

void ModuleManager::Start() {
    for (auto& module : modules_) {
        module.second->Start();
    }
}

void ModuleManager::Stop() {
    for (auto& module : modules_) {
        // module.second->Stop();
    }
}

} // namespace million