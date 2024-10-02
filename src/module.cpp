#include "milinet/module.h"

#include <cassert>

namespace milinet {

Module::Module(std::string_view module_name) {
    // std::filesystem::path path = module_name  + ".so";

    //dll_.Load(path);
}

Module::~Module() = default;

bool Module::Loaded() {
    return dll_.Loaded();
}

} //namespace milinet