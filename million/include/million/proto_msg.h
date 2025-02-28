#pragma once

#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <format>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/importer.h>

namespace million {

namespace protobuf = google::protobuf;

using ProtoMsg = protobuf::Message;
using ProtoMsgUnique = std::unique_ptr<ProtoMsg>;
using ProtoMsgShared = std::shared_ptr<const ProtoMsg>;
using ProtoMsgWeak = std::weak_ptr<const ProtoMsg>;

inline const google::protobuf::FieldDescriptor& GetFieldDescriptor(const google::protobuf::Descriptor& desc, int index) {
    const auto* field_desc = desc.field(index);
    if (!field_desc) {
        throw std::runtime_error(std::format("field{} is nullptr.", index));
    }
    return *field_desc;
}

// 函数重载：为每个字段类型实现专门的 SetValue 方法
inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, double value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_DOUBLE) {
        throw std::runtime_error("Type mismatch: expected TYPE_DOUBLE");
    }
    reflection.SetDouble(msg, &field_desc, value);
}

inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, float value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_FLOAT) {
        throw std::runtime_error("Type mismatch: expected TYPE_FLOAT");
    }
    reflection.SetFloat(msg, &field_desc, value);
}

inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, int64_t value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_INT64 &&
        field_desc.type() != google::protobuf::FieldDescriptor::TYPE_SINT64 &&
        field_desc.type() != google::protobuf::FieldDescriptor::TYPE_SFIXED64) {
        throw std::runtime_error("Type mismatch: expected TYPE_INT64, TYPE_SINT64, or TYPE_SFIXED64");
    }
    reflection.SetInt64(msg, &field_desc, value);
}

inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, uint64_t value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_UINT64 &&
        field_desc.type() != google::protobuf::FieldDescriptor::TYPE_FIXED64) {
        throw std::runtime_error("Type mismatch: expected TYPE_UINT64 or TYPE_FIXED64");
    }
    reflection.SetUInt64(msg, &field_desc, value);
}

inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, int32_t value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_INT32 &&
        field_desc.type() != google::protobuf::FieldDescriptor::TYPE_SINT32 &&
        field_desc.type() != google::protobuf::FieldDescriptor::TYPE_SFIXED32) {
        throw std::runtime_error("Type mismatch: expected TYPE_INT32, TYPE_SINT32, or TYPE_SFIXED32");
    }
    reflection.SetInt32(msg, &field_desc, value);
}

inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, uint32_t value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_UINT32 &&
        field_desc.type() != google::protobuf::FieldDescriptor::TYPE_FIXED32) {
        throw std::runtime_error("Type mismatch: expected TYPE_UINT32 or TYPE_FIXED32");
    }
    reflection.SetUInt32(msg, &field_desc, value);
}

inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, bool value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_BOOL) {
        throw std::runtime_error("Type mismatch: expected TYPE_BOOL");
    }
    reflection.SetBool(msg, &field_desc, value);
}

inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, const std::string& value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_STRING &&
        field_desc.type() != google::protobuf::FieldDescriptor::TYPE_BYTES) {
        throw std::runtime_error("Type mismatch: expected TYPE_STRING or TYPE_BYTES");
    }
    reflection.SetString(msg, &field_desc, value);
}

inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, const char* value) {
    const auto& field_desc = GetFieldDescriptor(desc, index);
    if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_STRING) {
        throw std::runtime_error("Type mismatch: expected TYPE_STRING");
    }
    reflection.SetString(msg, &field_desc, value);
}

template <typename T>
inline void SetValue(ProtoMsg* msg, const google::protobuf::Descriptor& desc, const google::protobuf::Reflection& reflection, int index, T&& value) {
    if constexpr (std::is_enum_v<std::remove_reference_t<T>>) {
        const auto& field_desc = GetFieldDescriptor(desc, index);
        if (field_desc.type() != google::protobuf::FieldDescriptor::TYPE_ENUM) {
            throw std::runtime_error("Type mismatch: expected type_enum");
        }
        const google::protobuf::EnumValueDescriptor* enum_value =
            field_desc.enum_type()->FindValueByNumber(value);
        reflection.SetEnum(msg, &field_desc, enum_value);
    }
    else {
        //static_assert(sizeof(T) == 0, "Type mismatch: no matching SetValue overload found.");
        std::cerr << "No matching SetValue overload found for type: " << typeid(T).name() << std::endl;
        throw std::runtime_error(std::format("Type mismatch: no matching SetValue overload found: {}.", typeid(T).name()));
    }
}

// 主要的 make_msg 实现，用于递归设置每个字段的值
template <typename MsgT, typename... Args>
inline std::unique_ptr<MsgT> make_proto_msg(Args&&... args) {
    auto msg = std::make_unique<MsgT>();
    const auto* desc = msg->GetDescriptor();
    if (!desc) {
        throw std::runtime_error("GetDescriptor is nullptr.");
    }
    const auto* reflection = msg->GetReflection();
    if (!reflection) {
        throw std::runtime_error("GetReflection is nullptr.");
    }

    // 填充字段的辅助函数
    size_t index = 0;
    (SetValue(msg.get(), *desc, *reflection, index++, std::forward<decltype(args)>(args)),  ...);
    return msg;
}

template <class MsgT>
inline constexpr bool is_proto_msg_v = std::is_base_of_v<ProtoMsg, MsgT>;

} // namespace million