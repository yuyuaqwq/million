#pragma once

#include <string_view>
#include <memory>
#include <unordered_map>

#include "milinet/detail/noncopyable.h"
#include "milinet/module.h"

namespace milinet {

class Milinet;
class ModuleMgr {
public:
    ModuleMgr(Milinet* milinet, std::string_view module_dir_path)
        : milinet_(milinet)
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
        auto module = std::make_unique<Module>(milinet_, path);
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
    Milinet* milinet_;
    std::string module_dir_path_;
    std::unordered_map<std::string, std::unique_ptr<Module>> modules_;
};

} // namespace milinet