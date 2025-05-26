#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/value.h>
#include <mjs/object.h>
#include <mjs/object_impl/array_object.h>
#include <mjs/object_impl/module_object.h>
#include <mjs/object_impl/promise_object.h>
#include <mjs/object_impl/cpp_module_object.h>

#include <yaml-cpp/yaml.h>

#include <db/db.h>
#include <config/config.h>

#include <million/imillion.h>

#include <jssvr/api.h>
#include "js_util.h"

namespace million {
namespace jssvr {

namespace fs = std::filesystem;


class JSRuntimeService;
inline JSRuntimeService& GetJSRuntineService(mjs::Runtime* runtime);
class JSService;
inline JSService& GetJSService(mjs::Context* context);

// 服务函数上下文，用于处理异步调用
struct ServiceFuncContext {
    //mjs::Value promise_cap;
    //mjs::Value resolving_funcs[2];
    ServiceHandle sender;
    std::optional<SessionId> waiting_session_id;
};


class JSService;
class JSModuleManager : public mjs::ModuleManagerBase {
public:
    void AddCppModule(std::string_view path, mjs::CppModuleObject* cpp_module_object) override;
    mjs::Value GetModule(mjs::Context* ctx, std::string_view path) override;
    mjs::Value GetModuleAsync(mjs::Context* ctx, std::string_view path) override;
    void ClearModuleCache() override;

private:
    // 读取模块脚本
    std::optional<std::string> ReadModuleScript(const std::filesystem::path& module_path);
    mjs::Value LoadJSModule(JSService* js_service, std::string_view module_name);
    mjs::Value FindJSModule(JSService* js_service, std::filesystem::path path);

private:
    std::vector<std::string> jssvr_dirs_;
    std::unordered_map<fs::path, mjs::Value> cpp_modules_;
    std::unordered_map<fs::path, mjs::Value> module_defs_;
};

// JS运行时服务
class JSRuntimeService : public IService {
    MILLION_SERVICE_DEFINE(JSRuntimeService);

public:
    using Base = IService;
    JSRuntimeService(IMillion* imillion)
        : Base(imillion)
        , js_runtime_(std::make_unique<JSModuleManager>()) {}


    mjs::Runtime& js_runtime() {
        return js_runtime_;
    }

    auto& jssvr_dirs() {
        return jssvr_dirs_;
    }

private:
    virtual bool OnInit() override;

    Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override {
        db_handle_ = *imillion().GetServiceByName(db::kDBServiceName);
        config_handle_ = *imillion().GetServiceByName(config::kConfigServiceName);
        co_return nullptr;
    }


private:
    friend JSRuntimeService& GetJSRuntineService(mjs::Runtime* runtime);
    mjs::Runtime js_runtime_;
    std::vector<std::string> jssvr_dirs_;
    ServiceHandle db_handle_;
    ServiceHandle config_handle_;
};


// JS值消息，用于在C++和JS之间传递值
MILLION_MESSAGE_DEFINE(, JSValueMsg, (mjs::Value) value)

// JS服务，负责执行JS代码和处理消息
class JSService : public IService {
    MILLION_SERVICE_DEFINE(JSService);

public:
    using Base = IService;
    JSService(IMillion* imillion, JSRuntimeService* js_runtime_service, const std::string& package)
        : Base(imillion)
        , js_context_(&js_runtime_service->js_runtime())
    {
        js_module_ = mjs::Value(mjs::String::New(package));
    }

private:
    // 初始化JS运行时和上下文
    bool OnInit() override {
        auto js_module_name = std::move(js_module_);
        js_module_ = js_context_.runtime().module_manager().GetModule(&js_context_, js_module_name.string_view());
        if (!JSCheckExceptionAndLog(js_module_)) {
            TaskAbort("LoadModule failed with exception: {}.", js_module_name.string_view());
        }
        return true;
    }

    // 消息处理函数
    million::Task<million::MessagePointer> OnStart(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer with_msg) override {
        co_return co_await CallFunc(std::move(with_msg), "onStart");
    }

