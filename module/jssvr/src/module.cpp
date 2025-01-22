#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

#include <yaml-cpp/yaml.h>

#include <google/protobuf/message.h>
#include <ss/ss_test.pb.h>

#include <million/imillion.h>
#include <million/msg.h>

#include <jssvr/api.h>

#include <quickjs/quickjs.h>
#include <quickjs/quickjs-libc.h>

struct ServiceFuncContext {
    JSValue promise_cap = JS_UNDEFINED;
    std::optional<million::SessionId> waiting_session_id;
};

class JsModuleService : public million::IService {
public:
    using Base = IService;
    using Base::Base;

private:
    virtual bool OnInit() override {
        const auto& config = imillion().YamlConfig();

        const auto& jssvr_config = config["jssvr"];
        if (!jssvr_config) {
            logger().Err("cannot find 'jssvr'.");
            return false;
        }
        if (!jssvr_config["dirs"]) {
            logger().Err("cannot find 'jssvr.dirs'.");
            return false;
        }
        jssvr_dirs_ = jssvr_config["dirs"].as<std::vector<std::string>>();

        return true;
    }

private:
    //JSValue FindModule(JSContext* js_ctx, std::filesystem::path path) {
    //    auto iter = module_bytecodes_map_.find(path);
    //    if (iter == module_bytecodes_map_.end()) {
    //        return JS_UNDEFINED;
    //    }

    //    // js_std_eval_binary;

    //    int flags = JS_READ_OBJ_BYTECODE;
    //    JSValue module = JS_ReadObject(js_ctx, iter->second.data(), iter->second.size(), flags);
    //    if (JS_VALUE_GET_TAG(module) == JS_TAG_MODULE) {
    //        if (JS_ResolveModule(js_ctx, module) < 0) {
    //            JS_FreeValue(js_ctx, module);
    //            return  JS_ThrowTypeError(js_ctx, "JS_ResolveModule < 0.");
    //        }
    //        js_module_set_import_meta(js_ctx, module, false, true);
    //        auto result = JS_EvalFunction(js_ctx, module);
    //        if (JS_IsException(result)) {
    //            return result;
    //        }
    //        // result = js_std_await(ctx, result);
    //    }
    //    return module;
    //}

    //JSValue LoadModule(JSContext* js_ctx, std::string* cur_path, const std::string& full_name) {


    //    auto script = ReadModuleScript(path);
    //    if (!script) {
    //        // 当前路径找不到，去配置路径找
    //        for (const auto& dir : jssvr_dirs_) {
    //            path = dir;
    //            path /= full_name + ".js";
    //            module = FindModule(js_ctx, path);
    //            if (!JS_IsUndefined(module)) {
    //                return module;
    //            }
    //        }

    //        for (const auto& dir : jssvr_dirs_) {
    //            path = dir;
    //            path /= full_name + ".js";
    //            script = ReadModuleScript(path);
    //            if (script) {
    //                *cur_path = dir;
    //                break;
    //            }
    //        }
    //        if (!script) {
    //            return JS_ThrowTypeError(js_ctx, "LoadModule failed: %s.", full_name.c_str());
    //        }
    //    }

    //    module = JS_Eval(js_ctx, script->data(), script->size(), full_name.c_str(), JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    //    if (JS_IsException(module)) {
    //        return module;
    //    }

    //    // 保存到已加载模块
    //    iter->second.emplace(full_name, module);

    //    uint8_t* out_buf;
    //    size_t out_buf_len;
    //    int flags = JS_WRITE_OBJ_BYTECODE;
    //    out_buf = JS_WriteObject(js_ctx, &out_buf_len, module, flags);
    //    if (!out_buf) {
    //        return JS_ThrowTypeError(js_ctx, "JS_WriteObject failed: %s.", full_name.c_str());
    //    }

    //    auto bytecodes = std::vector<uint8_t>(out_buf_len);
    //    std::memcpy(bytecodes.data(), out_buf, out_buf_len);
    //    js_free(js_ctx, out_buf);

    //    auto res = module_bytecodes_map_.emplace(std::move(path), std::move(bytecodes));
    //    assert(res.second);

    //    auto result = JS_EvalFunction(js_ctx, module);
    //    if (JS_IsException(result)) {
    //        return result;
    //    }

    //    return module;
    //}


public:
    std::vector<std::string> jssvr_dirs_;
    // std::unordered_map<std::filesystem::path, std::vector<uint8_t>> module_bytecodes_map_;
};

// JS_ToCString 可能存在内存泄漏，暂时懒得找了


