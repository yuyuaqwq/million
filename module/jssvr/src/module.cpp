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
    try {
        auto handle = imillion->NewService<JsService>(ptr, std::string(package));
        return handle;
    }
    catch (const std::exception& e) {
        imillion->logger().Err("New JsService failed.", e.what());
    }
    return std::nullopt;
}

extern "C" MILLION_JSSVR_API bool MillionModuleInit(IMillion* imillion) {
    auto handle = imillion->NewService<JsModuleService>();
    s_js_module_service = *handle;

    auto config = imillion->YamlConfig();
    const auto& jssvr_config = config["jssvr"];
    if (!jssvr_config) {
        imillion->logger().Err("cannot find 'jssvr'.");
        return false;
    }

    const auto& bootstarp_config = jssvr_config["bootstarp"];
    if (bootstarp_config) {
        auto bootstarp = bootstarp_config.as<std::string>();

        handle = NewJsService(imillion, bootstarp);
    }

    return true;
}

} // namespace jssvr
} // namespace million