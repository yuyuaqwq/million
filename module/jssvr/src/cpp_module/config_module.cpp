#include "config_module.h"

#include <million/jssvr/jssvr.h>

#include "../js_service.h"
#include "config_service.h"

namespace million {
namespace jssvr {

// ConfigModuleObject implementation
ConfigModuleObject::ConfigModuleObject(mjs::Runtime* rt) :
    CppModuleObject(rt)
{
    AddExportMethod(rt, "load", Load);
}

mjs::Value ConfigModuleObject::Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSService(context);

    if (par_count < 1) {
        return mjs::Error::Throw(context, "JSConfig Load requires 1 parameter.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "JSConfig Load parameter 1 must be a string for message type.");
    }

    auto msg_name = stack.get(0).string_view();
    const auto* desc = service.imillion().proto_mgr().FindMessageTypeByName(msg_name);
    if (!desc) {
        return mjs::Error::Throw(context, "JSConfig Load parameter 1 invalid message type.");
    }

    // 设置异步操作上下文
    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));

    // 从JSRuntimeService获取JSConfigService句柄
    auto& js_runtime_service = service.js_runtime_service();
    service.function_call_context().sender = js_runtime_service.js_config_service_handle();

    // 发送配置查询请求
    service.function_call_context().waiting_session_id = service.Send<JSConfigQueryReq>(
        service.function_call_context().sender, *desc);

    return service.function_call_context().promise;
}


// JSConfigTableClassDef implementation
ConfigTableClassDef::ConfigTableClassDef(mjs::Runtime* runtime)
    : ClassDef(runtime, static_cast<mjs::ClassId>(CustomClassId::kConfigTableObject), "ConfigTable") {

    auto getRowByIndex_const_index = runtime->global_const_pool().insert(mjs::Value("getRowByIndex"));
    auto findRow_const_index = runtime->global_const_pool().insert(mjs::Value("findRow"));
    auto getRowCount_const_index = runtime->global_const_pool().insert(mjs::Value("getRowCount"));

    prototype_.object().SetProperty(runtime, getRowByIndex_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        if (par_count < 1 || !stack.get(0).IsNumber()) {
            return mjs::Error::Throw(context, "getRowByIndex requires a number parameter");
        }

        auto& config_table_object = stack.this_val().object<ConfigTableObject>();
        auto index = static_cast<size_t>(stack.get(0).ToInt64().i64());

        if (index >= config_table_object.cached_rows().size()) {
            return mjs::Value(); // Return undefined for out of bounds
        }

        return config_table_object.cached_rows()[index];
    }));

    prototype_.object().SetProperty(runtime, findRow_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        if (par_count < 1 || !stack.get(0).IsFunctionObject() && !stack.get(0).IsFunctionDef()) {
            return mjs::Error::Throw(context, "findRow requires a function parameter");
        }

        auto& config_table_object = stack.this_val().object<ConfigTableObject>();
        auto predicate_func = stack.get(0);

        // 遍历缓存的JS行对象
        for (const auto& cached_row : config_table_object.cached_rows()) {
            // 调用谓词函数
            std::initializer_list<mjs::Value> args = { cached_row };
            auto result = context->CallFunction(&predicate_func, mjs::Value(), args.begin(), args.end());

            // 检查谓词是否返回true
            if (result.IsBoolean() && result.boolean()) {
                return cached_row;
            }
        }

        return mjs::Value(); // Return undefined if not found
    }));

    prototype_.object().SetProperty(runtime, getRowCount_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        auto& config_table_object = stack.this_val().object<ConfigTableObject>();
        return mjs::Value(config_table_object.cached_rows().size());
    }));
}


// JSConfigTableObject implementation
ConfigTableObject::ConfigTableObject(mjs::Runtime* runtime, const google::protobuf::Descriptor* descriptor, std::vector<mjs::Value>&& cached_rows)
    : Object(runtime, static_cast<mjs::ClassId>(CustomClassId::kConfigTableObject))
    , descriptor_(descriptor)
    , cached_rows_(std::move(cached_rows)) {
}



// ConfigTableWeakClassDef implementation
ConfigTableWeakClassDef::ConfigTableWeakClassDef(mjs::Runtime* runtime)
    : ClassDef(runtime, static_cast<mjs::ClassId>(CustomClassId::kConfigTableWeakObject), "ConfigTableWeak") {

    auto lock_const_index = runtime->global_const_pool().insert(mjs::Value("lock"));

    prototype_.object().SetProperty(runtime, lock_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        auto& service = GetJSService(context);
        auto& config_table_weak_object = stack.this_val().object<ConfigTableWeakObject>();

        // Get config service handle
        auto config_service_handle = service.js_runtime_service().config_service_handle();

        auto& weak_obj = stack.this_val().object<ConfigTableWeakObject>();

        // 这里未来需要修改C++消息类型，区分初次加载和加锁重载
        auto obj = weak_obj.Lock(context);
        return obj;
    }));
}

// ConfigTableWeakObject implementation
ConfigTableWeakObject::ConfigTableWeakObject(mjs::Context* context, mjs::Value&& table_object)
    : Object(context, static_cast<mjs::ClassId>(CustomClassId::kConfigTableWeakObject))
    , table_object_(std::move(table_object)) {}

mjs::Value ConfigTableWeakObject::Lock(mjs::Context* context) {
    auto& obj = table_object_.object<ConfigTableObject>();
    if (!obj.is_expired()) {
        return table_object_;
    }
    auto& service = GetJSService(context);

    // 重新加载
    service.function_call_context().sender = service.js_runtime_service().js_config_service_handle();

    service.function_call_context().waiting_session_id = service.Send<JSConfigQueryReq>(
        service.function_call_context().sender, *obj.descriptor());

    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));
    return service.function_call_context().promise;
}

} // namespace jssvr
} // namespace million
