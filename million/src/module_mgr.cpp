#include "module_mgr.h"

#include "million.h"

namespace million {

ModuleMgr::ModuleMgr(Million* million)
    : million_(million) {}

bool ModuleMgr::Load(const std::string& module_dir, const std::string& module_name) {
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
        std::cerr << "[module] [error] load module '" << path << "' err:" << ec.what() << "." << std::endl;
    }
    if (!module || !module->Loaded()) {
        return false;
    }
    modules_.emplace(std::make_pair(path.string(), std::move(module)));
    return true;
}

bool ModuleMgr::Init() {
    for (auto& module : modules_) {
        if (!module.second->Init()) {
            return false;
        }
    }
}

void ModuleMgr::Start() {
    for (auto& module : modules_) {
        module.second->Start();
    }
}

void ModuleMgr::Stop() {
    for (auto& module : modules_) {
        // module.second->Stop();
    }
}

} // namespace million