    million::Task<million::MessagePointer> OnMsg(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer msg) override {
        co_return co_await CallFunc(std::move(msg), "onMsg");
    }

    million::Task<million::MessagePointer> OnStop(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer with_msg) override {
        co_return co_await CallFunc(std::move(with_msg), "onStop");
    }

private:
    // 调用JS函数
    million::Task<million::MessagePointer> CallFunc(million::MessagePointer msg, std::string_view func_name) {
        // 获取模块的 namespace 对象
        // auto space = js_module_.module().namespace_obj();
        
        million::MessagePointer ret_msg;

        // 获取函数
        mjs::Value func;
        auto success = js_module_.module().GetComputedProperty(&js_context_, mjs::Value(func_name.data()), &func);
        if (!success) {
            co_return nullptr;
        }

        // 准备参数
        std::vector<mjs::Value> args;
        if (msg.IsProtoMessage()) {
            auto proto_msg = std::move(msg.GetProtoMessage());
            
            args.push_back(mjs::Value(proto_msg->GetDescriptor()->full_name().c_str()));
            args.push_back(JSUtil::ProtoMessageToJSObject(&js_context_, *proto_msg));
        }
        else {
            args.push_back(mjs::Value());
            args.push_back(mjs::Value());
        }

        // 调用函数
        auto result = js_context_.CallFunction(&func, mjs::Value(), args.begin(), args.end());
        
        // 处理返回值
        if (result.IsPromiseObject()) {
            auto& promise = result.promise();
            // 等待Promise完成
            while (true) {
                if (!promise.IsPending()) {
                    break;
                }

                // 如果有等待的session，处理响应
                if (func_ctx_.waiting_session_id) {
                    auto res_msg = co_await Recv(*func_ctx_.waiting_session_id);
                    if (res_msg.IsCppMessage()) {
                        res_msg = co_await MessageDispatch(func_ctx_.sender, *func_ctx_.waiting_session_id, std::move(res_msg));
                    }

                    if (res_msg.IsProtoMessage()) {
                        auto proto_res_msg = res_msg.GetProtoMessage();
                        auto msg_obj = JSUtil::ProtoMessageToJSObject(&js_context_, *proto_res_msg);

                        promise.Resolve(&js_context_, msg_obj);

                        // js_context_.CallFunction(&func_ctx_.resolving_funcs[0], mjs::Value(), &msg_obj, &msg_obj + 1);
                    }
                    else if (res_msg.IsType<JSValueMsg>()) {
                        auto msg_obj = res_msg.GetMessage<JSValueMsg>()->value;

                        promise.Resolve(&js_context_, msg_obj);
                        // js_context_.CallFunction(&func_ctx_.resolving_funcs[0], mjs::Value(), &msg_obj, &msg_obj + 1);
                    }

                    func_ctx_.waiting_session_id.reset();
                }

                // 执行微任务
                js_context_.ExecuteMicrotasks();
            }

            if (promise.IsFulfilled()) {
                // 获取返回值
                auto tmp = promise.result();
                result = tmp;
            }
            else if (promise.IsRejected()) {
                // Promise被拒绝，获取错误
                auto& reason = promise.reason();
                logger().LOG_ERROR("JS Promise rejected: [{}] {}", GetModuleDef().name(), reason.ToString(&js_context_).string_view());
                co_return nullptr;
            }
        }

        if (result.IsArrayObject()) {
            if (result.array().length() < 2) {
                logger().LOG_ERROR("The length of the array must be 2.");
                co_return nullptr;
            }

            // 直接返回数组形式的返回值
            auto msg_name = result.array()[0];
            if (!msg_name.IsString()) {
                logger().LOG_ERROR("message name must be a string.");
                co_return nullptr;
            }

            auto desc = imillion().proto_mgr().FindMessageTypeByName(msg_name.string_view());
            if (!desc) {
                logger().LOG_ERROR("Invalid message type.");
                co_return nullptr;
            }

            auto ret_proto_msg = imillion().proto_mgr().NewMessage(*desc);
            if (!ret_proto_msg) {
                logger().LOG_ERROR("New message failed.");
                co_return nullptr;
            }

            auto msg_obj = result.array()[1];
            if (!msg_obj.IsObject()) {
                logger().LOG_ERROR("message must be an object.");
                co_return nullptr;
            }

            JSUtil::JSObjectToProtoMessage(&js_context_, ret_proto_msg.get(), msg_obj);
            ret_msg = std::move(ret_proto_msg);
        }
        else if (result.IsUndefined()) {
            co_return nullptr;
        }
        else {
            logger().LOG_ERROR("Need to return undefined or array.");
            co_return nullptr;
        }

        co_return std::move(ret_msg);
    }






