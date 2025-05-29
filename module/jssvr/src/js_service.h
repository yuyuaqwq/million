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
#include "js_module_manager.h"
#include "js_cpp_module.h"

namespace million {
namespace jssvr {

namespace fs = std::filesystem;

class JSRuntimeService;
JSRuntimeService& GetJSRuntineService(mjs::Runtime* runtime);
class JSService;
JSService& GetJSService(mjs::Context* context);

// 服务函数上下文，用于处理异步调用
struct FunctionCallContext {
    mjs::Value promise;
    //mjs::Value resolving_funcs[2];
    ServiceHandle sender;
    std::optional<SessionId> waiting_session_id;
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

    const auto& jssvr_dirs() const {
        return jssvr_dirs_;
    }

    const auto& db_service_handle() const { return db_service_handle_; }
    const auto& config_service_handle() const { return config_service_handle_; }

private:
    virtual bool OnInit() override {
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

        js_runtime_.module_manager().AddCppModule("million", MillionModuleObject::New(&js_runtime_));
        js_runtime_.module_manager().AddCppModule("service", ServiceModuleObject::New(&js_runtime_));
        js_runtime_.module_manager().AddCppModule("logger", LoggerModuleObject::New(&js_runtime_));

        return true;
    }

    Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override {
        db_service_handle_ = *imillion().GetServiceByName(db::kDBServiceName);
        config_service_handle_ = *imillion().GetServiceByName(config::kConfigServiceName);
        co_return nullptr;
    }


private:
    friend JSRuntimeService& GetJSRuntineService(mjs::Runtime* runtime);
    mjs::Runtime js_runtime_;
    std::vector<std::string> jssvr_dirs_;
    ServiceHandle db_service_handle_;
    ServiceHandle config_service_handle_;
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

        FunctionCallContext function_call_context;

        // 调用函数
        // 这里看回调怎么拿到func_ctx_，要考虑多协程，不过Call js函数是同步执行完的，可以在每次Call之前设置一下成员变量指针
        // 多协程共用成员变量指针就行
        function_call_context_ = &function_call_context;
        js_context_.runtime().stack().clear();
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
                if (function_call_context.waiting_session_id) {
                    auto res_msg = co_await Recv(*function_call_context.waiting_session_id);
                    if (res_msg.IsCppMessage()) {
                        res_msg = co_await MessageDispatch(function_call_context.sender, *function_call_context.waiting_session_id, std::move(res_msg));
                    }

                    if (res_msg.IsProtoMessage()) {
                        auto proto_res_msg = res_msg.GetProtoMessage();
                        auto msg_obj = JSUtil::ProtoMessageToJSObject(&js_context_, *proto_res_msg);
                        function_call_context.promise.promise().Resolve(&js_context_, msg_obj);
                    }
                    else if (res_msg.IsType<JSValueMsg>()) {
                        auto msg_obj = res_msg.GetMessage<JSValueMsg>()->value;
                        function_call_context.promise.promise().Resolve(&js_context_, msg_obj);
                    }

                    function_call_context.waiting_session_id.reset();
                    function_call_context.promise = mjs::Value();
                }
                else {
                    if (js_context_.microtask_queue().empty()) {
                        logger().LOG_ERROR("There are no executable micro tasks available: [{}] {}", js_module_.ToModuleDef().name(), func.ToFunctionDef().name());
                        co_return nullptr;
                    }
                }

                function_call_context_ = &function_call_context;
                js_context_.runtime().stack().clear();
                // 执行微任务
                //while (!js_context_.microtask_queue().empty()) {
                //    auto& task = js_context_.microtask_queue().front();
                //    auto result = js_context_.CallFunction(&task.func(), task.this_val(), task.argv().begin(), task.argv().end());
                //    JSCheckExceptionAndLog(result);
                //    js_context_.microtask_queue().pop_front();
                //}
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
                logger().LOG_ERROR("JS Promise rejected: [{}] {}", js_module_.ToModuleDef().name(), reason.ToString(&js_context_).string_view());
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
            logger().LOG_ERROR("JS Exception: [{}] {}", js_module_.ToModuleDef().name(), value.ToString(&js_context_).string_view());
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

    auto& function_call_context() { return *function_call_context_; }

private:
    friend class JSModuleManager;

    // mjs上下文
    mjs::Context js_context_;
    
    // 模块相关
    mjs::Value js_module_;
    std::unordered_map<fs::path, mjs::Value> js_module_cache_;

    FunctionCallContext* function_call_context_ = nullptr;
};


inline JSRuntimeService& GetJSRuntineService(mjs::Runtime* runtime) {
    size_t offset = offsetof(JSRuntimeService, js_runtime_);
    auto* js_runtime_service = reinterpret_cast<JSRuntimeService*>(reinterpret_cast<char*>(runtime) - offset);
    return *js_runtime_service;
}

inline JSService& GetJSService(mjs::Context* context) {
    size_t offset = offsetof(JSService, js_context_);
    auto* js_service = reinterpret_cast<JSService*>(reinterpret_cast<char*>(context) - offset);
    return *js_service;
}

} // namespace jssvr
} // namespace million