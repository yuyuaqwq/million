#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/value.h>
#include <mjs/object.h>
#include <mjs/module_mgr.h>
#include <mjs/object_impl/array_object.h>

#include <yaml-cpp/yaml.h>

#include <db/db.h>
#include <config/config.h>

#include <million/imillion.h>

#include <jssvr/api.h>

namespace million {
namespace jssvr {

//// 服务函数上下文，用于处理异步调用
//struct ServiceFuncContext {
//    mjs::Value promise_cap;
//    mjs::Value resolving_funcs[2];
//    ServiceHandle sender;
//    std::optional<SessionId> waiting_session_id;
//};
//
//// JS运行时服务
//class JSRuntimeService : public IService {
//    MILLION_SERVICE_DEFINE(JSRuntimeService);
//
//public:
//    using Base = IService;
//    using Base::Base;
//
//private:
//    virtual bool OnInit() override {
//        const auto& settings = imillion().YamlSettings();
//
//        const auto& jssvr_settings = settings["jssvr"];
//        if (!jssvr_settings) {
//            logger().Err("cannot find 'jssvr'.");
//            return false;
//        }
//        if (!jssvr_settings["dirs"]) {
//            logger().Err("cannot find 'jssvr.dirs'.");
//            return false;
//        }
//        jssvr_dirs_ = jssvr_settings["dirs"].as<std::vector<std::string>>();
//        return true;
//    }
//
//    Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override {
//        db_handle_ = *imillion().GetServiceByName(db::kDBServiceName);
//        config_handle_ = *imillion().GetServiceByName(config::kConfigServiceName);
//        co_return nullptr;
//    }
//
//    mjs::Runtime& js_rt() {
//        return js_rt_;
//    }
//
//private:
//    mjs::Runtime js_rt_;
//    std::vector<std::string> jssvr_dirs_;
//    ServiceHandle db_handle_;
//    ServiceHandle config_handle_;
//};
//
//// JS值消息，用于在C++和JS之间传递值
//MILLION_MESSAGE_DEFINE(, JSValueMsg, (mjs::Value) value)
//
//// JS服务，负责执行JS代码和处理消息
//class JSService : public IService {
//    MILLION_SERVICE_DEFINE(JSService);
//
//public:
//    using Base = IService;
//    JSService(IMillion* imillion, JSRuntimeService* js_runtime_service, const std::string& package)
//        : Base(imillion)
//        , js_runtime_service_(js_runtime_service)
//        , js_ctx_(&js_runtime_service->js_rt())
//        , package_(package) {}
//
//    // 消息处理函数
//    virtual million::Task<million::MessagePointer> OnStart(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer with_msg) override {
//        co_return co_await CallFunc(std::move(with_msg), "onStart");
//    }
//
//    virtual million::Task<million::MessagePointer> OnMsg(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer msg) override {
//        co_return co_await CallFunc(std::move(msg), "onMsg");
//    }
//
//    virtual million::Task<million::MessagePointer> OnStop(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer with_msg) override {
//        co_return co_await CallFunc(std::move(with_msg), "onStop");
//    }
//
//private:
//    // 获取JS值
//    mjs::Value GetJsValueByProtoMsgField(const million::ProtoMessage& msg
//        , const google::protobuf::Reflection& reflection
//        , const google::protobuf::FieldDescriptor& field_desc)
//    {
//        switch (field_desc.type()) {
//        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
//            return mjs::Value(reflection.GetDouble(msg, &field_desc));
//        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
//            return mjs::Value(reflection.GetFloat(msg, &field_desc));
//        case google::protobuf::FieldDescriptor::TYPE_INT64:
//        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
//        case google::protobuf::FieldDescriptor::TYPE_SINT64:
//            return mjs::Value(reflection.GetInt64(msg, &field_desc));
//        case google::protobuf::FieldDescriptor::TYPE_UINT64:
//        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
//            return mjs::Value(reflection.GetUInt64(msg, &field_desc));
//        case google::protobuf::FieldDescriptor::TYPE_INT32:
//        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
//        case google::protobuf::FieldDescriptor::TYPE_SINT32:
//            return mjs::Value(reflection.GetInt32(msg, &field_desc));
//        case google::protobuf::FieldDescriptor::TYPE_UINT32:
//        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
//            return mjs::Value(reflection.GetUInt32(msg, &field_desc));
//        case google::protobuf::FieldDescriptor::TYPE_BOOL:
//            return mjs::Value(reflection.GetBool(msg, &field_desc));
//        case google::protobuf::FieldDescriptor::TYPE_STRING: {
//            std::string string;
//            auto string_ref = reflection.GetStringReference(msg, &field_desc, &string);
//            return mjs::Value(string_ref.c_str());
//        }
//        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
//            std::string bytes;
//            auto bytes_ref = reflection.GetStringReference(msg, &field_desc, &bytes);
//            // TODO: 实现二进制数据转换
//            return mjs::Value();
//        }
//        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
//            const auto enum_value = reflection.GetEnum(msg, &field_desc);
//            if (!enum_value) {
//                return mjs::Value("");
//            }
//            return mjs::Value(enum_value->name().c_str());
//        }
//        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
//            const auto& sub_msg = reflection.GetMessage(msg, &field_desc);
//            return ProtoMsgToJsObj(sub_msg);
//        }
//        default:
//            return mjs::Value();
//        }
//    }
//
//    // 获取重复字段的JS值
//    mjs::Value GetJsValueByProtoMsgRepeatedField(const million::ProtoMessage& msg
//        , const google::protobuf::Reflection& reflection
//        , const google::protobuf::FieldDescriptor& field_desc
//        , size_t j)
//    {
//        switch (field_desc.type()) {
//        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
//            return mjs::Value(reflection.GetRepeatedDouble(msg, &field_desc, j));
//        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
//            return mjs::Value(reflection.GetRepeatedFloat(msg, &field_desc, j));
//        case google::protobuf::FieldDescriptor::TYPE_INT64:
//        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
//        case google::protobuf::FieldDescriptor::TYPE_SINT64:
//            return mjs::Value(reflection.GetRepeatedInt64(msg, &field_desc, j));
//        case google::protobuf::FieldDescriptor::TYPE_UINT64:
//        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
//            return mjs::Value(reflection.GetRepeatedUInt64(msg, &field_desc, j));
//        case google::protobuf::FieldDescriptor::TYPE_INT32:
//        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
//        case google::protobuf::FieldDescriptor::TYPE_SINT32:
//            return mjs::Value(reflection.GetRepeatedInt32(msg, &field_desc, j));
//        case google::protobuf::FieldDescriptor::TYPE_UINT32:
//        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
//            return mjs::Value(reflection.GetRepeatedUInt32(msg, &field_desc, j));
//        case google::protobuf::FieldDescriptor::TYPE_BOOL:
//            return mjs::Value(reflection.GetRepeatedBool(msg, &field_desc, j));
//        case google::protobuf::FieldDescriptor::TYPE_STRING: {
//            std::string string;
//            auto string_ref = reflection.GetRepeatedStringReference(msg, &field_desc, j, &string);
//            return mjs::Value(string_ref.c_str());
//        }
//        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
//            std::string bytes;
//            auto bytes_ref = reflection.GetRepeatedStringReference(msg, &field_desc, j, &bytes);
//            // TODO: 实现二进制数据转换
//            return mjs::Value();
//        }
//        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
//            const auto enum_value = reflection.GetRepeatedEnum(msg, &field_desc, j);
//            if (!enum_value) {
//                return mjs::Value("");
//            }
//            return mjs::Value(enum_value->name().c_str());
//        }
//        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
//            const auto& sub_msg = reflection.GetRepeatedMessage(msg, &field_desc, j);
//            return ProtoMsgToJsObj(sub_msg);
//        }
//        default:
//            return mjs::Value();
//        }
//    }
//
//    // 转换单个字段
//    void ProtoMsgToJsObjOne(const million::ProtoMessage& msg, mjs::Value obj_val, const google::protobuf::FieldDescriptor& field_desc) {
//        const auto desc = msg.GetDescriptor();
//        const auto refl = msg.GetReflection();
//
//        if (field_desc.is_repeated()) {
//            auto js_array = mjs::ArrayObject::New(&js_ctx_, {});
//            for (size_t j = 0; j < refl->FieldSize(msg, &field_desc); ++j) {
//                auto js_value = GetJsValueByProtoMsgRepeatedField(msg, *refl, field_desc, j);
//                js_array.SetProperty(j, js_value);
//            }
//            obj.SetProperty(field_desc.name().c_str(), js_array);
//        }
//        else {
//            auto js_value = GetJsValueByProtoMsgField(msg, *refl, field_desc);
//            obj.SetProperty(field_desc.name().c_str(), js_value);
//        }
//    }
//
//    // Protobuf消息转JS对象
//    mjs::Value ProtoMsgToJsObj(const million::ProtoMessage& msg) {
//        auto obj = mjs::Value::NewObject(js_ctx_.get());
//
//        const auto desc = msg.GetDescriptor();
//        const auto refl = msg.GetReflection();
//
//        for (size_t i = 0; i < desc->field_count(); ++i) {
//            const auto field_desc = desc->field(i);
//            ProtoMsgToJsObjOne(msg, obj, *field_desc);
//        }
//
//        return obj;
//    }
//
//    // 设置重复字段
//    void SetProtoMsgRepeatedFieldFromJsValue(million::ProtoMessage* msg
//        , const google::protobuf::Descriptor& desc
//        , const google::protobuf::Reflection& reflection
//        , const google::protobuf::FieldDescriptor& field_desc
//        , const mjs::Value& repeated_value, size_t j)
//    {
//        switch (field_desc.type()) {
//        case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
//            if (!repeated_value.IsNumber()) {
//                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
//            }
//            reflection.AddDouble(msg, &field_desc, repeated_value.f64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
//            if (!repeated_value.IsNumber()) {
//                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
//            }
//            reflection.AddFloat(msg, &field_desc, repeated_value.f64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_INT64:
//        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
//        case google::protobuf::FieldDescriptor::TYPE_SINT64: {
//            if (!repeated_value.IsInt64()) {
//                TaskAbort("Field {}.{}[{}] is not a bigint.", desc.name(), field_desc.name(), j);
//            }
//            reflection.AddInt64(msg, &field_desc, repeated_value.i64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_UINT64:
//        case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
//            if (!repeated_value.IsUInt64()) {
//                TaskAbort("Field {}.{}[{}] is not a bigint.", desc.name(), field_desc.name(), j);
//            }
//            reflection.AddUInt64(msg, &field_desc, repeated_value.u64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_INT32:
//        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
//        case google::protobuf::FieldDescriptor::TYPE_SINT32: {
//            if (!repeated_value.IsNumber()) {
//                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
//            }
//            reflection.AddInt32(msg, &field_desc, repeated_value.i64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_UINT32:
//        case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
//            if (!repeated_value.IsNumber()) {
//                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
//            }
//            reflection.AddUInt32(msg, &field_desc, repeated_value.u64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
//            if (!repeated_value.IsBoolean()) {
//                TaskAbort("Field {}.{}[{}] is not a bool.", desc.name(), field_desc.name(), j);
//            }
//            reflection.AddBool(msg, &field_desc, repeated_value.boolean());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_STRING: {
//            if (!repeated_value.IsString()) {
//                TaskAbort("Field {}.{}[{}] is not a string.", desc.name(), field_desc.name(), j);
//            }
//            reflection.AddString(msg, &field_desc, repeated_value.string_view());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
//            // TODO: 实现二进制数据转换
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
//            if (!repeated_value.IsString()) {
//                TaskAbort("Field {}.{} is not a string(enum).", desc.name(), field_desc.name());
//            }
//
//            // 获取枚举描述符
//            const google::protobuf::EnumDescriptor* enum_desc = field_desc.enum_type();
//            if (!enum_desc) {
//                TaskAbort("Field {}.{} has no enum descriptor.", desc.name(), field_desc.name());
//            }
//
//            // 通过字符串查找枚举值
//            const google::protobuf::EnumValueDescriptor* enum_value = enum_desc->FindValueByName(repeated_value.string_view());
//            if (!enum_value) {
//                TaskAbort("Field {}.{} has no enum value named {}.", desc.name(), field_desc.name(), repeated_value.string_view());
//            }
//
//            // 设置枚举值
//            reflection.AddEnum(msg, &field_desc, enum_value);
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
//            if (!repeated_value.IsObject()) {
//                TaskAbort("Field {}.{}[{}] is not a object.", desc.name(), field_desc.name(), j);
//            }
//
//            const auto sub_msg = reflection.AddMessage(msg, &field_desc);
//            JsObjToProtoMsg(sub_msg, repeated_value);
//            break;
//        }
//        default:
//            TaskAbort("Field {}.{}[{}] cannot convert type.", desc.name(), field_desc.name(), j);
//            break;
//        }
//    }
//
//    // 设置字段
//    void SetProtoMsgFieldFromJsValue(million::ProtoMessage* msg
//        , const google::protobuf::Descriptor& desc
//        , const google::protobuf::Reflection& reflection
//        , const google::protobuf::FieldDescriptor& field_desc
//        , const mjs::Value& js_value)
//    {
//        switch (field_desc.type()) {
//        case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
//            if (!js_value.IsNumber()) {
//                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
//            }
//            reflection.SetDouble(msg, &field_desc, js_value.f64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
//            if (!js_value.IsNumber()) {
//                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
//            }
//            reflection.SetFloat(msg, &field_desc, js_value.f64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_INT64:
//        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
//        case google::protobuf::FieldDescriptor::TYPE_SINT64: {
//            if (!js_value.IsInt64()) {
//                TaskAbort("Field {}.{} is not a bigint.", desc.name(), field_desc.name());
//            }
//            reflection.SetInt64(msg, &field_desc, js_value.i64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_UINT64:
//        case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
//            if (!js_value.IsUInt64()) {
//                TaskAbort("Field {}.{} is not a bigint.", desc.name(), field_desc.name());
//            }
//            reflection.SetUInt64(msg, &field_desc, js_value.u64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_INT32:
//        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
//        case google::protobuf::FieldDescriptor::TYPE_SINT32: {
//            if (!js_value.IsNumber()) {
//                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
//            }
//            reflection.SetInt32(msg, &field_desc, js_value.i64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_UINT32:
//        case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
//            if (!js_value.IsNumber()) {
//                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
//            }
//            reflection.SetUInt32(msg, &field_desc, js_value.u64());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
//            if (!js_value.IsBoolean()) {
//                TaskAbort("Field {}.{} is not a bool.", desc.name(), field_desc.name());
//            }
//            reflection.SetBool(msg, &field_desc, js_value.boolean());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_STRING: {
//            if (!js_value.IsString()) {
//                TaskAbort("Field {}.{} is not a string.", desc.name(), field_desc.name());
//            }
//            reflection.SetString(msg, &field_desc, js_value.string_view());
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
//            // TODO: 实现二进制数据转换
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
//            if (!js_value.IsString()) {
//                TaskAbort("Field {}.{} is not a string(enum).", desc.name(), field_desc.name());
//            }
//
//            // 获取枚举描述符
//            const google::protobuf::EnumDescriptor* enum_desc = field_desc.enum_type();
//            if (!enum_desc) {
//                TaskAbort("Field {}.{} has no enum descriptor.", desc.name(), field_desc.name());
//            }
//
//            // 通过字符串查找枚举值
//            const google::protobuf::EnumValueDescriptor* enum_value = enum_desc->FindValueByName(js_value.string_view());
//            if (!enum_value) {
//                TaskAbort("Field {}.{} has no enum value named {}.", desc.name(), field_desc.name(), js_value.string_view());
//            }
//
//            // 设置枚举值
//            reflection.SetEnum(msg, &field_desc, enum_value);
//            break;
//        }
//        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
//            if (!js_value.IsObject()) {
//                TaskAbort("Field {}.{} is not a object.", desc.name(), field_desc.name());
//            }
//            auto sub_msg = reflection.MutableMessage(msg, &field_desc);
//            JsObjToProtoMsg(sub_msg, js_value);
//            break;
//        }
//        default:
//            TaskAbort("Field {}.{} cannot convert type.", desc.name(), field_desc.name());
//            break;
//        }
//    }
//
//    // 转换单个字段
//    void JsObjToProtoMsgOne(million::ProtoMessage* msg, const mjs::Value& field_value, const google::protobuf::FieldDescriptor& field_desc) {
//        const auto desc = msg->GetDescriptor();
//        const auto refl = msg->GetReflection();
//
//        if (field_value.IsUndefined()) {
//            return;  // 如果 JS 对象没有该属性，则跳过
//        }
//
//        if (field_desc.is_repeated()) {
//            if (!field_value.IsArrayObject()) {
//                TaskAbort("JsObjToProtoMsg: Not an array.");
//                return;
//            }
//
//            auto len = field_value.GetProperty("length").u64();
//            for (size_t j = 0; j < len; ++j) {
//                auto repeated_value = field_value.GetProperty(j);
//                SetProtoMsgRepeatedFieldFromJsValue(msg, *desc, *refl, field_desc, repeated_value, j);
//            }
//        }
//        else {
//            SetProtoMsgFieldFromJsValue(msg, *desc, *refl, field_desc, field_value);
//        }
//    }
//
//    // JS对象转Protobuf消息
//    void JsObjToProtoMsg(million::ProtoMessage* msg, const mjs::Value& obj) {
//        const auto desc = msg->GetDescriptor();
//        const auto refl = msg->GetReflection();
//
//        for (size_t i = 0; i < desc->field_count(); ++i) {
//            const auto field_desc = desc->field(i);
//            auto field_value = obj.GetProperty(field_desc->name().c_str());
//            JsObjToProtoMsgOne(msg, field_value, *field_desc);
//        }
//    }
//
//    // 调用JS函数
//    million::Task<million::MessagePointer> CallFunc(million::MessagePointer msg, std::string_view func_name) {
//        if (!msg.IsProtoMsg()) {
//            co_return nullptr;
//        }
//
//        auto proto_msg = std::move(msg.GetProtoMsg());
//        million::MessagePointer ret_msg;
//
//        // 获取模块的 namespace 对象
//        auto space = js_service_module_.module().namespace_obj();
//        
//        // 获取函数
//        auto func = space.GetProperty(func_name.data());
//        if (!func.IsFunctionObject()) {
//            co_return nullptr;
//        }
//
//        // 准备参数
//        std::vector<mjs::Value> args;
//        if (proto_msg) {
//            args.push_back(mjs::Value(proto_msg->GetDescriptor()->full_name().c_str()));
//            args.push_back(ProtoMsgToJsObj(*proto_msg));
//        }
//
//        // 调用函数
//        auto result = js_ctx_->CallFunction(&func, mjs::Value(), args.begin(), args.end());
//        
//        // 处理返回值
//        if (result.IsPromiseObject()) {
//            // 等待Promise完成
//            while (true) {
//                auto state = result.promise().state();
//                if (state != mjs::PromiseState::Pending) {
//                    break;
//                }
//
//                // 执行微任务
//                js_ctx_->ExecuteMicrotasks();
//
//                // 如果有等待的session，处理响应
//                if (func_ctx_.waiting_session_id) {
//                    auto res_msg = co_await Recv(*func_ctx_.waiting_session_id);
//                    if (res_msg.IsCppMsg()) {
//                        res_msg = co_await MsgDispatch(func_ctx_.sender, *func_ctx_.waiting_session_id, std::move(res_msg));
//                    }
//
//                    if (res_msg.IsProtoMsg()) {
//                        auto proto_res_msg = res_msg.GetProtoMsg();
//                        auto msg_obj = ProtoMsgToJsObj(*proto_res_msg);
//                        js_ctx_->CallFunction(&func_ctx_.resolving_funcs[0], mjs::Value(), &msg_obj, &msg_obj + 1);
//                    }
//                    else if (res_msg.IsType<JSValueMsg>()) {
//                        auto msg_obj = res_msg.GetMsg<JSValueMsg>()->value;
//                        js_ctx_->CallFunction(&func_ctx_.resolving_funcs[0], mjs::Value(), &msg_obj, &msg_obj + 1);
//                    }
//
//                    func_ctx_.waiting_session_id.reset();
//                }
//            }
//
//            if (state == mjs::PromiseState::Fulfilled) {
//                // 获取返回值
//                auto value = result.promise().value();
//                if (value.IsUndefined()) {
//                    co_return nullptr;
//                }
//
//                if (!value.IsArrayObject()) {
//                    logger().Err("Need to return undefined or array.");
//                    co_return nullptr;
//                }
//
//                // 解析返回值数组
//                auto msg_name = value.GetProperty(0);
//                if (!msg_name.IsString()) {
//                    logger().Err("message name must be a string.");
//                    co_return nullptr;
//                }
//
//                auto desc = imillion().proto_mgr().FindMessageTypeByName(msg_name.string_view());
//                if (!desc) {
//                    logger().Err("Invalid message type.");
//                    co_return nullptr;
//                }
//
//                auto ret_proto_msg = imillion().proto_mgr().NewMessage(*desc);
//                if (!ret_proto_msg) {
//                    logger().Err("New message failed.");
//                    co_return nullptr;
//                }
//
//                auto msg_obj = value.GetProperty(1);
//                if (!msg_obj.IsObject()) {
//                    logger().Err("message must be an object.");
//                    co_return nullptr;
//                }
//
//                JsObjToProtoMsg(ret_proto_msg.get(), msg_obj);
//                ret_msg = std::move(ret_proto_msg);
//            }
//            else if (state == mjs::PromiseState::Rejected) {
//                // Promise被拒绝，获取错误
//                auto error = result.promise().value();
//                logger().Err("JS Promise rejected: {}.", error.ToString(js_ctx_.get()).string_view());
//            }
//        }
//        else if (result.IsArrayObject()) {
//            // 直接返回数组形式的返回值
//            auto msg_name = result.GetProperty(0);
//            if (!msg_name.IsString()) {
//                logger().Err("message name must be a string.");
//                co_return nullptr;
//            }
//
//            auto desc = imillion().proto_mgr().FindMessageTypeByName(msg_name.string_view());
//            if (!desc) {
//                logger().Err("Invalid message type.");
//                co_return nullptr;
//            }
//
//            auto ret_proto_msg = imillion().proto_mgr().NewMessage(*desc);
//            if (!ret_proto_msg) {
//                logger().Err("New message failed.");
//                co_return nullptr;
//            }
//
//            auto msg_obj = result.GetProperty(1);
//            if (!msg_obj.IsObject()) {
//                logger().Err("message must be an object.");
//                co_return nullptr;
//            }
//
//            JsObjToProtoMsg(ret_proto_msg.get(), msg_obj);
//            ret_msg = std::move(ret_proto_msg);
//        }
//
//        co_return std::move(ret_msg);
//    }
//
//    // 初始化JS运行时和上下文
//    virtual bool OnInit() override {
//        js_rt_ = std::make_unique<mjs::Runtime>();
//        js_ctx_ = std::make_unique<mjs::Context>(js_rt_.get());
//
//        if (!CreateMillionModule()) {
//            TaskAbort("CreateMillionModule failed.");
//        }
//        if (!CreateServiceModule()) {
//            TaskAbort("CreateServiceModule failed.");
//        }
//        if (!CreateLoggerModule()) {
//            TaskAbort("CreateLoggerModule failed.");
//        }
//        if (!CreateDBModule()) {
//            TaskAbort("CreateDBModule failed.");
//        }
//        if (!CreateConfigModule()) {
//            TaskAbort("CreateConfigModule failed.");
//        }
//
//        LoadScript(package_);
//        return true;
//    }
//
//    // 加载JS脚本
//    void LoadScript(const std::string& package) {
//        try {
//            auto script = ReadModuleScript(package);
//            if (!script) {
//                TaskAbort("LoadModule failed: {}.", package);
//            }
//
//            js_service_module_ = js_ctx_->Eval(package, *script);
//            if (js_service_module_.IsException()) {
//                TaskAbort("LoadModule failed with exception.");
//            }
//        }
//        catch (...) {
//            if (js_service_module_.IsModuleObject()) {
//                js_service_module_ = mjs::Value();
//            }
//            throw;
//        }
//    }
//
//    // 读取模块脚本
//    std::optional<std::string> ReadModuleScript(const std::filesystem::path& module_path) {
//        auto file = std::ifstream(module_path);
//        if (!file) {
//            return std::nullopt;
//        }
//        auto content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//        file.close();
//        return content;
//    }
//
//    // 添加模块
//    bool AddModule(const std::string& module_name, mjs::ModuleObject* module) {
//        if (modules_.find(module_name) != modules_.end()) {
//            logger().Err("Module already exists: {}.", module_name);
//            return false;
//        }
//        modules_[module_name] = mjs::Value(module);
//        return true;
//    }
//
//    // 检查JS异常
//    bool JsCheckException(const mjs::Value& value) {
//        if (value.IsException()) {
//            logger().Err("JS Exception: {}.", value.ToString(js_ctx_.get()).string_view());
//            return false;
//        }
//        return true;
//    }
//
//private:
//    // mjs上下文
//    mjs::Context js_ctx_;
//    
//    // 模块相关
//    mjs::Value js_service_module_;
//    std::unordered_map<std::string, mjs::Value> modules_;
//    
//    // 其他成员
//    JSRuntimeService* js_runtime_service_;
//    std::string package_;
//    std::filesystem::path cur_path_;
//    ServiceFuncContext func_ctx_;
//};

} // namespace jssvr
} // namespace million