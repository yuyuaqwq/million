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

namespace million {
namespace jssvr {

namespace fs = std::filesystem;

// 服务函数上下文，用于处理异步调用
struct ServiceFuncContext {
    //mjs::Value promise_cap;
    //mjs::Value resolving_funcs[2];
    ServiceHandle sender;
    std::optional<SessionId> waiting_session_id;
};

class MillionModuleObject : public mjs::CppModuleObject {
public:
    MillionModuleObject(mjs::Runtime* rt)
        : CppModuleObject(rt)
    {
        AddExportMethod(rt, "newservice", [](mjs::Context * context, uint32_t par_count, const mjs::StackFrame & stack) -> mjs::Value {
            
            return mjs::Value();
        });
    }
};

class LoggerModuleObject : public mjs::CppModuleObject {
public:
    LoggerModuleObject(mjs::Runtime* rt)
        : CppModuleObject(rt)
    {
        AddExportMethod(rt, "err", [](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {

            return mjs::Value();
        });
        AddExportMethod(rt, "info", [](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {

            return mjs::Value();
        });
    }
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

        js_runtime_.module_manager().AddCppModule("million", new MillionModuleObject(&js_runtime_));
        js_runtime_.module_manager().AddCppModule("logger", new LoggerModuleObject(&js_runtime_));

        return true;
    }

    Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override {
        db_handle_ = *imillion().GetServiceByName(db::kDBServiceName);
        config_handle_ = *imillion().GetServiceByName(config::kConfigServiceName);
        co_return nullptr;
    }


private:
    friend class JSService;
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
        //if (!CreateMillionModule()) {
        //    TaskAbort("CreateMillionModule failed.");
        //}
        //if (!CreateServiceModule()) {
        //    TaskAbort("CreateServiceModule failed.");
        //}
        //if (!CreateLoggerModule()) {
        //    TaskAbort("CreateLoggerModule failed.");
        //}
        //if (!CreateDBModule()) {
        //    TaskAbort("CreateDBModule failed.");
        //}
        //if (!CreateConfigModule()) {
        //    TaskAbort("CreateConfigModule failed.");
        //}
        auto js_module_name = std::move(js_module_);
        js_module_ = js_context_.runtime().module_manager().GetModule(&js_context_, js_module_name.string_view());
        if (JSCheckExceptionAndLog(js_module_)) {
            TaskAbort("LoadModule failed with exception.");
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
    // 获取JS值
    mjs::Value GetJSValueByProtoMessageField(const million::ProtoMessage& msg
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc)
    {
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            return mjs::Value(reflection.GetDouble(msg, &field_desc));
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            return mjs::Value(reflection.GetFloat(msg, &field_desc));
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
            return mjs::Value(reflection.GetInt64(msg, &field_desc));
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            return mjs::Value(reflection.GetUInt64(msg, &field_desc));
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
            return mjs::Value(reflection.GetInt32(msg, &field_desc));
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            return mjs::Value(reflection.GetUInt32(msg, &field_desc));
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            return mjs::Value(reflection.GetBool(msg, &field_desc));
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            std::string string;
            auto string_ref = reflection.GetStringReference(msg, &field_desc, &string);
            return mjs::Value(string_ref.c_str());
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            std::string bytes;
            auto bytes_ref = reflection.GetStringReference(msg, &field_desc, &bytes);
            // TODO: 实现二进制数据转换
            return mjs::Value();
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            const auto enum_value = reflection.GetEnum(msg, &field_desc);
            if (!enum_value) {
                return mjs::Value("");
            }
            return mjs::Value(enum_value->name().c_str());
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            const auto& sub_msg = reflection.GetMessage(msg, &field_desc);
            return ProtoMessageToJSObject(sub_msg);
        }
        default:
            return mjs::Value();
        }
    }

