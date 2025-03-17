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

static ServiceHandle s_js_module_service;

std::optional<ServiceHandle> NewJsService(IMillion* imillion, std::string_view package) {
    auto lock = s_js_module_service.lock();
    auto ptr = s_js_module_service.get_ptr<JsModuleService>(lock);

    auto handle = imillion->NewService<JsService>(ptr, std::string(package));
    if (!handle) {
        return std::nullopt;
    }

    // imillion->StartService<JsServiceLoadScriptMsg>(*handle, std::string(package));
    return std::nullopt;
}

extern "C" MILLION_JSSVR_API bool MillionModuleInit(IMillion* imillion) {
    auto handle = imillion->NewService<JsModuleService>();
    s_js_module_service = *handle;

    auto settings = imillion->YamlSettings();
    const auto& jssvr_settings = settings["jssvr"];
    if (!jssvr_settings) {
        imillion->logger().Err("cannot find 'jssvr'.");
        return false;
    }

    const auto& bootstarp_settings = jssvr_settings["bootstarp"];
    if (bootstarp_settings) {
        auto bootstarp = bootstarp_settings.as<std::string>();

        handle = NewJsService(imillion, bootstarp);
    }

    return true;
}

} // namespace jssvr
} // namespace million