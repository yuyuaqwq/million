#pragma once

#include <unordered_map>

#include <million/noncopyable.h>

#include "module.h"

namespace million {

class Million;
class ModuleMgr {
public:
    ModuleMgr(Million* million);

    bool Load(const std::string& module_dir, const std::string& module_name);

    bool Init();
    void Start();
    void Stop();

private:
    Million* million_;
    std::unordered_map<std::string, std::unique_ptr<Module>> modules_;
};

} // namespace million