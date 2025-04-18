#pragma once

#include <variant>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <functional>

#include <million/exception.h>
#include <million/proto_msg.h>

namespace million {

class MILLION_API ProtoFieldAny {
public:
    // 先声明成员变量
private:
    std::variant<
        std::monostate, int32_t, uint32_t, int64_t, uint64_t,
        bool, float, double, std::string> data_;

public:
    ProtoFieldAny() = default;
    ~ProtoFieldAny() = default;

    // 通用模板构造函数
    template <typename T,
              typename = std::enable_if_t<
                  std::is_constructible_v<
                      decltype(data_), 
                      std::decay_t<T>>
              >>
    ProtoFieldAny(T&& value) 
        : data_(std::forward<T>(value)) {}
    
    ProtoFieldAny(const ProtoFieldAny&) = default;
    ProtoFieldAny(ProtoFieldAny&&) noexcept = default;

    void operator=(const ProtoFieldAny& r) noexcept {
        data_ = r.data_;
    }

    void operator=(ProtoFieldAny&& r) noexcept {
        data_ = std::move(r.data_);
    }


    // 访问器方法
    template <typename T>
    bool is() const {
        return std::holds_alternative<T>(data_);
    }

    template <typename T>
    const T& get() const {
        return std::get<T>(data_);
    }

    template <typename T>
    T& get() {
        return std::get<T>(data_);
    }

    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        return std::get<T>(data_.emplace<T>(std::forward<Args>(args)...));
    }

    bool operator==(const ProtoFieldAny& other) const {
        return data_ == other.data_;
    }

    bool operator!=(const ProtoFieldAny& other) const {
        return !(*this == other);
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        return std::visit(std::forward<Visitor>(vis), data_);
    }


    std::string ToString() {
        return visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                TaskAbort("std::monostate Cannot be converted to string.");
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            }
            else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
                return std::to_string(arg);
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            }
            });
    }

};

// 哈希函数为variant特化
struct MILLION_API ProtoFieldAnyHash {
    size_t operator()(const ProtoFieldAny& key) const {
        return key.visit([](const auto& k) {
            using T = std::decay_t<decltype(k)>;
            return std::hash<T>{}(k);
        });
    }
};

// 比较函数为variant特化
struct MILLION_API ProtoFieldAnyEqual {
    bool operator()(const ProtoFieldAny& lhs, const ProtoFieldAny& rhs) const {
        return lhs == rhs;
    }
};


using CompositeProtoFieldAny = std::vector<ProtoFieldAny>;

struct MILLION_API CompositeProtoFieldAnyHash {
    size_t operator()(const CompositeProtoFieldAny& keys) const {
        size_t seed = 0;
        for (const auto& key : keys) {
            key.visit([&seed](const auto& k) {
                using T = std::decay_t<decltype(k)>;
                seed ^= std::hash<T>{}(k) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            });
        }
        return seed;
    }
};

struct MILLION_API CompositeProtoFieldAnyEqual {
    bool operator()(const CompositeProtoFieldAny& lhs, const CompositeProtoFieldAny& rhs) const {
        return lhs == rhs;
    }
};


inline ProtoFieldAny ProtoMsgGetFieldAny(const google::protobuf::Reflection& reflection
    , const ProtoMsg& row
    , const google::protobuf::FieldDescriptor* field_desc)
{
    switch (field_desc->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        return reflection.GetInt32(row, field_desc);
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        return reflection.GetInt64(row, field_desc);
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        return reflection.GetUInt32(row, field_desc);
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        return reflection.GetUInt64(row, field_desc);
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        return reflection.GetDouble(row, field_desc);
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        return reflection.GetFloat(row, field_desc);
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        return reflection.GetBool(row, field_desc);
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        return reflection.GetEnumValue(row, field_desc);
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        return reflection.GetString(row, field_desc);
    default:
        TaskAbort("Unsupported field type : {}", field_desc->cpp_type_name());
    }
}

inline void ProtoMsgSetFieldAny(const google::protobuf::Reflection& reflection
    , ProtoMsg* row
    , const google::protobuf::FieldDescriptor* field_desc
    , const ProtoFieldAny& any)
{
    switch (field_desc->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        return reflection.SetInt32(row, field_desc, any.get<int32_t>());
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        return reflection.SetInt64(row, field_desc, any.get<int64_t>());
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        return reflection.SetUInt32(row, field_desc, any.get<uint32_t>());
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        return reflection.SetUInt64(row, field_desc, any.get<uint64_t>());
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        return reflection.SetDouble(row, field_desc, any.get<double>());
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        return reflection.SetFloat(row, field_desc, any.get<float>());
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        return reflection.SetBool(row, field_desc, any.get<bool>());
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        return reflection.SetEnumValue(row, field_desc, any.get<int>());
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        return reflection.SetString(row, field_desc, any.get<std::string>());
    default:
        TaskAbort("Unsupported field type : {}", field_desc->cpp_type_name());
    }
}

} // namespace million