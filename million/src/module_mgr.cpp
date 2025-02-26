#include "module_mgr.h"

#include "million.h"

namespace million {

ModuleMgr::ModuleMgr(Million* million, const std::vector<std::string>& module_dirs)
    : million_(million)
    , module_dirs_(module_dirs) {}

bool ModuleMgr::Load(const std::string& module_name) {
    if (modules_.find(module_name) != modules_.end()) {
        return false;
    }
    for (auto& module_dir : module_dirs_) {
        std::filesystem::path path = module_dir;
#ifdef __linux__
        path /= module_name + ".so";
#elif WIN32
        path /= module_name + ".dll";
#endif
        std::unique_ptr<Module> module;
        try {
            module = std::make_unique<Module>(million_, path);
        }
        catch (std::system_error ec) {
            std::cerr << "[module] [error] load module '" << path << "' err:" << ec.what() << "." << std::endl;
        }
        if (!module || !module->Loaded()) {
            continue;
        }
        modules_.emplace(std::make_pair(module_name, std::move(module)));
        return true;
    }
    return false;
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