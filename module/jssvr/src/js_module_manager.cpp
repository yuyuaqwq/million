#include "js_module_manager.h"

#include "js_service.h"

namespace million {
namespace jssvr {


void JSModuleManager::AddCppModule(std::string_view path, mjs::CppModuleObject* cpp_module_object) {
    cpp_modules_.emplace(path, mjs::Value(cpp_module_object));
}

mjs::Value JSModuleManager::GetModule(mjs::Context* ctx, std::string_view path) {
    auto iter = cpp_modules_.find(path);
    if (iter != cpp_modules_.end()) {
        return iter->second;
    }
    size_t offset = offsetof(JSService, js_context_);
    auto* js_service = reinterpret_cast<JSService*>(reinterpret_cast<char*>(ctx) - offset);
    return LoadJSModule(js_service, path);
}

mjs::Value JSModuleManager::GetModuleAsync(mjs::Context* ctx, std::string_view path) {
    return GetModule(ctx, path);
}

void JSModuleManager::ClearModuleCache() {
    module_defs_.clear();
}

// 读取模块脚本
std::optional<std::string> JSModuleManager::ReadModuleScript(const std::filesystem::path& module_path) {
    auto file = std::ifstream(module_path);
    if (!file) {
        return std::nullopt;
    }
    auto content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

mjs::Value JSModuleManager::LoadJSModule(JSService* js_service, std::string_view module_name) {
    std::filesystem::path path;
    mjs::Value module;
    auto& js_module = js_service->js_module();
    if (!js_module.IsUndefined()) {
        fs::path cur_module_path = js_module.module_def().name();

        path = cur_module_path.parent_path();
        assert(path.is_absolute());
        // 从模块所在路径找模块
        path /= module_name;
        path = path.lexically_normal();
        module = FindJSModule(js_service, path);
    }

    if (module.IsUndefined()) {
        // 当前路径找不到，去配置路径找
        auto& jssvr_dirs = js_service->js_runtime_service().jssvr_dirs();
        for (const auto& dir : jssvr_dirs) {
            path = fs::absolute(dir);
            path /= module_name;
            module = FindJSModule(js_service, path);
            if (!module.IsUndefined()) break;
        }

        if (module.IsUndefined()) {
            return mjs::Value(mjs::String::Format("LoadModule failed: {}.", module_name)).SetException();
        }
    }

    if (module.IsModuleDef()) {
        auto result = js_service->js_context().CallModule(&module);
        if (result.IsException()) {
            return result;
        }
        js_service->js_module_cache().emplace(path, module);
    }
    return module;
}

mjs::Value JSModuleManager::FindJSModule(JSService* js_service, std::filesystem::path path) {
    assert(path.is_absolute());

    try {
        path = fs::canonical(path);
    }
    catch (const fs::filesystem_error& e) {
        return mjs::Value();
    }

    // 找Context缓存是否存在此模块
    auto& context_cache = js_service->js_module_cache();
    auto iter = context_cache.find(path);
    if (iter != context_cache.end()) {
        return iter->second;
    }

    // 找Runtime缓存的模块定义
    iter = module_defs_.find(path);
    if (iter != module_defs_.end()) {
        return iter->second;
    }

    auto script = ReadModuleScript(path);
    if (!script) {
        return mjs::Value();
    }

    auto module_def = js_service->js_context().CompileModule(path.string(), *script);
    module_defs_.emplace(path, module_def);
    return module_def;
}

} // namespace jssvr
} // namespace million