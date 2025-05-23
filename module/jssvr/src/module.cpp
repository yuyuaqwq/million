#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <jssvr/jssvr.h>

#include "js_service.h"

namespace million {
namespace jssvr {

static ServiceHandle js_runtime_service;

std::optional<ServiceHandle> NewJSService(IMillion* imillion, std::string_view package) {
    auto lock = js_runtime_service.lock();
    auto ptr = js_runtime_service.get_ptr<JSRuntimeService>(lock);

    auto handle = imillion->NewService<JSService>(ptr, std::string(package));
    if (!handle) {
        return std::nullopt;
    }

    // imillion->StartService<JsServiceLoadScriptMsg>(*handle, std::string(package));
    return *handle;
}

extern "C" MILLION_JSSVR_API bool MillionModuleInit(IMillion* imillion) {
    auto handle = imillion->NewService<JSRuntimeService>();
    js_runtime_service = *handle;

    auto settings = imillion->YamlSettings();
    const auto& jssvr_settings = settings["jssvr"];
    if (!jssvr_settings) {
        imillion->logger().LOG_ERROR("cannot find 'jssvr'.");
        return false;
    }

    const auto& bootstarp_settings = jssvr_settings["bootstarp"];
    if (bootstarp_settings) {
        auto bootstarp = bootstarp_settings.as<std::string>();

        handle = NewJSService(imillion, bootstarp);
    }

    return true;
}

} // namespace jssvr
} // namespace million