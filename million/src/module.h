#pragma once

#include <filesystem>

#include "detail/dl.h"

#include <million/detail/noncopyable.h>

namespace million {

class Million;

using ModuleInitFunc = bool(Million*);
using ModuleExitFunc = void(Million*);

constexpr std::string_view kModuleInitName = "MillionModuleInit";
constexpr std::string_view kModuleExitName = "MillionModuleExit";

class Module : noncopyable {
public:
    Module(Million* million, const std::filesystem::path& module_file_path);
    ~Module();

    bool Loaded();

    bool Init();

private:
    Million* million_;
    detail::Dll dll_;
    bool init_ = false;
};

} // namespace million