    // 添加模块
    //bool AddModule(const std::string& module_name, mjs::ModuleObject* module) {
    //    if (modules_.find(module_name) != modules_.end()) {
    //        logger().LOG_ERROR("Module already exists: {}.", module_name);
    //        return false;
    //    }
    //    modules_[module_name] = mjs::Value(module);
    //    return true;
    //}

    // 检查JS异常
    bool JSCheckExceptionAndLog(const mjs::Value& value) {
        if (value.IsException()) {
            logger().LOG_ERROR("JS Exception: [{}] {}", GetModuleDef().name(), value.ToString(&js_context_).string_view());
            return false;
        }
        return true;
    }

public:
    friend JSService& GetJSService(mjs::Context* context);

    JSRuntimeService& js_runtime_service() const { 
        return GetJSRuntineService(&js_context_.runtime());
    }
    auto& js_context() { return js_context_; }
    auto& js_module() { return js_module_; }
    auto& js_module_cache() { return js_module_cache_; }

    const mjs::ModuleDef& GetModuleDef() const {
        if (js_module_.IsModuleDef()) {
            return js_module_.module_def();
        }
        else if (js_module_.IsModuleObject()) {
            return js_module_.module().module_def();
        }
        else {
            throw std::runtime_error("Incorrect JS module");
        }
    }

private:
    friend class JSModuleManager;

    // mjs上下文
    mjs::Context js_context_;
    
    // 模块相关
    mjs::Value js_module_;
    std::unordered_map<fs::path, mjs::Value> js_module_cache_;
    

    ServiceFuncContext func_ctx_;
};


JSRuntimeService& GetJSRuntineService(mjs::Runtime* runtime) {
    size_t offset = offsetof(JSRuntimeService, js_runtime_);
    auto* js_runtime_service = reinterpret_cast<JSRuntimeService*>(reinterpret_cast<char*>(runtime) - offset);
    return *js_runtime_service;
}

JSService& GetJSService(mjs::Context* context) {
    size_t offset = offsetof(JSService, js_context_);
    auto* js_service = reinterpret_cast<JSService*>(reinterpret_cast<char*>(context) - offset);
    return *js_service;
}


class MillionModuleObject : public mjs::CppModuleObject {
public:
    MillionModuleObject(mjs::Runtime* rt)
        : CppModuleObject(rt)
    {
        AddExportMethod(rt, "newservice", NewService);
    }

