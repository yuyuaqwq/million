#include "module.h"

#include <cassert>

#include "million.h"

namespace million {

Module::Module(Million* million, const std::filesystem::path& module_file_path)
    : million_(million) {
    dll_.Load(module_file_path);
}

Module::~Module() {
    if (!dll_.Loaded() || !init_) {
        return;
    }
    auto exit_func = dll_.GetFunc<ModuleExitFunc>(kModuleExitName);
    if (!exit_func) {
        return;
    }
    exit_func(&million_->imillion());
}

bool Module::Loaded() {
    return dll_.Loaded();
}

bool Module::Init() {
    assert(!init_);
    auto init_func = dll_.GetFunc<ModuleInitFunc>(kModuleInitName);
    if (!init_func) {
        return false;
    }
    init_ = init_func(&million_->imillion());
    return init_;
}

void Module::Start() {
    assert(!init_);
    auto start_func = dll_.GetFunc<ModuleStartFunc>(kModuleStartName);
    if (!start_func) {
        return;
    }
    start_func(&million_->imillion());
}

} //namespace million