    // 获取重复字段的JS值
    mjs::Value GetJSValueByProtoMessgaeRepeatedField(const million::ProtoMessage& msg
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc
        , size_t j)
    {
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            return mjs::Value(reflection.GetRepeatedDouble(msg, &field_desc, j));
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            return mjs::Value(reflection.GetRepeatedFloat(msg, &field_desc, j));
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
            return mjs::Value(reflection.GetRepeatedInt64(msg, &field_desc, j));
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            return mjs::Value(reflection.GetRepeatedUInt64(msg, &field_desc, j));
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
            return mjs::Value(reflection.GetRepeatedInt32(msg, &field_desc, j));
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            return mjs::Value(reflection.GetRepeatedUInt32(msg, &field_desc, j));
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            return mjs::Value(reflection.GetRepeatedBool(msg, &field_desc, j));
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            std::string string;
            auto string_ref = reflection.GetRepeatedStringReference(msg, &field_desc, j, &string);
            return mjs::Value(string_ref.c_str());
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            std::string bytes;
            auto bytes_ref = reflection.GetRepeatedStringReference(msg, &field_desc, j, &bytes);
            // TODO: 实现二进制数据转换
            return mjs::Value();
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            const auto enum_value = reflection.GetRepeatedEnum(msg, &field_desc, j);
            if (!enum_value) {
                return mjs::Value("");
            }
            return mjs::Value(enum_value->name().c_str());
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            const auto& sub_msg = reflection.GetRepeatedMessage(msg, &field_desc, j);
            return ProtoMessageToJSObject(sub_msg);
        }
        default:
            return mjs::Value();
        }
    }

    // 转换单个字段
    void ProtoMessageToJSObjectOne(const million::ProtoMessage& msg, mjs::Object* obj, const google::protobuf::FieldDescriptor& field_desc) {
        const auto desc = msg.GetDescriptor();
        const auto refl = msg.GetReflection();

        if (field_desc.is_repeated()) {
            auto js_array = mjs::ArrayObject::New(&js_context_, {});
            for (size_t j = 0; j < refl->FieldSize(msg, &field_desc); ++j) {
                auto js_value = GetJSValueByProtoMessgaeRepeatedField(msg, *refl, field_desc, j);
                js_array->operator[](j) = js_value;
            }
            obj->SetComputedProperty(&js_context_, mjs::Value(field_desc.name()), mjs::Value(js_array));
        }
        else {
            auto js_value = GetJSValueByProtoMessageField(msg, *refl, field_desc);
            obj->SetComputedProperty(&js_context_, mjs::Value(field_desc.name()), std::move(js_value));
        }
    }

    // Protobuf消息转JS对象
    mjs::Value ProtoMessageToJSObject(const million::ProtoMessage& msg) {
        auto obj = mjs::Object::New(&js_context_);

        const auto desc = msg.GetDescriptor();
        const auto refl = msg.GetReflection();

        for (size_t i = 0; i < desc->field_count(); ++i) {
            const auto field_desc = desc->field(i);
            ProtoMessageToJSObjectOne(msg, obj, *field_desc);
        }

        return mjs::Value(obj);
    }