    static mjs::Value NewService(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
        auto& service = GetJSRuntineService(&context->runtime());
        if (par_count < 1) {
            return mjs::Value("Creating a service requires 1 parameter.").SetException();
        }
        NewJSService(&service.imillion(), stack.get(0).ToString(context).string_view());
        return mjs::Value();
    }
};

class ServiceModuleObject : public mjs::CppModuleObject {
public:
    ServiceModuleObject(mjs::Runtime* rt)
        : CppModuleObject(rt)
    {
        AddExportMethod(rt, "send", [](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
            auto& service = GetJSService(context);
            if (par_count < 3) {
                return mjs::Value("Send requires 2 parameter.").SetException();
            }

            if (!stack.get(0).IsString()) {
                return mjs::Value("The service name is not a string.").SetException();
            }
            if (!stack.get(1).IsString()) {
                return mjs::Value("The message name is not a string.").SetException();
            }
            if (!stack.get(2).IsObject()) {
                return mjs::Value("The message is not an object.").SetException();
            }

            auto target = service.imillion().GetServiceByName(stack.get(0).string_view());
            if (!target) {
                return mjs::Value();
            }

            auto desc = service.imillion().proto_mgr().FindMessageTypeByName(stack.get(1).string_view());
            if (!desc) {
                return mjs::Value();
            }
            auto msg = service.imillion().proto_mgr().NewMessage(*desc);

            JSUtil::JSObjectToProtoMessage(context, msg.get(), stack.get(2));

            service.Send(*target, MessagePointer(std::move(msg)));
            return mjs::Value();
        });
    }
};

class LoggerModuleObject : public mjs::CppModuleObject {
public:
    LoggerModuleObject(mjs::Runtime* rt)
        : CppModuleObject(rt)
    {
        AddExportMethod(rt, "debug", Debug);
        AddExportMethod(rt, "info", Info);
        AddExportMethod(rt, "error", Error);
    }

    static mjs::Value Debug(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
        auto& service = GetJSRuntineService(&context->runtime());
        auto source_location = GetSourceLocation(stack);
        for (uint32_t i = 0; i < par_count; ++i) {
            service.logger().Log(source_location, ::million::Logger::LogLevel::kDebug, stack.get(i).ToString(context).string_view());
        }
        return mjs::Value();
    }

    static mjs::Value Info(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
        auto& service = GetJSRuntineService(&context->runtime());
        auto source_location = GetSourceLocation(stack);
        for (uint32_t i = 0; i < par_count; ++i) {
            service.logger().Log(source_location, ::million::Logger::LogLevel::kInfo, stack.get(i).ToString(context).string_view());
        }
        return mjs::Value();
    }

    static mjs::Value Error(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
        auto& service = GetJSRuntineService(&context->runtime());
        auto source_location = GetSourceLocation(stack);
        for (uint32_t i = 0; i < par_count; ++i) {
            service.logger().Log(source_location, ::million::Logger::LogLevel::kError, stack.get(i).ToString(context).string_view());
        }
        return mjs::Value();
    }
private:
    static million::SourceLocation GetSourceLocation(const mjs::StackFrame& stack) {
        const mjs::StackFrame* js_stack = &stack;
        while (!js_stack->function_def()) {
            if (!js_stack->upper_stack_frame()) {
                break;
            }
            js_stack = js_stack->upper_stack_frame();
        }
        auto debug_info = js_stack->function_def()->debug_table().FindEntry(js_stack->pc());
        if (debug_info && js_stack) {
            auto source_location = million::SourceLocation{
                .line = debug_info->source_line,
                .column = 0,
                .file_name = js_stack->function_def()->module_def().name(),
                .function_name = js_stack->function_def()->name(),
            };
            return source_location;
        }
        else {
            auto source_location = million::SourceLocation{
                .line = 0,
                .column = 0,
                .file_name = "<unknown>",
                .function_name = "<unknown>",
            };
            return source_location;
        }

    }
};




bool JSRuntimeService::OnInit() {
    const auto& settings = imillion().YamlSettings();

    const auto& jssvr_settings = settings["jssvr"];
    if (!jssvr_settings) {
        logger().LOG_ERROR("cannot find 'jssvr'.");
        return false;
    }
    if (!jssvr_settings["dirs"]) {
        logger().LOG_ERROR("cannot find 'jssvr.dirs'.");
        return false;
    }
    jssvr_dirs_ = jssvr_settings["dirs"].as<std::vector<std::string>>();

    js_runtime_.module_manager().AddCppModule("million", new MillionModuleObject(&js_runtime_));
    js_runtime_.module_manager().AddCppModule("service", new ServiceModuleObject(&js_runtime_));
    js_runtime_.module_manager().AddCppModule("logger", new LoggerModuleObject(&js_runtime_));

    return true;
}


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