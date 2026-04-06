#include "db_module.h"

#include <million/jssvr/jssvr.h>
#include <million/db/ss_db.pb.h>

#include "../js_service.h"
#include "../js_util.h"

namespace million {
namespace jssvr {

DBModuleObject::DBModuleObject(mjs::Runtime* rt) :
    CppModuleObject(rt)
{
    AddExportMethod(rt, "load", Load);
}

mjs::Value DBModuleObject::Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSService(context);

    if (par_count < 1) {
        return mjs::Error::Throw(context, "DB Load requires 1 parameter.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "DB Load parameter 1 must be a string for message type.");
    }
    if (!stack.get(1).IsString()) {
        return mjs::Error::Throw(context, "DB Load parameter 2 must be a string for key field name.");
    }

    auto msg_name = stack.get(0).string_view();

    const auto* desc = service.imillion().proto_mgr().FindMessageTypeByName(msg_name);
    if (!desc) {
        return mjs::Error::Throw(context, "DB Load parameter 1 invalid message type.");
    }

    auto key_field_name = stack.get(1).string_view();
    const auto* field = desc->FindFieldByName(key_field_name);
    if (!field) {
        return mjs::Error::Throw(context, "DB Load parameter 2 invalid field name.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "DB Load parameter must be a string for file name.");
    }

    auto key = stack.get(2).ToString(context);
    if (key.IsException()) {
        return mjs::Error::Throw(context, "DB Load parameter 3 must be a string for key value.");
    }

    // 这里让OnMsg等待，发现是C++消息再做分发

    service.function_call_context().sender = service.js_runtime_service().db_service_handle();

    service.function_call_context().waiting_session_id = service.Send<db::DBRowLoadReq>(service.function_call_context().sender, *desc
        , field->number(), std::string(key.string_view()));

    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));
    return service.function_call_context().promise;
}

DBRowClassDef::DBRowClassDef(mjs::Runtime* runtime)
    : ClassDef(runtime, static_cast<mjs::ClassId>(CustomClassId::kDBRowObject), "DBRow") {

    auto const_index = runtime->global_const_pool().insert(mjs::Value("commit"));

    prototype_.object().SetProperty(runtime, const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const  mjs::StackFrame& stack) -> mjs::Value {
        auto& service = GetJSService(context);

        service.function_call_context().sender = service.js_runtime_service().db_service_handle();

        auto& db_row_object = stack.this_val().object<DBRowObject>();

        auto task = db_row_object.db_row().Commit(&service, service.function_call_context().sender);
        service.function_call_context().waiting_session_id = task.coroutine.promise().session_awaiter()->waiting_session_id();

        // Start async commit operation
        service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));

        return service.function_call_context().promise;
    }));
}


DBRowObject::DBRowObject(mjs::Context* context, db::DBRow&& db_row)
    : Object(context, static_cast<mjs::ClassId>(CustomClassId::kDBRowObject))
    , db_row_(std::move(db_row)) {}


void DBRowObject::SetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value&& value) {
    auto key_str = context->GetConstValue(key).string_view();

    const auto& desc = db_row_.GetDescriptor();
    const auto& reflection = db_row_.GetReflection();

    // Find the field by name
    const auto* field = desc.FindFieldByName(key_str);
    if (!field) {
        return; // Field not found, ignore silently like JavaScript objects
    }

    // Mark field as dirty when setting
    db_row_.MarkDirtyByFieldIndex(field->index());

    // Convert JavaScript value to protobuf field
    auto& proto_msg = db_row_.get();
    JSUtil::SetProtoMessageFieldFromJSValue(context, &proto_msg, desc, reflection, *field, value);
}

bool DBRowObject::GetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value* value) {
    auto key_str = context->GetConstValue(key).string_view();

    const auto& desc = db_row_.GetDescriptor();
    const auto& reflection = db_row_.GetReflection();

    // Find the field by name
    const auto* field = desc.FindFieldByName(key_str);
    if (!field) {
        return Object::GetProperty(context, key, value); // Field not found
    }

    // Convert protobuf field to JavaScript value
    const auto& proto_msg = db_row_.get();
    *value = JSUtil::GetJSValueByProtoMessageField(context, proto_msg, reflection, *field);
    return true;
}

} // namespace jssvr
} // namespace million