    // 设置重复字段
    void SetProtoMessageRepeatedFieldFromJSValue(million::ProtoMessage* msg
        , const google::protobuf::Descriptor& desc
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc
        , const mjs::Value& repeated_value, size_t j)
    {
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
            if (!repeated_value.IsNumber()) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            reflection.AddDouble(msg, &field_desc, repeated_value.f64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
            if (!repeated_value.IsNumber()) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            reflection.AddFloat(msg, &field_desc, repeated_value.f64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64: {
            if (!repeated_value.IsInt64()) {
                TaskAbort("Field {}.{}[{}] is not a bigint.", desc.name(), field_desc.name(), j);
            }
            reflection.AddInt64(msg, &field_desc, repeated_value.i64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
            if (!repeated_value.IsUInt64()) {
                TaskAbort("Field {}.{}[{}] is not a bigint.", desc.name(), field_desc.name(), j);
            }
            reflection.AddUInt64(msg, &field_desc, repeated_value.u64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32: {
            if (!repeated_value.IsNumber()) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            reflection.AddInt32(msg, &field_desc, repeated_value.i64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
            if (!repeated_value.IsNumber()) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            reflection.AddUInt32(msg, &field_desc, repeated_value.u64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
            if (!repeated_value.IsBoolean()) {
                TaskAbort("Field {}.{}[{}] is not a bool.", desc.name(), field_desc.name(), j);
            }
            reflection.AddBool(msg, &field_desc, repeated_value.boolean());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            if (!repeated_value.IsString()) {
                TaskAbort("Field {}.{}[{}] is not a string.", desc.name(), field_desc.name(), j);
            }
            reflection.AddString(msg, &field_desc, repeated_value.string_view());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            // TODO: 实现二进制数据转换
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            if (!repeated_value.IsString()) {
                TaskAbort("Field {}.{} is not a string(enum).", desc.name(), field_desc.name());
            }

            // 获取枚举描述符
            const google::protobuf::EnumDescriptor* enum_desc = field_desc.enum_type();
            if (!enum_desc) {
                TaskAbort("Field {}.{} has no enum descriptor.", desc.name(), field_desc.name());
            }

            // 通过字符串查找枚举值
            const google::protobuf::EnumValueDescriptor* enum_value = enum_desc->FindValueByName(repeated_value.string_view());
            if (!enum_value) {
                TaskAbort("Field {}.{} has no enum value named {}.", desc.name(), field_desc.name(), repeated_value.string_view());
            }

            // 设置枚举值
            reflection.AddEnum(msg, &field_desc, enum_value);
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            if (!repeated_value.IsObject()) {
                TaskAbort("Field {}.{}[{}] is not a object.", desc.name(), field_desc.name(), j);
            }

            const auto sub_msg = reflection.AddMessage(msg, &field_desc);
            JSObjectToProtoMessage(sub_msg, repeated_value);
            break;
        }
        default:
            TaskAbort("Field {}.{}[{}] cannot convert type.", desc.name(), field_desc.name(), j);
            break;
        }
    }

    // 设置字段
    void SetProtoMessageFieldFromJSValue(million::ProtoMessage* msg
        , const google::protobuf::Descriptor& desc
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc
        , const mjs::Value& js_value)
    {
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
            if (!js_value.IsNumber()) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            reflection.SetDouble(msg, &field_desc, js_value.f64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
            if (!js_value.IsNumber()) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            reflection.SetFloat(msg, &field_desc, js_value.f64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64: {
            if (!js_value.IsInt64()) {
                TaskAbort("Field {}.{} is not a bigint.", desc.name(), field_desc.name());
            }
            reflection.SetInt64(msg, &field_desc, js_value.i64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
            if (!js_value.IsUInt64()) {
                TaskAbort("Field {}.{} is not a bigint.", desc.name(), field_desc.name());
            }
            reflection.SetUInt64(msg, &field_desc, js_value.u64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32: {
            if (!js_value.IsNumber()) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            reflection.SetInt32(msg, &field_desc, js_value.i64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
            if (!js_value.IsNumber()) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            reflection.SetUInt32(msg, &field_desc, js_value.u64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
            if (!js_value.IsBoolean()) {
                TaskAbort("Field {}.{} is not a bool.", desc.name(), field_desc.name());
            }
            reflection.SetBool(msg, &field_desc, js_value.boolean());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            if (!js_value.IsString()) {
                TaskAbort("Field {}.{} is not a string.", desc.name(), field_desc.name());
            }
            reflection.SetString(msg, &field_desc, js_value.string_view());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            // TODO: 实现二进制数据转换
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            if (!js_value.IsString()) {
                TaskAbort("Field {}.{} is not a string(enum).", desc.name(), field_desc.name());
            }

            // 获取枚举描述符
            const google::protobuf::EnumDescriptor* enum_desc = field_desc.enum_type();
            if (!enum_desc) {
                TaskAbort("Field {}.{} has no enum descriptor.", desc.name(), field_desc.name());
            }

            // 通过字符串查找枚举值
            const google::protobuf::EnumValueDescriptor* enum_value = enum_desc->FindValueByName(js_value.string_view());
            if (!enum_value) {
                TaskAbort("Field {}.{} has no enum value named {}.", desc.name(), field_desc.name(), js_value.string_view());
            }

            // 设置枚举值
            reflection.SetEnum(msg, &field_desc, enum_value);
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            if (!js_value.IsObject()) {
                TaskAbort("Field {}.{} is not a object.", desc.name(), field_desc.name());
            }
            auto sub_msg = reflection.MutableMessage(msg, &field_desc);
            JSObjectToProtoMessage(sub_msg, js_value);
            break;
        }
        default:
            TaskAbort("Field {}.{} cannot convert type.", desc.name(), field_desc.name());
            break;
        }
    }

    // 转换单个字段
    void JSObjectToProtoMessageOne(million::ProtoMessage* msg, const mjs::Value& field_value, const google::protobuf::FieldDescriptor& field_desc) {
        const auto desc = msg->GetDescriptor();
        const auto refl = msg->GetReflection();

        if (field_value.IsUndefined()) {
            return;  // 如果 JS 对象没有该属性，则跳过
        }

        if (field_desc.is_repeated()) {
            if (!field_value.IsArrayObject()) {
                TaskAbort("JSObjectToProtoMessage: Not an array.");
                return;
            }

            auto len = field_value.array().length();
            for (size_t j = 0; j < len; ++j) {
                auto repeated_value = field_value.array()[j];
                SetProtoMessageRepeatedFieldFromJSValue(msg, *desc, *refl, field_desc, repeated_value, j);
            }
        }
        else {
            SetProtoMessageFieldFromJSValue(msg, *desc, *refl, field_desc, field_value);
        }
    }

    // JS对象转Protobuf消息
    void JSObjectToProtoMessage(million::ProtoMessage* msg, const mjs::Value& obj_val) {
        const auto desc = msg->GetDescriptor();
        const auto refl = msg->GetReflection();

        auto& obj = obj_val.object();

        for (size_t i = 0; i < desc->field_count(); ++i) {
            const auto field_desc = desc->field(i);
            mjs::Value field_value;
            if (obj.GetComputedProperty(&js_context_, mjs::Value(field_desc->name().c_str()), &field_value)) {
                JSObjectToProtoMessageOne(msg, field_value, *field_desc);
            }
        }
    }

    // 调用JS函数
    million::Task<million::MessagePointer> CallFunc(million::MessagePointer msg, std::string_view func_name) {
        if (!msg.IsProtoMessage()) {
            co_return nullptr;
        }

        auto proto_msg = std::move(msg.GetProtoMessage());
        million::MessagePointer ret_msg;

        // 获取模块的 namespace 对象
        // auto space = js_module_.module().namespace_obj();
        


        // 获取函数
        mjs::Value func;
        auto success = js_module_.module().GetComputedProperty(&js_context_, mjs::Value(func_name.data()), &func);
        if (!success) {
            co_return nullptr;
        }

        // 准备参数
        std::vector<mjs::Value> args;
        if (proto_msg) {
            args.push_back(mjs::Value(proto_msg->GetDescriptor()->full_name().c_str()));
            args.push_back(ProtoMessageToJSObject(*proto_msg));
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
                        auto msg_obj = ProtoMessageToJSObject(*proto_res_msg);

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
                auto& result = promise.result();
                if (result.IsUndefined()) {
                    co_return nullptr;
                }

                if (!result.IsArrayObject()) {
                    logger().LOG_ERROR("Need to return undefined or array.");
                    co_return nullptr;
                }

                // 解析返回值数组
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

                JSObjectToProtoMessage(ret_proto_msg.get(), msg_obj);
                ret_msg = std::move(ret_proto_msg);
            }
            else if (promise.IsRejected()) {
                // Promise被拒绝，获取错误
                auto& error = promise.reason();
                logger().LOG_ERROR("JS Promise rejected: {}.", error.ToString(&js_context_).string_view());
            }
        }
        else if (result.IsArrayObject()) {
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

            JSObjectToProtoMessage(ret_proto_msg.get(), msg_obj);
            ret_msg = std::move(ret_proto_msg);
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
            logger().LOG_ERROR("JS Exception: {}.", value.ToString(&js_context_).string_view());
            return false;
        }
        return true;
    }

public:
    JSRuntimeService& js_runtime_service() const { 
        auto& runtime = js_context_.runtime();
        size_t offset = offsetof(JSRuntimeService, js_runtime_);
        auto* js_runtime_service = reinterpret_cast<JSRuntimeService*>(reinterpret_cast<char*>(&runtime) - offset);
        return *js_runtime_service;
    }
    auto& js_context() { return js_context_; }
    auto& js_module() { return js_module_; }
    auto& js_module_cache() { return js_module_cache_; }

private:
    friend class JSModuleManager;

    // mjs上下文
    mjs::Context js_context_;
    
    // 模块相关
    mjs::Value js_module_;
    std::unordered_map<fs::path, mjs::Value> js_module_cache_;
    

    ServiceFuncContext func_ctx_;
};



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
            return mjs::Value(std::format("LoadModule failed: {}.", module_name));
        }
    }

    if (module.IsModuleDef()) {
        js_service->js_context().CallModule(&module);
        js_service->js_module_cache().emplace(path, module);
    }
    return module;
}

mjs::Value JSModuleManager::FindJSModule(JSService* js_service, std::filesystem::path path) {
    assert(path.is_absolute());

    // 再找下Cpp模块

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