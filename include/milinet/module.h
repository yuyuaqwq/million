#pragma once

#include <filesystem>

#include "milinet/detail/dl.h"
#include "milinet/noncopyable.h"

namespace milinet {

class Milinet;

using ModuleInitFunc = bool(Milinet*);
using ModuleExitFunc = void(Milinet*);

constexpr std::string_view kModuleInitName = "MiliModuleInit";
constexpr std::string_view kModuleExitName = "MiliModuleExit";

class Module : noncopyable {
public:
    Module(Milinet* milinet, const std::filesystem::path& module_file_path);
    ~Module();

    bool Loaded();

    bool Init();

private:
    Milinet* milinet_;
    detail::Dll dll_;
    bool init_ = false;
};

} // namespace milinet