class JsService : public million::IService {
public:
    using Base = IService;
    JsService(million::IMillion* imillion, JsModuleService* js_module_service, std::string package)
        : Base(imillion)
        , js_module_service_(js_module_service)
    {
        try {
            js_rt_ = JS_NewRuntime();
            // JS_SetDumpFlags(js_rt_, 1);

            if (!js_rt_) {
                throw std::runtime_error("JS_NewRuntime failed.");
            }
            JS_SetRuntimeOpaque(js_rt_, this);

            js_std_init_handlers(js_rt_);

            js_ctx_ = JS_NewContext(js_rt_);
            if (!js_ctx_) {
                throw std::runtime_error("JS_NewContext failed.");
            }

            js_init_module_std(js_ctx_, "std");
            js_init_module_os(js_ctx_, "os");

            JS_SetModuleLoaderFunc(js_rt_, nullptr, JsModuleLoader, nullptr);

            if (!CreateMillionModule()) {
                throw std::runtime_error("CreateLoggerModule failed.");
            }
            if (!CreateServiceModule()) {
                throw std::runtime_error("CreateServiceModule failed.");
            }
            if (!CreateLoggerModule()) {
                throw std::runtime_error("CreateLoggerModule failed.");
            }

            js_main_module_ = ModuleLoader(package + "/main");

            if (!JsCheckException(js_main_module_)) {
                throw std::runtime_error("LoadModule JsCheckException failed.");
            }
        } 
        catch (...) {
            if (!JS_IsUndefined(js_main_module_)) {
                JS_FreeValue(js_ctx_, js_main_module_);
            }
            if (js_ctx_) {
                JS_FreeContext(js_ctx_);
                js_ctx_ = nullptr;
            }
            if (js_rt_) {
                JS_FreeRuntime(js_rt_);
                js_rt_ = nullptr;
            }
            throw;
        }
    }

    using Base::Base;

    virtual ::million::Task<::million::MsgPtr> OnStart(::million::ServiceHandle sender, ::million::SessionId session_id) override {
        co_return nullptr;
    }

    virtual void OnStop(::million::ServiceHandle sender, ::million::SessionId session_id) override {

    }

    virtual million::Task<million::MsgPtr> OnMsg(million::ServiceHandle sender, million::SessionId session_id, million::MsgPtr msg) override {
        if (!msg.IsProtoMsg()) {
            // 只处理proto msg
            co_return nullptr;
        }
        auto proto_msg = std::move(msg.GetProtoMsg());

        JSModuleDef* js_main_module = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(js_main_module_));

        JSValue space = JS_UNDEFINED;
        JSValue onMsg_func = JS_UNDEFINED;
        JSValue par[2] = { JS_UNDEFINED };
        ServiceFuncContext func_ctx;
        JSValue promise = JS_UNDEFINED;
        JSValue result = JS_UNDEFINED;

