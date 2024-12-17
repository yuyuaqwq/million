#include "iostream"

#include <google/protobuf/message.h>
#include <string>

#include <million/imillion.h>
#include <million/msg.h>

#include <pysvr/api.h>

#include <quickjs/quickjs.h>
#include <quickjs/quickjs-libc.h>


//bool set_field(google::protobuf::Message* message, const std::string& field_name, const py::object& value) {
//    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
//    const google::protobuf::Reflection* reflection = message->GetReflection();
//
//    const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName(field_name);
//    if (!field) {
//        py::print("Field", field_name, "not found in message type", descriptor->name());
//        return false;  // 字段未找到
//    }
//
//    // 根据字段类型设置相应的值
//    if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
//        reflection->SetString(message, field, value.cast<std::string>());
//    }
//    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT32) {
//        reflection->SetInt32(message, field, value.cast<int>());
//    }
//    // 可以继续扩展更多类型，例如 BOOL、FLOAT、DOUBLE 等
//
//    return true;
//}
//
//void send(const std::string& message_type, pybind11::kwargs kwargs) {
//    std::unique_ptr<google::protobuf::Message> message;
//
//    // 根据消息类型名称创建消息实例
//    //if (message_type == "test") {
//        // message = ;
//    //}
//    //else {
//        // throw std::runtime_error("Unsupported message type: " + message_type);
//    //}
//
//    // 遍历关键字参数并设置消息字段
//    for (auto item : kwargs) {
//        std::string field_name = item.first.cast<std::string>();
//        py::object value = item.second;
//        set_field(message.get(), field_name, value);
//    }
//
//    // 将 Protobuf 对象序列化为字节流
//    std::string serialized_data;
//    message->SerializeToString(&serialized_data);
//
//
//    // return pybind11::bytes(serialized_data);
//}
//
//PYBIND11_EMBEDDED_MODULE(million, m) {
//    m.doc() = "million core module";
//
//    // 绑定 send 函数
//    m.def("send", &send, "Create a protobuf message by type name and kwargs",
//        py::arg("message_type"));
//}



class JsService : public million::IService {
public:
    using Base = IService;
    using Base::Base;

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
        JsService* service = static_cast<JsService*>(opaque);
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
        js_std_eval_binary

        service->Send();
        return JSValue();
    }

    static int ServiceModuleInit(JSContext* ctx, JSModuleDef* m) {
        // 导出函数到模块
        JSCFunctionListEntry list[] = {
            JS_CFUNC_DEF("send", 2, ServiceModuleSend),
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
        if (!JsAddModule("service", module)) {
            logger().Err("JsAddModule failed: {}.", "service");
            // 不释放module，等自动回收
            return nullptr;
        }
        return module;
    }



    virtual bool OnInit() override {
        bool success = false;
        do {
            js_rt_ = JS_NewRuntime();
            if (!js_rt_) {
                logger().Err("JS_NewRuntime failed.");
                break;
            }
            js_std_init_handlers(js_rt_);

            js_ctx_ = JS_NewContext(js_rt_);
            if (!js_ctx_) {
                logger().Err("JS_NewContext failed.");
                break;
            }
            JS_SetContextOpaque(js_ctx_, this);

            js_init_module_std(js_ctx_, "std");
            js_init_module_os(js_ctx_, "os");

            JS_SetModuleLoaderFunc(js_rt_, nullptr, JsModuleLoader, this);

            if (!CreateServiceModule()) {
                logger().Err("CreateServiceModule failed.");
                break;
            }

            const char* test_val = R"(
              import * as std from 'std';
              import * as os from 'os';
              import { send } from 'service';
              send();
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

    virtual void OnStop() override {

    }

    virtual million::Task<> OnMsg(million::ServiceHandle sender, million::SessionId session_id, million::MsgUnique) override {
        
        co_return;
    }


private:
    JSRuntime* js_rt_ = nullptr;
    JSContext* js_ctx_ = nullptr;

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
    imillion->NewService<JsService>();


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
    //const char* str =
    //    "import * as std from 'std';\n"
    //    "import * as os from 'os';\n"
    //    "globalThis.std = std;\n"
    //    "globalThis.os = os;\n"
    //    "var console = {};\n"
    //    "console.log = value => std.printf(value);\n";
    //JSValue init_compile =
    //    JS_Eval(ctx, str, strlen(str), "<main>", JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    // js_module_set_import_meta(ctx, init_compile, 1, 1);
    // JSValue init_run = JS_EvalFunction(ctx, init_compile);



  //  const char* test_val = R"(
  //  var console = {};
  //  console.log = value => globalThis.std.printf(value + "\n");
  //  console.log('ok')
  //  for(var key in globalThis){
  //    console.log("--->" + key)
  //  }
  //  for(var key in std){
  //    console.log("--->" + key)
  //  }
  //  for(var key in os){
  //    console.log("--->" + key)
  //  }
  //  globalThis.std.printf('hello_world\n');
  //  globalThis.std.printf(globalThis + "\n");
  //  var a = false;
  //  os.setTimeout(()=>{std.printf('AAB\n')}, 2000)
  //  globalThis.std.printf(a + "\n");
  //)";
  //  result = JS_Eval(ctx, test_val, strlen(test_val), "test", JS_EVAL_TYPE_MODULE);
  //  printf("loop\n");
  //  js_std_loop(ctx);


    // 创建 JavaScript 引擎的执行环境
    // JSValue global = JS_GetGlobalObject(ctx);

    // 在全局对象上注册 Call 函数
    // JS_SetPropertyStr(ctx, global, "Call", JS_NewCFunction(ctx, Call, "Call", 0));

    // JavaScript 代码（函数 A）
    const char* js_code =
        "async function asyncFunction(obj) {"
        "    std.printf('!!!start!!!');"
        "    let result = await Call(obj);"  // 暴露全局的 resolve 函数
        "    std.printf('!!!end!!!', result);"
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

    // 获取 promise 中的 resolve 函数，恢复异步操作
    //JSValue resolve_func = JS_GetPropertyStr(ctx, obj, "resolve");
    //if (JS_IsException(resolve_func)) {
    //    // 获取异常值
    //    JSValue exception = JS_GetException(ctx);
    //    const char* error_str = JS_ToCString(ctx, exception);
    //    printf("Script execution failed: %s\n", error_str);
    //    JS_FreeCString(ctx, error_str);
    //    JS_FreeValue(ctx, exception);
    //}


    result = JS_Call(ctx, resolve_func, JS_UNDEFINED, 0, NULL);
    //wake_up_async(ctx, promise, JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), "resolvePromise"));
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