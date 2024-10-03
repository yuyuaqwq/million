#pragma once

#include <string_view>

#include "milinet/detail/dl.hpp"
#include "milinet/noncopyable.h"

namespace milinet {

class Milinet;

using ModuleInitFunc = bool(Milinet*);
using ModuleExitFunc = void(Milinet*);

constexpr std::string_view kModuleInitName = "MiliModuleInit";
constexpr std::string_view kModuleExitName = "MiliModuleExit";

class Module : noncopyable {
public:
    Module(Milinet* milinet, std::string_view module_file_path);
    ~Module();

    bool Loaded();

private:
    Milinet* milinet_;
    detail::Dll dll_;
};

} // namespace milinet