        do {
            // 获取模块的 namespace 对象
            space = JS_GetModuleNamespace(js_ctx_, js_main_module);
            if (!JsCheckException(space)) break;

            onMsg_func = JS_GetPropertyStr(js_ctx_, space, "onMsg");
            if (!JsCheckException(onMsg_func)) break;
            if (!JS_IsFunction(js_ctx_, onMsg_func)) break;

            JS_SetContextOpaque(js_ctx_, &func_ctx);

            JSValue resolving_funcs[2];
            func_ctx.promise_cap = JS_NewPromiseCapability(js_ctx_, resolving_funcs);
            JSValue resolve_func = resolving_funcs[0];

            par[0] = JS_NewString(js_ctx_, proto_msg->GetDescriptor()->full_name().c_str());
            par[1] = ProtoMsgToJsObj(*proto_msg);
            promise = JS_Call(js_ctx_, onMsg_func, JS_UNDEFINED, 2, par);
            if (!JsCheckException(promise)) break;
            
            JSPromiseStateEnum state;
            do {
                state = JS_PromiseState(js_ctx_, promise);
                if (state != JS_PROMISE_PENDING) {
                    break;
                }

                if (!func_ctx.waiting_session_id) {
                    // logger
                    break;
                }

                auto res_msg = co_await Recv<million::ProtoMsg>(*func_ctx.waiting_session_id);

                // res_msg转js对象，唤醒
                JSValue msg_obj = ProtoMsgToJsObj(*res_msg);
                result = JS_Call(js_ctx_, resolve_func, JS_UNDEFINED, 1, &msg_obj);
                JS_FreeValue(js_ctx_, msg_obj);
                if (!JsCheckException(result)) break;

                func_ctx.waiting_session_id.reset();

                // 手动触发事件循环，确保异步操作继续执行
                JSContext* ctx_ = nullptr;
                while (JS_ExecutePendingJob(js_rt_, &ctx_)) {
                    // 执行事件循环，直到没有更多的异步任务
                }
            } while (true);

            if (state == JS_PROMISE_FULFILLED) {
                // 获取返回值
                result = JS_PromiseResult(js_ctx_, promise);
                if (!JsCheckException(result)) break;

                if (JS_IsUndefined(result)) {
                    break;
                }

                if (!JS_IsArray(js_ctx_, result)) {
                    logger().Err("Need to return undefined or array.");
                    break;
                }

                int64_t length = 0;
                if (JS_GetLength(js_ctx_, result, &length)) {
                    logger().Err("JS_GetLength failed.");
                    break;
                }

                if (length < 2) {
                    logger().Err("Need message name and object.");
                    break;
                }

                // 解析result，第一个是message name，第二个是js_obj
                auto msg_name = JS_GetPropertyUint32(js_ctx_, result, 0);

                if (!JS_IsString(msg_name)) {
                    logger().Err("message name must be a string.");
                    break;
                }
                auto msg_name_cstr = JS_ToCString(js_ctx_, msg_name);
                if (!msg_name_cstr) {
                    logger().Err("message name to cstring failed.");
                    break;
                }

                auto desc = imillion().proto_mgr().FindMessageTypeByName(msg_name_cstr);
                if (!desc) {
                    logger().Err("Invalid message type.");
                    break;
                }

                auto ret_msg = imillion().proto_mgr().NewMessage(*desc);
                if (!ret_msg) {
                    logger().Err("New message failed.");
                    break;
                }

                auto msg_obj = JS_GetPropertyUint32(js_ctx_, result, 1);
                if (!JS_IsObject(msg_obj)) {
                    logger().Err("message must be an object.");
                    break;
                }

                JsObjToProtoMsg(ret_msg.get(), msg_obj);
                co_return std::move(ret_msg);
            }

        } while (false);

        JS_FreeValue(js_ctx_, space);
        JS_FreeValue(js_ctx_, onMsg_func);
        JS_FreeValue(js_ctx_, par[0]);
        JS_FreeValue(js_ctx_, par[1]);
        JS_FreeValue(js_ctx_, func_ctx.promise_cap);
        JS_FreeValue(js_ctx_, promise);
        JS_FreeValue(js_ctx_, result);
        
        co_return nullptr;
    }

