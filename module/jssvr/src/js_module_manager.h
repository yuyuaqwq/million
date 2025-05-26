#pragma once

#include <filesystem>

#include <mjs/runtime.h>

namespace million {
namespace jssvr {

namespace fs = std::filesystem;

class JSService;
class JSModuleManager : public mjs::ModuleManagerBase {
public:
    void AddCppModule(std::string_view path, mjs::CppModuleObject* cpp_module_object) override;
    mjs::Value GetModule(mjs::Context* ctx, std::string_view path) override;
    mjs::Value GetModuleAsync(mjs::Context* ctx, std::string_view path) override;
    void ClearModuleCache() override;

private:
    // ¶ÁÈ¡Ä£¿é½Å±¾
    std::optional<std::string> ReadModuleScript(const std::filesystem::path& module_path);
    mjs::Value LoadJSModule(JSService* js_service, std::string_view module_name);
    mjs::Value FindJSModule(JSService* js_service, std::filesystem::path path);

private:
    std::vector<std::string> jssvr_dirs_;
    std::unordered_map<fs::path, mjs::Value> cpp_modules_;
    std::unordered_map<fs::path, mjs::Value> module_defs_;
};

} // namespace jssvr
} // namespace million