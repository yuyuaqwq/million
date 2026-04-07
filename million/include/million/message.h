#pragma once

#include <variant>
#include <typeindex>

#include <million/api.h>
#include <million/proto_message.h>
#include <million/cpp_message.h>
#include <million/proto_field_any.h>

namespace million {

using MessageTypeKey = uintptr_t;

template <typename MessageT>
inline MessageTypeKey GetMessageTypeKey() {
    if constexpr (is_proto_message_v<MessageT>) {
        return reinterpret_cast<MessageTypeKey>(MessageT::GetDescriptor());
    }
    else if constexpr (is_cpp_message_v<MessageT>) {
        return reinterpret_cast<MessageTypeKey>(&MessageT::type_static());
    }
    else {
        static_assert(is_proto_message_v<MessageT> || is_cpp_message_v<MessageT>,
            "Unsupported message type.");
    }
}

class MILLION_API MessagePointer {
public:
    MessagePointer() = default;

    MessagePointer(std::nullptr_t) 
        : message_ptr_(nullptr) {}

    MessagePointer(MessagePointer&& other) noexcept 
        : message_ptr_(std::move(other.message_ptr_)){}

    void operator=(MessagePointer&& other) noexcept {
        message_ptr_ = std::move(other.message_ptr_);
    }

    MessagePointer(const MessagePointer& other) = delete;

    void operator=(const MessagePointer& other) = delete;

    template <typename T>
        requires is_proto_message_v<T>
    MessagePointer(std::unique_ptr<T>&& other) noexcept
        : message_ptr_(ProtoMessageUnique(std::move(other))) {}

    template <typename T>
        requires is_proto_message_v<T>
    void operator=(std::unique_ptr<T>&& other) noexcept {
        message_ptr_ = ProtoMessageUnique(std::move(other));
    }

    template <typename T>
        requires is_cpp_message_v<T>
    MessagePointer(std::unique_ptr<T>&& other) noexcept
        : message_ptr_(CppMessageUnique(std::move(other))) {}

    template <typename T>
        requires is_cpp_message_v<T>
    void operator=(std::unique_ptr<T>&& other) noexcept {
        message_ptr_ = CppMessageUnique(std::move(other));
    }

    template <typename T>
        requires is_proto_message_v<T> || is_cpp_message_v<T>
    MessagePointer(std::shared_ptr<const T>&& other) noexcept
        : message_ptr_(std::move(other)){}

    template <typename T>
        requires is_proto_message_v<T> || is_cpp_message_v<T>
    void operator=(std::shared_ptr<const T>&& other) noexcept {
        message_ptr_ = std::move(other);
    }

    template <typename T>
        requires is_proto_message_v<T> || is_cpp_message_v<T>
    MessagePointer(const std::shared_ptr<const T>& v) noexcept
        : message_ptr_(v) {}

    template <typename T>
        requires is_proto_message_v<T> || is_cpp_message_v<T>
    void operator=(const std::shared_ptr<const T>& v) noexcept {
        message_ptr_ = v;
    }


    bool IsProtoMessage() const {
        return IsProtoMessageUnique() || IsProtoMessageShared();
    }

    bool IsCppMessage() const {
        return IsCppMessageUnique() || IsCppMessageShared();
    }


    MessageTypeKey GetTypeKey() const {
        if (IsProtoMessage()) {
            return reinterpret_cast<MessageTypeKey>(GetProtoMessage()->GetDescriptor());
        }
        else if (IsCppMessage()) {
            return reinterpret_cast<MessageTypeKey>(&GetCppMessage()->type());
        }
        return 0;
    }

    template <typename MessageT>
    bool IsType() const {
        return GetTypeKey() == GetMessageTypeKey<MessageT>();
    }


    template<typename MessageT>
    const MessageT* GetMessage() const {
        assert(IsType<MessageT>());
        if constexpr (is_proto_message_v<MessageT>) {
            return static_cast<const MessageT*>(GetProtoMessage());
        }
        else if constexpr (is_cpp_message_v<MessageT>) {
            return static_cast<const MessageT*>(GetCppMessage());
        }
        else {
            static_assert(is_proto_message_v<MessageT> || is_cpp_message_v<MessageT>,
                "Unsupported message type.");
        }
    }

    template<typename MessageT>
    MessageT* GetMutableMessage() const {
        assert(IsType<MessageT>());
        if constexpr (is_proto_message_v<MessageT>) {
            return static_cast<MessageT*>(GetMutableProtoMessage());
        }
        else if constexpr (is_cpp_message_v<MessageT>) {
            return static_cast<MessageT*>(GetMutableCppMessage());
        }
        else {
            static_assert(is_proto_message_v<MessageT> || is_cpp_message_v<MessageT>,
                "Unsupported message type.");
        }
    }

