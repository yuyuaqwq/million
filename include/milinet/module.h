#pragma once

#include <string_view>

#include "milinet/detail/dl.hpp"
#include "milinet/noncopyable.h"

namespace milinet {

class Milinet;
using MiliModuleInitFunc = bool(Milinet*);
using MiliModuleExitFunc = void(Milinet*);

constexpr std::string_view MiliModuleInitName = "MiliModuleInit";
constexpr std::string_view MiliModuleExitName = "MiliModuleExit";

class Module : noncopyable {
public:
    Module(std::string_view module_name);
    ~Module();

    bool Loaded();

private:
    detail::Dll dll_;
};

} // namespace milinet