private:
    JSValue GetJsValueByProtoMsgField(const million::ProtoMsg& msg
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc)
    {
        JSValue js_value;
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            js_value = JS_NewFloat64(js_ctx_, reflection.GetDouble(msg, &field_desc));
            break;
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            js_value = JS_NewFloat64(js_ctx_, reflection.GetFloat(msg, &field_desc));
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
            js_value = JS_NewBigInt64(js_ctx_, reflection.GetInt64(msg, &field_desc));
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            js_value = JS_NewBigUint64(js_ctx_, reflection.GetUInt64(msg, &field_desc));
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
            js_value = JS_NewInt32(js_ctx_, reflection.GetInt32(msg, &field_desc));
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            js_value = JS_NewUint32(js_ctx_, reflection.GetUInt32(msg, &field_desc));
            break;
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            js_value = JS_NewBool(js_ctx_, reflection.GetBool(msg, &field_desc));
            break;
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            std::string string;
            auto string_ref = reflection.GetStringReference(msg, &field_desc, &string);
            js_value = JS_NewString(js_ctx_, string_ref.c_str());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            std::string bytes;
            auto bytes_ref = reflection.GetStringReference(msg, &field_desc, &bytes);
            js_value = JS_NewArrayBufferCopy(js_ctx_, reinterpret_cast<const uint8_t*>(bytes_ref.data()), bytes_ref.size());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            const auto enum_value = reflection.GetEnumValue(msg, &field_desc);
            js_value = JS_NewInt32(js_ctx_, enum_value);
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            const auto& sub_msg = reflection.GetMessage(msg, &field_desc);
            js_value = ProtoMsgToJsObj(sub_msg);
            break;
        }
        default:
            js_value = JS_UNDEFINED;  // 未知类型，返回未定义
            break;
        }
        return js_value;
    }

    JSValue GetJsValueByProtoMsgRepeatedField(const million::ProtoMsg& msg
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc
        , size_t j)
    {
        JSValue js_value;
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            js_value = JS_NewFloat64(js_ctx_, reflection.GetRepeatedDouble(msg, &field_desc, j));
            break;
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            js_value = JS_NewFloat64(js_ctx_, reflection.GetRepeatedFloat(msg, &field_desc, j));
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
            js_value = JS_NewBigInt64(js_ctx_, reflection.GetRepeatedInt64(msg, &field_desc, j));
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            js_value = JS_NewBigUint64(js_ctx_, reflection.GetRepeatedUInt64(msg, &field_desc, j));
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
            js_value = JS_NewInt32(js_ctx_, reflection.GetRepeatedInt32(msg, &field_desc, j));
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            js_value = JS_NewUint32(js_ctx_, reflection.GetRepeatedUInt32(msg, &field_desc, j));
            break;
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            js_value = JS_NewBool(js_ctx_, reflection.GetRepeatedBool(msg, &field_desc, j));
            break;
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            std::string string;
            auto string_ref = reflection.GetRepeatedStringReference(msg, &field_desc, j, &string);
            js_value = JS_NewString(js_ctx_, string_ref.c_str());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            std::string bytes;
            auto bytes_ref = reflection.GetRepeatedStringReference(msg, &field_desc, j, &bytes);
            js_value = JS_NewArrayBufferCopy(js_ctx_, reinterpret_cast<const uint8_t*>(bytes_ref.data()), bytes_ref.size());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            const auto enum_value = reflection.GetRepeatedEnumValue(msg, &field_desc, j);
            js_value = JS_NewInt32(js_ctx_, enum_value);
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            const auto& sub_msg = reflection.GetRepeatedMessage(msg, &field_desc, j);
            js_value = ProtoMsgToJsObj(sub_msg);
            break;
        }
        default:
            js_value = JS_UNDEFINED;  // 未知类型，返回未定义
            break;
        }
        return js_value;
    }

    JSValue ProtoMsgToJsObj(const million::ProtoMsg& msg) {
        JSValue obj = JS_NewObject(js_ctx_);

        const auto desc = msg.GetDescriptor();
        const auto reflection = msg.GetReflection();

        for (size_t i = 0; i < desc->field_count(); ++i) {
            const auto field_desc = desc->field(i);
            if (field_desc->is_repeated()) {
                JSValue js_array = JS_NewArray(js_ctx_);
                for (size_t j = 0; j < reflection->FieldSize(msg, field_desc); ++j) {
                    JSValue js_value = GetJsValueByProtoMsgRepeatedField(msg, *reflection, *field_desc, j);
                    JS_SetPropertyUint32(js_ctx_, js_array, j, js_value);  // 添加到数组中
                }
                JS_SetPropertyStr(js_ctx_, obj, field_desc->name().c_str(), js_array);
            }
            else {
                JSValue js_value = GetJsValueByProtoMsgField(msg, *reflection, *field_desc);
                JS_SetPropertyStr(js_ctx_, obj, field_desc->name().c_str(), js_value);  // 添加字段到对象中
            }
        }

        return obj;
    }

    void SetProtoMsgRepeatedFieldFromJsValue(million::ProtoMsg* msg
        , const google::protobuf::Descriptor& desc
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc
        , JSValue repeated_value , size_t j)
    {
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            double value;
            if (JS_ToFloat64(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddDouble(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            double value;
            if (JS_ToFloat64(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddFloat(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64: {
            if (!JS_IsBigInt(js_ctx_, repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a bigint.", desc.name(), field_desc.name(), j);
            }
            int64_t value;
            if (JS_ToBigInt64(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddInt64(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
            if (!JS_IsBigInt(js_ctx_, repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a bigint.", desc.name(), field_desc.name(), j);
            }
            uint64_t value;
            if (JS_ToBigUint64(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddUInt64(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            int32_t value;
            if (JS_ToInt32(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddInt32(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            uint32_t value;
            if (JS_ToUint32(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddUInt32(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
            if (!JS_IsBool(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a bool.", desc.name(), field_desc.name(), j);
            }
            reflection.AddBool(msg, &field_desc, JS_ToBool(js_ctx_, repeated_value));
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            if (!JS_IsString(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a string.", desc.name(), field_desc.name(), j);
            }
            const char* str = JS_ToCString(js_ctx_, repeated_value);
            if (!str) {
                TaskAbort("Field {}.{}[{}] to cstring failed.", desc.name(), field_desc.name(), j);
            }
            reflection.AddString(msg, &field_desc, str);
            JS_FreeCString(js_ctx_, str);
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            if (!JS_IsArrayBuffer(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a array buffer.", desc.name(), field_desc.name(), j);
            }

            size_t size;
            const uint8_t* data = JS_GetArrayBuffer(js_ctx_, &size, repeated_value);
            reflection.AddString(msg, &field_desc, std::string(reinterpret_cast<const char*>(data), size));
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a number.", desc.name(), field_desc.name(), j);
            }
            int32_t value;
            if (JS_ToInt32(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddEnumValue(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            if (!JS_IsObject(repeated_value)) {
                TaskAbort("Field {}.{}[{}] is not a object.", desc.name(), field_desc.name(), j);
            }

            JSValue sub_obj = repeated_value;
            const auto sub_msg = reflection.AddMessage(msg, &field_desc);
            JsObjToProtoMsg(sub_msg, sub_obj);
            break;
        }
        default:
            TaskAbort("Field {}.{}[{}] cannot convert type.", desc.name(), field_desc.name(), j);
            break;
        }
        
    }

    void SetProtoMsgFieldFromJsValue(million::ProtoMsg* msg
        , const google::protobuf::Descriptor& desc
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc
        , JSValue repeated_value)
    {
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            double value;
            if (JS_ToFloat64(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetDouble(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            double value;
            if (JS_ToFloat64(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetFloat(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64: {
            if (!JS_IsBigInt(js_ctx_, repeated_value)) {
                TaskAbort("Field {}.{} is not a bigint.", desc.name(), desc.name(), field_desc.name());
            }
            int64_t value;
            if (JS_ToBigInt64(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetInt64(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
            if (!JS_IsBigInt(js_ctx_, repeated_value)) {
                TaskAbort("Field {}.{} is not a bigint.", desc.name(), field_desc.name());
            }
            uint64_t value;
            if (JS_ToBigUint64(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetUInt64(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            int32_t value;
            if (JS_ToInt32(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetInt32(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            uint32_t value;
            if (JS_ToUint32(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetUInt32(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
            if (!JS_IsBool(repeated_value)) {
                TaskAbort("Field {}.{} is not a bool.", desc.name(), field_desc.name());
            }
            reflection.SetBool(msg, &field_desc, JS_ToBool(js_ctx_, repeated_value));
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            if (!JS_IsString(repeated_value)) {
                TaskAbort("Field {}.{} is not a string.", desc.name(), field_desc.name());
            }
            const char* str = JS_ToCString(js_ctx_, repeated_value);
            if (!str) {
                TaskAbort("Field {}.{} to cstring failed.", desc.name(), field_desc.name());
            }
            reflection.SetString(msg, &field_desc, str);
            JS_FreeCString(js_ctx_, str);
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            if (!JS_IsArrayBuffer(repeated_value)) {
                TaskAbort("Field {}.{} is not a array buffer.", desc.name(), field_desc.name());
            }

            size_t size;
            const uint8_t* data = JS_GetArrayBuffer(js_ctx_, &size, repeated_value);
            reflection.SetString(msg, &field_desc, std::string(reinterpret_cast<const char*>(data), size));
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            if (!JS_IsNumber(repeated_value)) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            int32_t value;
            if (JS_ToInt32(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetEnumValue(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            if (!JS_IsObject(repeated_value)) {
                TaskAbort("Field {}.{} is not a object.", desc.name(), field_desc.name());
            }
            JSValue sub_obj = repeated_value;
            auto sub_msg = reflection.MutableMessage(msg, &field_desc);
            JsObjToProtoMsg(sub_msg, sub_obj);
            break;
        }
        default:
            TaskAbort("Field {}.{} cannot convert type.", desc.name(), field_desc.name());
            break;
        }

    }


    void JsObjToProtoMsg(million::ProtoMsg* msg, JSValue obj) {
        const auto desc = msg->GetDescriptor();
        const auto reflection = msg->GetReflection();

        for (size_t i = 0; i < desc->field_count(); ++i) {
            const auto field_desc = desc->field(i);

            JSValue field_value = JS_GetPropertyStr(js_ctx_, obj, field_desc->name().c_str());

            if (JS_IsUndefined(field_value)) {
                continue;  // 如果 JS 对象没有该属性，则跳过
            }

            if (field_desc->is_repeated()) {
                for (size_t j = 0; j < reflection->FieldSize(*msg, field_desc); ++j) {
                    JSValue repeated_value = JS_GetPropertyUint32(js_ctx_, field_value, j);
                    SetProtoMsgRepeatedFieldFromJsValue(msg, *desc, *reflection, *field_desc, repeated_value, j);
                }
            }
            else {
                SetProtoMsgFieldFromJsValue(msg, *desc, *reflection, *field_desc, field_value);
            }
        }
    }

    bool JsCheckException(JSValue value) {
        if (JS_IsException(value)) {
            JSValue exception = JS_GetException(js_ctx_);
            const char* error = JS_ToCString(js_ctx_, exception);
            // logger().Err("JS Exception: {}.", error);
            std::cout << error << std::endl;
            JS_FreeCString(js_ctx_, error);
            JS_FreeValue(js_ctx_, exception);
            return false;
        }
        return true;
    }


    static JSModuleDef* JsModuleLoader(JSContext* ctx, const char* module_name, void* opaque) {
        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));

        auto module_ = service->ModuleLoader(module_name);
        if (JS_IsException(module_)) {
            return nullptr;
        }

        return static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(module_));
    }

    JSValue ModuleLoader(const std::string& module_name) {
        // 首先找已加载的模块
        //auto iter2 = iter->second.find(full_name);
        //if (iter2 != iter->second.end()) {
        //    return iter2->second;
        //}

        std::string module_name_ = module_name;
        std::filesystem::path path = cur_path_;
        // 从模块所在路径找模块
        path /= module_name_ + ".js";
        auto script = ReadModuleScript(path);
        if (!script) {
            // 当前路径找不到，去配置路径找
            for (const auto& dir : js_module_service_->jssvr_dirs_) {
                path = dir;
                path /= module_name_ + ".js";
                script = ReadModuleScript(path);
                if (script) {
                    if (cur_path_.empty()) {
                        cur_path_ = path.parent_path();
                    }
                    break;
                }
            }

            if (!script) {
                return JS_ThrowTypeError(js_ctx_, "LoadModule failed: %s.", module_name_.c_str());
            }
        }

        auto module_ = JS_Eval(js_ctx_, script->data(), script->size(), module_name_.c_str(), JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JsCheckException(module_)) {
            return module_;
        }

        auto result = JS_EvalFunction(js_ctx_, module_);
        if (JS_IsException(result)) {
            return result;
        }

        //// 获取一下当前路径
        //auto module = service->js_module_service_->LoadModule(ctx, &service->cur_path_, module_name);
        //if (service->JsCheckException(module)) return nullptr;
        return module_;;
    }

    std::optional<std::string> ReadModuleScript(const std::filesystem::path& module_path) {
        auto file = std::ifstream(module_path);
        if (!file) {
            return std::nullopt;
        }
        auto content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return content;
    }


    static JSValue ServiceModuleSend(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        auto res = ServiceModuleCall(ctx, this_val, argc, argv);
        
        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        auto func_ctx = static_cast<ServiceFuncContext*>(JS_GetContextOpaque(ctx));
        func_ctx->waiting_session_id = million::kSessionIdInvalid;

        // 未来考虑改掉，这样返回的会可以co_await，并且错误提示也有点问题
        return res;
    }

    static JSValue ServiceModuleCall(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
       if (argc < 3) {
            return JS_ThrowTypeError(ctx, "ServiceModuleCall argc: %d.", argc);
       }

        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        auto func_ctx = static_cast<ServiceFuncContext*>(JS_GetContextOpaque(ctx));

        const char* service_name = nullptr;
        const char* msg_name = nullptr;

        JSValue result = func_ctx->promise_cap;
        do {
            if (!JS_IsString(argv[0])) {
                result = JS_ThrowTypeError(ctx, "ServiceModuleCall 1 argument must be a string.");
                break;
            }
            service_name = JS_ToCString(ctx, argv[0]);
            if (!service_name) {
                result = JS_ThrowInternalError(ctx, "ServiceModuleCall failed to convert first argument to string.");
                break;
            }

            auto target = service->imillion().GetServiceByName(service_name);
            if (!target) {
                result = JS_ThrowInternalError(ctx, "ServiceModuleCall Service does not exist: %s .", service_name);
                break;
            }


            if (!JS_IsString(argv[1])) {
                result = JS_ThrowTypeError(ctx, "ServiceModuleCall 2 argument must be a string.");
                break;
            }
            msg_name = JS_ToCString(ctx, argv[1]);
            if (!msg_name) {
                result = JS_ThrowInternalError(ctx, "ServiceModuleCall failed to convert 2 argument to string.");
                break;
            }

            if (!JS_IsObject(argv[2])) {
                result = JS_ThrowTypeError(ctx, "ServiceModuleCall 3 argument must be a object.");
                break;
            }

            auto desc = service->imillion().proto_mgr().FindMessageTypeByName(msg_name);
            if (!desc) {
                result = JS_ThrowTypeError(ctx, "ServiceModuleCall 2 argument Invalid message type.");
                break;
            }

            auto msg = service->imillion().proto_mgr().NewMessage(*desc);
            if (!msg) {
                result = JS_ThrowTypeError(ctx, "new message failed.");
                break;
            }

            // 发送消息，并将session_id传回
            func_ctx->waiting_session_id = service->Send(*target, std::move(msg));

        } while (false);
        
        JS_FreeCString(ctx, service_name);
        JS_FreeCString(ctx, msg_name);

        return result;
    }

    static JSCFunctionListEntry* ServiceModuleExportList(size_t* count) {
        static JSCFunctionListEntry list[] = {
            JS_CFUNC_DEF("send", 2, ServiceModuleSend),
            JS_CFUNC_DEF("call", 2, ServiceModuleCall),
        };
        *count = sizeof(list) / sizeof(JSCFunctionListEntry);
        return list;
    }

    static int ServiceModuleInit(JSContext* ctx, JSModuleDef* m) {
        // 导出函数到模块
        size_t count = 0;
        auto list = ServiceModuleExportList(&count);
        return JS_SetModuleExportList(ctx, m, list, count);
    }

    JSModuleDef* CreateServiceModule() {
        JSModuleDef* module = JS_NewCModule(js_ctx_, "service", ServiceModuleInit);
        if (!module) {
            logger().Err("JS_NewCModule failed: {}.", "service");
            return nullptr;
        }
        size_t count = 0;
        auto list = ServiceModuleExportList(&count);
        JS_AddModuleExportList(js_ctx_, module, list, count);
        if (!AddModule("service", module)) {
            logger().Err("JsAddModule failed: {}.", "service");
            return nullptr;
        }
        return module;
    }


    static JSValue LoggerModuleDebug(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        if (argc < 1) {
            return JS_ThrowTypeError(ctx, "LoggerModuleErr argc: %d.", argc);
        }

        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        
        const char* info;
        if (!JS_IsString(argv[0])) {
            return JS_ThrowTypeError(ctx, "ServiceModuleCall 1 argument must be a string.");
        }
        info = JS_ToCString(ctx, argv[0]);
        if (!info) {
            return JS_ThrowInternalError(ctx, "ServiceModuleCall failed to convert first argument to string.");
        }

        service->logger().Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_DEBUG, info);

        return JS_UNDEFINED;
    }
    
    static JSValue LoggerModuleInfo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        if (argc < 1) {
            return JS_ThrowTypeError(ctx, "LoggerModuleErr argc: %d.", argc);
        }

        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));

        const char* info;
        if (!JS_IsString(argv[0])) {
            return JS_ThrowTypeError(ctx, "ServiceModuleCall 1 argument must be a string.");
        }
        info = JS_ToCString(ctx, argv[0]);
        if (!info) {
            return JS_ThrowInternalError(ctx, "ServiceModuleCall failed to convert first argument to string.");
        }

        service->logger().Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_INFO, info);

        return JS_UNDEFINED;
    }

    static JSValue LoggerModuleWarn(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        if (argc < 1) {
            return JS_ThrowTypeError(ctx, "LoggerModuleErr argc: %d.", argc);
        }

        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));

        const char* info;
        if (!JS_IsString(argv[0])) {
            return JS_ThrowTypeError(ctx, "ServiceModuleCall 1 argument must be a string.");
        }
        info = JS_ToCString(ctx, argv[0]);
        if (!info) {
            return JS_ThrowInternalError(ctx, "ServiceModuleCall failed to convert first argument to string.");
        }

        service->logger().Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_WARN, info);

        return JS_UNDEFINED;
    }

    static JSValue LoggerModuleErr(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        if (argc < 1) {
            return JS_ThrowTypeError(ctx, "LoggerModuleErr argc: %d.", argc);
        }

        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));

        const char* info;
        if (!JS_IsString(argv[0])) {
            return JS_ThrowTypeError(ctx, "ServiceModuleCall 1 argument must be a string.");
        }
        info = JS_ToCString(ctx, argv[0]);
        if (!info) {
            return JS_ThrowInternalError(ctx, "ServiceModuleCall failed to convert first argument to string.");
        }

        service->logger().Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_ERR, info);

        return JS_UNDEFINED;
    }

    static JSCFunctionListEntry* LoggerModuleExportList(size_t* count) {
        static JSCFunctionListEntry list[] = {
            JS_CFUNC_DEF("debug", 1, LoggerModuleDebug),
            JS_CFUNC_DEF("info", 1, LoggerModuleInfo),
            JS_CFUNC_DEF("warn", 1, LoggerModuleWarn),
            JS_CFUNC_DEF("err", 1, LoggerModuleErr),
        };
        *count = sizeof(list) / sizeof(JSCFunctionListEntry);
        return list;
    }

    static int LoggerModuleInit(JSContext* ctx, JSModuleDef* m) {
        // 导出函数到模块
        size_t count = 0;
        auto list = LoggerModuleExportList(&count);
        return JS_SetModuleExportList(ctx, m, list, count);
    }

    JSModuleDef* CreateLoggerModule() {
        JSModuleDef* module = JS_NewCModule(js_ctx_, "logger", LoggerModuleInit);
        if (!module) {
            logger().Err("JS_NewCModule failed: {}.", "logger");
            return nullptr;
        }
        size_t count = 0;
        auto list = LoggerModuleExportList(&count);
        JS_AddModuleExportList(js_ctx_, module, list, count);
        if (!AddModule("logger", module)) {
            logger().Err("JsAddModule failed: {}.", "logger");
            return nullptr;
        }
        return module;
    }



    static JSValue MillionModuleNewService(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        if (argc < 1) {
            return JS_ThrowTypeError(ctx, "MillionModuleNewService argc: %d.", argc);
        }

        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));

        if (!JS_IsString(argv[0])) {
            return JS_ThrowTypeError(ctx, "MillionModuleNewService 1 argument must be a string.");
        }
        auto module_name = JS_ToCString(ctx, argv[0]);
        if (!module_name) {
            return JS_ThrowInternalError(ctx, "MillionModuleNewService failed to convert first argument to string.");
        }

        try {
            auto handle = service->imillion().NewService<JsService>(service->js_module_service_, module_name);
            if (!handle) {
                return JS_ThrowInternalError(ctx, "MillionModuleNewService New JsService failed.");
            }
        }
        catch (const std::exception& e) {
            return JS_ThrowInternalError(ctx, std::format("MillionModuleNewService New JsService failed: {}.", e.what()).c_str());
        }
        
        return JS_UNDEFINED;
    }

    static JSCFunctionListEntry* MillionModuleExportList(size_t* count) {
        static JSCFunctionListEntry list[] = {
            JS_CFUNC_DEF("newservice", 1, MillionModuleNewService),
        };
        *count = sizeof(list) / sizeof(JSCFunctionListEntry);
        return list;
    }

    static int MillionModuleInit(JSContext* ctx, JSModuleDef* m) {
        // 导出函数到模块
        size_t count = 0;
        auto list = MillionModuleExportList(&count);
        return JS_SetModuleExportList(ctx, m, list, count);
    }

    JSModuleDef* CreateMillionModule() {
        JSModuleDef* module = JS_NewCModule(js_ctx_, "million", MillionModuleInit);
        if (!module) {
            logger().Err("JS_NewCModule failed: {}.", "million");
            return nullptr;
        }
        size_t count = 0;
        auto list = MillionModuleExportList(&count);
        JS_AddModuleExportList(js_ctx_, module, list, count);
        if (!AddModule("million", module)) {
            logger().Err("JsAddModule failed: {}.", "million");
            return nullptr;
        }
        return module;
    }

    bool AddModule(const std::string& module_name, JSModuleDef* module) {
        if (modules_.find(module_name) != modules_.end()) {
            logger().Err("Module already exists: {}.", module_name);
            return false;
        }
        JSValue module_obj = JS_GetImportMeta(js_ctx_, module);
        auto res = modules_.emplace(module_name, module_obj);
        assert(res.second);

        // static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(func_val))
    return true;
}

private:
    JsModuleService* js_module_service_;

    std::unordered_map<std::string, JSValue> modules_;
    JSRuntime* js_rt_ = nullptr;
    JSContext* js_ctx_ = nullptr;
    JSValue js_main_module_ = JS_UNDEFINED;
    std::filesystem::path cur_path_;
};


namespace million {
namespace jssvr {

extern "C" MILLION_JSSVR_API bool MillionModuleInit(IMillion* imillion) {
    auto handle = imillion->NewService<JsModuleService>();
    auto ptr = handle->get_ptr<JsModuleService>(handle->lock());

    auto config = imillion->YamlConfig();
    const auto& jssvr_config = config["jssvr"];
    if (!jssvr_config) {
        imillion->logger().Err("cannot find 'jssvr'.");
        return false;
    }

    const auto& bootstarp_config = jssvr_config["bootstarp"];
    if (!bootstarp_config) {
        imillion->logger().Err("cannot find 'jssvr.bootstarp'.");
        return false;
    }
    auto bootstarp = bootstarp_config.as<std::string>();

    try {
        handle = imillion->NewService<JsService>(ptr, bootstarp);
    }
    catch (const std::exception& e) {
        imillion->logger().Err("New JsService failed.", e.what());
    }
    
    return true;
}

} // namespace jssvr
} // namespace million