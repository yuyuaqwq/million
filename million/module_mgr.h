#pragma once

#include <string_view>
#include <memory>
#include <unordered_map>

#include "million/detail/noncopyable.h"
#include "million/module.h"

namespace million {

class Million;
class ModuleMgr {
public:
    ModuleMgr(Million* million, std::string_view module_dir_path)
        : million_(million)
        , module_dir_path_(module_dir_path) {
        
    }

    bool Load(const std::string& module_name) {
        if (modules_.find(module_name) != modules_.end()) {
            return false;
        }
        std::filesystem::path path = module_dir_path_;
#ifdef __linux__
        path /=  module_name + ".so";
#elif WIN32
        path /= module_name + ".dll";
#endif
        auto module = std::make_unique<Module>(million_, path);
        if (!module->Loaded()) {
            return false;
        }
        modules_.emplace(std::make_pair(module_name, std::move(module)));
        return true;
    }

    void Init() {
        for (auto& module : modules_) {
            module.second->Init();
        }
    }

private:
    Million* million_;
    std::string module_dir_path_;
    std::unordered_map<std::string, std::unique_ptr<Module>> modules_;
};

} // namespace million