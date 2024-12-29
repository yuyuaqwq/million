#pragma once

#include <filesystem>

#include "detail/dl.h"

#include <million/noncopyable.h>

namespace million {

class Million;
class IMillion;

using ModuleInitFunc = bool(IMillion*);
using ModuleStartFunc = void(IMillion*);
using ModuleStopFunc = void(IMillion*);
using ModuleExitFunc = void(IMillion*);

constexpr std::string_view kModuleInitName = "MillionModuleInit";
constexpr std::string_view kModuleStartName = "MillionModuleStart";
constexpr std::string_view kModuleStopName = "MillionModuleStop";
constexpr std::string_view kModuleExitName = "MillionModuleExit";

class Module : noncopyable {
public:
    Module(Million* million, const std::filesystem::path& module_file_path);
    ~Module();

    bool Loaded();
    bool Init();
    void Start();

private:
    Million* million_;
    detail::Dll dll_;
    bool init_ = false;
};

} // namespace million