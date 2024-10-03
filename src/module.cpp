#include "milinet/module.h"

#include <cassert>

namespace milinet {

Module::Module(Milinet* milinet, std::string_view module_file_path)
    : milinet_(milinet) {
    dll_.Load(module_file_path);
    if (!dll_.Loaded()) {
        return;
    }
    auto init_func = dll_.GetFunc<ModuleInitFunc>(kModuleInitName);
    if (!init_func || !init_func(milinet)) {
        dll_.Unload();
        return;
    }
}

Module::~Module() {
    if (!dll_.Loaded()) {
        return;
    }
    auto exit_func = dll_.GetFunc<ModuleExitFunc>(kModuleExitName);
    if (!exit_func) {
        return;
    }
    exit_func(milinet_);
}

bool Module::Loaded() {
    return dll_.Loaded();
}

} //namespace milinet