    const ProtoMessage* GetProtoMessage() const {
        if (IsProtoMessageUnique()) {
            return GetProtoMessageUnique().get();
        }
        else if (IsProtoMessageShared()) {
            return GetProtoMessageShared().get();
        }
        else {
            throw std::bad_variant_access();
        }
    }

    const CppMessage* GetCppMessage() const {
        if (IsCppMessageUnique()) {
            return GetCppMessageUnique().get();
        }
        else if (IsCppMessageShared()) {
            return GetCppMessageShared().get();
        }
        else {
            throw std::bad_variant_access();
        }
    }

    ProtoMessage* GetMutableProtoMessage() const {
        return GetProtoMessageUnique().get();
    }

    CppMessage* GetMutableCppMessage() const {
        return GetCppMessageUnique().get();
    }


    void* Release() {
        if (IsProtoMessageUnique()) {
            return GetProtoMessageUnique().release();
        }
        else if (IsCppMessageUnique()) {
            return GetCppMessageUnique().release();
        }
        return nullptr;
    }

    // 如果是UniquePtr，会提升为SharedPtr，再进行复制
    MessagePointer Share() {
        if (IsProtoMessageUnique()) {
            auto cpp_msg = GetProtoMessageUnique().release();
            message_ptr_ = ProtoMessageShared(cpp_msg);
            return GetProtoMessageShared();
        }
        else if (IsCppMessageUnique()) {
            auto cpp_msg = GetCppMessageUnique().release();
            message_ptr_ = CppMessageShared(cpp_msg);
            return GetCppMessageShared();
        }
        else if (IsProtoMessageShared()) {
            return GetProtoMessageShared();
        }
        else if (IsCppMessageShared()) {
            return GetCppMessageShared();
        }
        throw std::bad_variant_access();
    }


    operator bool() const {
        if (IsProtoMessage()) {
            return GetProtoMessage();
        }
        else if (IsCppMessage()) {
            return GetCppMessage();
        }
        else {
            return false;
        }
        throw std::bad_variant_access();
    }

private:
    bool IsProtoMessageUnique() const {
        return std::holds_alternative<ProtoMessageUnique>(message_ptr_);
    }

    bool IsProtoMessageShared() const {
        return std::holds_alternative<ProtoMessageShared>(message_ptr_);
    }

    bool IsCppMessageUnique() const {
        return std::holds_alternative<CppMessageUnique>(message_ptr_);
    }

    bool IsCppMessageShared() const {
        return std::holds_alternative<CppMessageShared>(message_ptr_);
    }

    const ProtoMessageUnique& GetProtoMessageUnique() const {
        return std::get<ProtoMessageUnique>(message_ptr_);
    }

    const CppMessageUnique& GetCppMessageUnique() const {
        return std::get<CppMessageUnique>(message_ptr_);
    }

    const ProtoMessageShared& GetProtoMessageShared() const {
        return std::get<ProtoMessageShared>(message_ptr_);
    }

    const CppMessageShared& GetCppMessageShared() const {
        return std::get<CppMessageShared>(message_ptr_);
    }

    ProtoMessageUnique& GetProtoMessageUnique() {
        return std::get<ProtoMessageUnique>(message_ptr_);
    }

    CppMessageUnique& GetCppMessageUnique() {
        return std::get<CppMessageUnique>(message_ptr_);
    }

    ProtoMessageShared& GetProtoMessageShared() {
        return std::get<ProtoMessageShared>(message_ptr_);
    }

    CppMessageShared& GetCppMessageShared() {
        return std::get<CppMessageShared>(message_ptr_);
    }

private:
    std::variant<
        std::nullptr_t
        , ProtoMessageUnique
        , ProtoMessageShared
        , CppMessageUnique
        , CppMessageShared> message_ptr_;
};

template <typename MessageT, typename... Args>
inline MessagePointer make_message(Args&&... args) {
    if constexpr (is_proto_message_v<MessageT>) {
        return MessagePointer(make_proto_message<MessageT>(std::forward<Args>(args)...));
    }
    else if constexpr (is_cpp_message_v<MessageT>) {
        return MessagePointer(make_cpp_message<MessageT>(std::forward<Args>(args)...));
    }
    else {
        static_assert(is_proto_message_v<MessageT> || is_cpp_message_v<MessageT>,
            "Unsupported message type.");
    }
}

}// namespace million