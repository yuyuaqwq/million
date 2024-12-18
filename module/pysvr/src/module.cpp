#include "iostream"

#include <google/protobuf/message.h>
#include <string>

#include <million/imillion.h>
#include <million/msg.h>

#include <pysvr/api.h>

#include <quickjs/quickjs.h>
#include <quickjs/quickjs-libc.h>


class JsService : public million::IService {
public:
    using Base = IService;
    JsService(million::IMillion* imillion)
        : Base(imillion) {}

    using Base::Base;



    virtual bool OnInit() override {
        bool success = false;
        do {
            js_rt_ = JS_NewRuntime();
            if (!js_rt_) {
                logger().Err("JS_NewRuntime failed.");
                break;
            }
            JS_SetRuntimeOpaque(js_rt_, this);

            js_std_init_handlers(js_rt_);

            js_ctx_ = JS_NewContext(js_rt_);
            if (!js_ctx_) {
                logger().Err("JS_NewContext failed.");
                break;
            }

            js_init_module_std(js_ctx_, "std");
            js_init_module_os(js_ctx_, "os");

            JS_SetModuleLoaderFunc(js_rt_, nullptr, JsModuleLoader, nullptr);

            if (!CreateServiceModule()) {
                logger().Err("CreateServiceModule failed.");
                break;
            }

            const char* test_val = R"(
              import * as std from 'std';
              import * as os from 'os';
              import * as service from 'service';
              service.send();
              var console = {};
              console.log = value => std.printf(value + "\n");
              console.log('ok')
              for(var key in globalThis){
                console.log("--->" + key)
              }
              for(var key in std){
                console.log("--->" + key)
              }
              for(var key in os){
                console.log("--->" + key)
              }
              std.printf('hello_world\n');
              std.printf(globalThis + "\n");
              var a = false;
              os.setTimeout(()=>{std.printf('AAB\n')}, 2000);
              std.printf(a + "\n");
            )";
          if (!JsCreateModule("test", test_val)) break;

          //result = JS_Call(js_ctx_, init, JS_UNDEFINED, 0, nullptr);  // 调用异步函数
          //if (!JsCheckException(result)) break;

          // js_main_module_ = ;

          success = true;
        } while (false);
       
        if (!success) {
            if (js_rt_) {
                JS_FreeRuntime(js_rt_);
                js_rt_ = nullptr;
            }
            if (js_ctx_) {
                JS_FreeContext(js_ctx_);
                js_ctx_ = nullptr;
            }
        }

        return success;
    }

    virtual void OnStop(::million::ServiceHandle sender, ::million::SessionId session_id) override {

    }

    virtual million::Task<> OnMsg(million::ServiceHandle sender, million::SessionId session_id, million::MsgUnique) override {
        JSValue resolving_funcs[2];
        JSValue promise = JS_NewPromiseCapability(js_ctx_, resolving_funcs);
        JSValue resolve_func = resolving_funcs[0];

        JSValue on_msg = JS_GetPropertyStr(js_ctx_, js_main_module_, "on_msg");

        JSValue result;

        ServiceFuncContext func_ctx;
        JS_SetContextOpaque(js_ctx_, &func_ctx);

        // 传入sender，session_id，msg
        result = JS_Call(js_ctx_, on_msg, JS_UNDEFINED, 0, nullptr);  // 调用异步函数
        if (!JsCheckException(result)) co_return;

        if (func_ctx.waiting_session_id != million::kSessionIdInvalid) {
            auto res_msg = co_await Recv<million::ProtoMessage>(func_ctx.waiting_session_id);

            // res_msg转js对象，传入
            // result = JS_Call(js_ctx_, resolve_func, JS_UNDEFINED, 1, &);
            if (!JsCheckException(result)) co_return;


        }

        co_return;
    }


