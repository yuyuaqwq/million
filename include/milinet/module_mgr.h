#pragma once

#include <string_view>
#include <memory>
#include <unordered_map>

#include "milinet/noncopyable.h"
#include "milinet/module.h"

namespace milinet {

class Milinet;
class ModuleMgr {
public:
    ModuleMgr(Milinet* milinet, std::string_view module_dir_path)
        : milinet_(milinet)
        , module_dir_path_(module_dir_path_) {
        
    }

    bool Load(const std::string& module_name) {
        if (modules_.find(module_name) != modules_.end()) {
            return false;
        }
        auto module_file_path = module_dir_path_ + module_name + ".so";
        auto module = std::make_unique<Module>(milinet_, module_file_path);
        if (!module->Loaded()) {
            return false;
        }
        modules_.emplace(std::make_pair(module_name, std::move(module)));
        return true;
    }

private:
    Milinet* milinet_;
    std::string module_dir_path_;
    std::unordered_map<std::string, std::unique_ptr<Module>> modules_;
};

} // namespace milinet