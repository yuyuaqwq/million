#pragma once

#include <string_view>
#include <memory>
#include <unordered_map>

#include <million/noncopyable.h>

#include "module.h"

namespace million {

class Million;
class ModuleMgr {
public:
    ModuleMgr(Million* million, const std::vector<std::string>& module_dirs)
        : million_(million)
        , module_dirs_(module_dirs) {}

    bool Load(const std::string& module_name) {
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
            auto module = std::make_unique<Module>(million_, path);
            if (!module->Loaded()) {
                continue;
            }
            modules_.emplace(std::make_pair(module_name, std::move(module)));
            return true;
        }
        return false;
    }

    bool Init() {
        for (auto& module : modules_) {
            if (!module.second->Init()) {
                return false;
            }
        }
    }

    void Start() {
        for (auto& module : modules_) {
            module.second->Start();
        }
    }

    void Stop() {
        for (auto& module : modules_) {
            // module.second->Stop();
        }
    }

private:
    Million* million_;
    std::vector<std::string> module_dirs_;
    std::unordered_map<std::string, std::unique_ptr<Module>> modules_;
};

} // namespace million