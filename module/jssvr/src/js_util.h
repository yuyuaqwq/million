#pragma once

#include <mjs/context.h>
#include <mjs/object_impl/array_object.h>

#include <million/imillion.h>

namespace million {
namespace jssvr {

class JSUtil {
public:
    // 获取JS值
    template <typename JSContext>
    static mjs::Value GetJSValueByProtoMessageField(JSContext* context, const million::ProtoMessage& msg
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
            return mjs::Value(mjs::String::New(string_ref));
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
            return ProtoMessageToJSObject(context, sub_msg);
        }
        default:
            return mjs::Value();
        }
    }

    // 获取重复字段的JS值
    template <typename JSContext>
    static mjs::Value GetJSValueByProtoMessgaeRepeatedField(JSContext* context, const million::ProtoMessage& msg
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
            return mjs::Value(mjs::String::New(string_ref));
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
            return ProtoMessageToJSObject(context, sub_msg);
        }
        default:
            return mjs::Value();
        }
    }

    // 转换单个字段
    static void ProtoMessageToJSObjectOne(mjs::Context* context, const million::ProtoMessage& msg, mjs::Object* obj, const google::protobuf::FieldDescriptor& field_desc) {
        const auto desc = msg.GetDescriptor();
        const auto refl = msg.GetReflection();

        if (field_desc.is_repeated()) {
            auto js_array = mjs::ArrayObject::New(context, {});
            for (size_t j = 0; j < refl->FieldSize(msg, &field_desc); ++j) {
                auto js_value = GetJSValueByProtoMessgaeRepeatedField(context, msg, *refl, field_desc, j);
                js_array->Push(context, std::move(js_value));
            }
            obj->SetComputedProperty(context, mjs::Value(field_desc.name().data()), mjs::Value(js_array));
        }
        else {
            auto js_value = GetJSValueByProtoMessageField(context, msg, *refl, field_desc);
            obj->SetComputedProperty(context, mjs::Value(field_desc.name().data()), std::move(js_value));
        }
    }

    static void ProtoMessageToJSObjectOne(mjs::Runtime* runtime, const million::ProtoMessage& msg, mjs::Object* obj, const google::protobuf::FieldDescriptor& field_desc) {
        const auto desc = msg.GetDescriptor();
        const auto refl = msg.GetReflection();

        if (field_desc.is_repeated()) {
            auto js_array = mjs::ArrayObject::New(runtime, {});
            for (size_t j = 0; j < refl->FieldSize(msg, &field_desc); ++j) {
                auto js_value = GetJSValueByProtoMessgaeRepeatedField(runtime, msg, *refl, field_desc, j);
                js_array->Push(runtime, std::move(js_value));
            }
            obj->SetProperty(runtime, field_desc.name().data(), mjs::Value(js_array));
        }
        else {
            auto js_value = GetJSValueByProtoMessageField(runtime, msg, *refl, field_desc);
            obj->SetProperty(runtime, field_desc.name().data(), std::move(js_value));
        }
    }


    // Protobuf消息转JS对象
    template <typename JSContext>
    static mjs::Value ProtoMessageToJSObject(JSContext* context, const million::ProtoMessage& msg) {
        auto obj = mjs::Object::New(context);

        const auto desc = msg.GetDescriptor();
        const auto refl = msg.GetReflection();

        for (size_t i = 0; i < desc->field_count(); ++i) {
            const auto field_desc = desc->field(i);
            ProtoMessageToJSObjectOne(context, msg, obj, *field_desc);
        }

        return mjs::Value(obj);
    }

    // 设置重复字段
    template <typename JSContext>
    static void SetProtoMessageRepeatedFieldFromJSValue(JSContext* context, million::ProtoMessage* msg
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
            JSObjectToProtoMessage(context, sub_msg, repeated_value);
            break;
        }
        default:
            TaskAbort("Field {}.{}[{}] cannot convert type.", desc.name(), field_desc.name(), j);
            break;
        }
    }

    // 设置字段
    template <typename JSContext>
    static void SetProtoMessageFieldFromJSValue(JSContext* context, million::ProtoMessage* msg
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
            reflection.SetInt64(msg, &field_desc, js_value.ToInt64().i64());
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
            reflection.SetInt32(msg, &field_desc, js_value.ToInt64().i64());
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
            if (!js_value.IsNumber()) {
                TaskAbort("Field {}.{} is not a number.", desc.name(), field_desc.name());
            }
            reflection.SetUInt32(msg, &field_desc, js_value.ToUInt64().u64());
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
            JSObjectToProtoMessage(context, sub_msg, js_value);
            break;
        }
        default:
            TaskAbort("Field {}.{} cannot convert type.", desc.name(), field_desc.name());
            break;
        }
    }

    // 转换单个字段
    template <typename JSContext>
    static void JSObjectToProtoMessageOne(JSContext* context, million::ProtoMessage* msg, const mjs::Value& field_value, const google::protobuf::FieldDescriptor& field_desc) {
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
                SetProtoMessageRepeatedFieldFromJSValue(context, msg, *desc, *refl, field_desc, repeated_value, j);
            }
        }
        else {
            SetProtoMessageFieldFromJSValue(context, msg, *desc, *refl, field_desc, field_value);
        }
    }

    // JS对象转Protobuf消息
    static void JSObjectToProtoMessage(mjs::Context* context, million::ProtoMessage* msg, const mjs::Value& obj_val) {
        const auto desc = msg->GetDescriptor();
        const auto refl = msg->GetReflection();

        auto& obj = obj_val.object();

        for (size_t i = 0; i < desc->field_count(); ++i) {
            const auto field_desc = desc->field(i);
            mjs::Value field_value;
            if (obj.GetComputedProperty(context, mjs::Value(field_desc->name().c_str()), &field_value)) {
                JSObjectToProtoMessageOne(context, msg, field_value, *field_desc);
            }
        }
    }

    static void JSObjectToProtoMessage(mjs::Runtime* runtime, million::ProtoMessage* msg, const mjs::Value& obj_val) {
        const auto desc = msg->GetDescriptor();
        const auto refl = msg->GetReflection();

        auto& obj = obj_val.object();

        for (size_t i = 0; i < desc->field_count(); ++i) {
            const auto field_desc = desc->field(i);
            mjs::Value field_value;
            if (obj.GetProperty(runtime, field_desc->name().c_str(), &field_value)) {
                JSObjectToProtoMessageOne(runtime, msg, field_value, *field_desc);
            }
        }
    }
};

} // namespace jssvr
} // namespace million