private:
    struct ServiceFuncContext {
        million::SessionId waiting_session_id = million::kSessionIdInvalid;
    };
    
    JSValue GetJsValueByProtoMsgField(const million::ProtoMessage& msg, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field_desc) {
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

    JSValue GetJsValueByProtoMsgRepeatedField(const million::ProtoMessage& msg, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field_desc, size_t j) {
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

    JSValue ProtoMsgToJsObj(const million::ProtoMessage& msg) {
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

    void SetProtoMsgRepeatedFieldFromJsValue(million::ProtoMessage* msg
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc
        , JSValue repeated_value
        , size_t j) {
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
            if (!JS_IsNumber(repeated_value)) {

            }
            double value;
            if (JS_ToFloat64(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddDouble(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
            if (!JS_IsNumber(repeated_value)) {

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

            }
            uint32_t value;
            if (JS_ToUint32(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddUInt32(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
            if (!JS_IsBool(repeated_value)) {

            }
            reflection.AddBool(msg, &field_desc, JS_ToBool(js_ctx_, repeated_value));
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            if (!JS_IsString(repeated_value)) {

            }
            const char* str = JS_ToCString(js_ctx_, repeated_value);
            reflection.AddString(msg, &field_desc, str);
            JS_FreeCString(js_ctx_, str);
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            if (!JS_IsArrayBuffer(repeated_value)) {

            }

            size_t size;
            const uint8_t* data = JS_GetArrayBuffer(js_ctx_, &size, repeated_value);
            reflection.AddString(msg, &field_desc, std::string(reinterpret_cast<const char*>(data), size));
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            if (!JS_IsNumber(repeated_value)) {

            }
            int32_t value;
            if (JS_ToInt32(js_ctx_, &value, repeated_value) == 0) {
                reflection.AddEnumValue(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            if (!JS_IsObject(repeated_value)) {

            }

            JSValue sub_obj = repeated_value;
            // 递归调用，假设ProtoMsgToJsObj已支持递归
            const auto sub_msg = reflection.AddMessage(msg, &field_desc);
            JsObjToProtoMsg(sub_msg, sub_obj);
            
            break;
        }
        default:
            break;
        }
        
    }

    void SetProtoMsgFieldFromJsValue(million::ProtoMessage* msg
        , const google::protobuf::Reflection& reflection
        , const google::protobuf::FieldDescriptor& field_desc
        , JSValue repeated_value) {
        switch (field_desc.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
            if (!JS_IsNumber(repeated_value)) {

            }
            double value;
            if (JS_ToFloat64(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetDouble(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
            if (!JS_IsNumber(repeated_value)) {

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

            }
            uint32_t value;
            if (JS_ToUint32(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetUInt32(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL: {
            if (!JS_IsBool(repeated_value)) {

            }
            reflection.SetBool(msg, &field_desc, JS_ToBool(js_ctx_, repeated_value));
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING: {
            if (!JS_IsString(repeated_value)) {

            }
            const char* str = JS_ToCString(js_ctx_, repeated_value);
            reflection.SetString(msg, &field_desc, str);
            JS_FreeCString(js_ctx_, str);
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES: {
            if (!JS_IsArrayBuffer(repeated_value)) {

            }

            size_t size;
            const uint8_t* data = JS_GetArrayBuffer(js_ctx_, &size, repeated_value);
            reflection.SetString(msg, &field_desc, std::string(reinterpret_cast<const char*>(data), size));
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM: {
            if (!JS_IsNumber(repeated_value)) {

            }
            int32_t value;
            if (JS_ToInt32(js_ctx_, &value, repeated_value) == 0) {
                reflection.SetEnumValue(msg, &field_desc, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            if (!JS_IsObject(repeated_value)) {

            }
            JSValue sub_obj = repeated_value;
            auto sub_msg = reflection.MutableMessage(msg, &field_desc);
            JsObjToProtoMsg(sub_msg, sub_obj);
            break;
        }
        default:
            break;
        }

    }


    void JsObjToProtoMsg(million::ProtoMessage* msg, JSValue obj) {
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
                    SetProtoMsgRepeatedFieldFromJsValue(msg, *reflection, *field_desc, repeated_value, j);
                }
            }
            else {
                SetProtoMsgFieldFromJsValue(msg, *reflection, *field_desc, field_value);
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

        auto iter = service->js_modules_.find(module_name);
        if (iter == service->js_modules_.end()) {
            return nullptr;
        }
        return iter->second;
    }

    bool JsAddModule(std::string_view module_name, JSModuleDef* module) {
        if (js_modules_.find(module_name.data()) != js_modules_.end()) {
            logger().Err("Module already exists: {}.", module_name);
            return false;
        }
        auto res = js_modules_.emplace(module_name, module);
        assert(res.second);
        return res.first->second;
    }

    JSModuleDef* JsCreateModule(std::string_view module_name, std::string_view js_script) {
        JSValue func_val = JS_Eval(js_ctx_, js_script.data(), js_script.size(), module_name.data(), JS_EVAL_TYPE_MODULE);
        if (!JsCheckException(func_val)) {
            return nullptr;
        }
        auto module = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(func_val));
        if (!JsAddModule(module_name, module)) {
            JS_FreeValue(js_ctx_, func_val);
            return nullptr;
        }
        return module;
    }



    static JSValue ServiceModuleSend(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        JsService* service = static_cast<JsService*>(JS_GetContextOpaque(ctx));
        //js_std_eval_binary

        //service->Send();
        return JSValue();
    }

    static JSValue ServiceModuleCall(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        JsService* service = static_cast<JsService*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        if (argc < 1) {
            return JS_ThrowTypeError(ctx, "ServiceModuleCall argc: %d.", argc);
        }

        auto func_ctx = static_cast<ServiceFuncContext*>(JS_GetContextOpaque(ctx));
        
        const char* service_name;
        if (!JS_IsString(argv[0])) {
            return JS_ThrowTypeError(ctx, "ServiceModuleCall first argument must be a string.");
        }
        service_name = JS_ToCString(ctx, argv[0]);
        if (!service_name) {
            return JS_ThrowInternalError(ctx, "ServiceModuleCall failed to convert first argument to string.");
        }

        auto target = service->imillion().GetServiceByName(service_name);
        if (!target) {
            return JS_ThrowInternalError(ctx, "ServiceModuleCall Service does not exist: %s .", service_name);
        }

        // 发送消息，并将session_id传回
        func_ctx->waiting_session_id = service->Send<million::ProtoMessage>(*target);

        JSValue* promise = static_cast<JSValue*>(JS_GetContextOpaque(ctx));
        return *promise;
    }

    static int ServiceModuleInit(JSContext* ctx, JSModuleDef* m) {
        // 导出函数到模块
        JSCFunctionListEntry list[] = {
            JS_CFUNC_DEF("send", 2, ServiceModuleSend),
            JS_CFUNC_DEF("call", 2, ServiceModuleCall),
        };
        return JS_SetModuleExportList(ctx, m, list, sizeof(list) / sizeof(JSCFunctionListEntry));
    }

    JSModuleDef* CreateServiceModule() {
        JSModuleDef* module = JS_NewCModule(js_ctx_, "service", ServiceModuleInit);
        if (!module) {
            logger().Err("JS_NewCModule failed: {}.", "service");
            return nullptr;
        }
        JS_AddModuleExport(js_ctx_, module, "send");
        JS_AddModuleExport(js_ctx_, module, "call");
        if (!JsAddModule("service", module)) {
            logger().Err("JsAddModule failed: {}.", "service");
            // 不释放module，等自动回收
            return nullptr;
        }

        //js_std_eval_binary

        return module;
    }

private:
    JSRuntime* js_rt_ = nullptr;
    JSContext* js_ctx_ = nullptr;
    JSValue js_main_module_ = JS_UNDEFINED;

    std::unordered_map<std::string, JSModuleDef*> js_modules_;
};


namespace million {
namespace pysvr {

// C端的Call函数，创建一个Promise并返回
JSValue Call(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    // 判断传递进来的参数类型
    if (argc < 1 || !JS_IsObject(argv[0])) {
        printf("Error: Expected an object as argument.\n");
        return JS_EXCEPTION;  // 返回异常
    }

    // 复用一个Promise？
    return argv[0];

    // 创建一个新的Promise
    //JSValue resolving_funcs[2];
    //JSValue cap = JS_NewPromiseCapability(ctx, resolving_funcs);

    //JS_SetPropertyStr(ctx, argv[0], "resolve", resolving_funcs[0]);  // 设置 resolve 函数
    //JS_SetPropertyStr(ctx, argv[0], "reject", resolving_funcs[1]);   // 设置 reject 函数

    // return cap;
}


extern "C" MILLION_PYSVR_API  bool MillionModuleInit(IMillion* imillion) {
    //imillion->NewService<JsService>();


    auto count = 1;
    for (auto i = 0; i < count; ++i) {

    
    // 创建 JavaScript 引擎实例
    JSRuntime* runtime = JS_NewRuntime();
    if (!runtime) {
        fprintf(stderr, "无法创建 JavaScript 引擎\n");
        return 1;
    }

    js_std_init_handlers(runtime);

    // 创建新的执行环境
    JSContext* ctx = JS_NewContext(runtime);
    if (!ctx) {
        fprintf(stderr, "无法创建 JavaScript 上下文\n");
        JS_FreeRuntime(runtime);
        return 1;
    }

    JSValue result;

    /* system modules */
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");
    const char* str =
        "import * as std from 'std';\n"
        "import * as os from 'os';\n"
        "globalThis.std = std;\n"
        "globalThis.os = os;\n"
        "var console = {};\n"
        "console.log = value => std.printf(value);\n";
    JSValue init_compile =
        JS_Eval(ctx, str, strlen(str), "<main>", JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

     js_module_set_import_meta(ctx, init_compile, 1, 1);
     JSValue init_run = JS_EvalFunction(ctx, init_compile);



    const char* test_val = R"(
    var console = {};
    console.log = value => globalThis.std.printf(value + "\n");
    console.log('ok')
    for(var key in globalThis){
      console.log("--->" + key)
    }
    for(var key in std){
      console.log("--->" + key)
    }
    for(var key in os){
      console.log("--->" + key)
    }
    globalThis.std.printf('hello_world\n');
    globalThis.std.printf(globalThis + "\n");
    var a = false;
    os.setTimeout(()=>{std.printf('AAB\n')}, 2000)
    globalThis.std.printf(a + "\n");
  )";
    result = JS_Eval(ctx, test_val, strlen(test_val), "test", JS_EVAL_TYPE_MODULE);

    // 创建 JavaScript 引擎的执行环境
    JSValue global = JS_GetGlobalObject(ctx);

    // 在全局对象上注册 Call 函数
    JS_SetPropertyStr(ctx, global, "Call", JS_NewCFunction(ctx, Call, "Call", 0));

    // JavaScript 代码（函数 A）
    const char* js_code =
        "async function asyncFunction(obj) {"
        "    std.printf('!!!start!!!');"
        "    let result = await Call(obj);"  // 暴露全局的 resolve 函数
        "    std.printf(result);"
        "    return result;"
        "}";

    // 执行 JavaScript 代码
    result = JS_Eval(ctx, js_code, strlen(js_code), "<eval>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        // 获取异常值
        JSValue exception = JS_GetException(ctx);
        const char* error_str = JS_ToCString(ctx, exception);
        printf("Script execution failed: %s\n", error_str);
        JS_FreeCString(ctx, error_str);
        JS_FreeValue(ctx, exception);
    }

    JSValue async_func = JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), "asyncFunction");

    // JSValue obj = JS_NewObject(ctx);
    JSValue resolving_funcs[2];
    JSValue cap = JS_NewPromiseCapability(ctx, resolving_funcs);
    JSValue resolve_func = resolving_funcs[0];

    result = JS_Call(ctx, async_func, JS_UNDEFINED, 1, &cap);  // 调用异步函数
    if (JS_IsException(result)) {
        // 获取异常值
        JSValue exception = JS_GetException(ctx);
        const char* error_str = JS_ToCString(ctx, exception);
        printf("Script execution failed: %s\n", error_str);
        JS_FreeCString(ctx, error_str);
        JS_FreeValue(ctx, exception);
    }

    JSValue obj = JS_NewString(ctx, "!!!end!!!");
    result = JS_Call(ctx, resolve_func, JS_UNDEFINED, 1, &obj);
    if (JS_IsException(result)) {
        // 获取异常值
        JSValue exception = JS_GetException(ctx);
        const char* error_str = JS_ToCString(ctx, exception);
        printf("Script execution failed: %s\n", error_str);
        JS_FreeCString(ctx, error_str);
        JS_FreeValue(ctx, exception);
    }


    // 手动触发事件循环，确保异步操作继续执行
    JSContext* ctx_ = nullptr;
    while (JS_ExecutePendingJob(runtime, &ctx_)) {
        // 执行事件循环，直到没有更多的异步任务
    }

    const char* script = "std.printf('Hello, World!');";
    result = JS_Eval(ctx, script, strlen(script), "<input>", JS_EVAL_TYPE_GLOBAL);
    // 判断是否执行成功
    if (JS_IsException(result)) {
        // 获取异常值
        JSValue exception = JS_GetException(ctx);
        const char* error_str = JS_ToCString(ctx, exception);
        printf("Script execution failed: %s\n", error_str);
        JS_FreeCString(ctx, error_str);
        JS_FreeValue(ctx, exception);
    }
    else {
        printf("Script executed successfully!\n");
    }
    }


    //// JavaScript 代码定义生成器函数
    //js_code = R"(
    //    function* myGenerator() {
    //        var a = yield 1;
    //        std.printf('a:' + a);
    //        var b = yield 2;
    //        var c = yield 3;
    //    }
    //)";

    //// 执行 JavaScript 代码
    //global = JS_GetGlobalObject(ctx);
    //JSValue eval_result = JS_Eval(ctx, js_code, strlen(js_code), "<input>", JS_EVAL_TYPE_GLOBAL);
    //if (JS_IsException(eval_result)) {
    //    // 获取异常值
    //    JSValue exception = JS_GetException(ctx);
    //    const char* error_str = JS_ToCString(ctx, exception);
    //    printf("Script execution failed: %s\n", error_str);
    //    JS_FreeCString(ctx, error_str);
    //    JS_FreeValue(ctx, exception);

    //    std::cerr << "Failed to evaluate JavaScript code" << std::endl;
    //    return 1;
    //}

    //// 获取生成器函数
    //JSValue generatorFunc = JS_GetPropertyStr(ctx, global, "myGenerator");
    //if (JS_IsException(generatorFunc)) {
    //    std::cerr << "Failed to get myGenerator function" << std::endl;
    //    return 1;
    //}

    //// 调用生成器函数并传入 C++ 中的值
    //callGenerator(ctx, generatorFunc, 10.5, 20.5, 30.5);

    // 清理资源
    //JS_FreeValue(ctx, eval_result);
    //JS_FreeValue(ctx, generatorFunc);
    //JS_FreeContext(ctx);
    //JS_FreeRuntime(runtime);

    return true;
}

} // namespace pysvr
} // namespace million