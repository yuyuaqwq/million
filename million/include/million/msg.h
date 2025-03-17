#pragma once

#include <variant>

#include <million/api.h>
#include <million/proto_msg.h>
#include <million/cpp_msg.h>

namespace million {

using MsgTypeKey = uintptr_t;

template <typename MsgT>
inline MsgTypeKey GetMsgTypeKey() {
    if constexpr (is_proto_msg_v<MsgT>) {
        return reinterpret_cast<MsgTypeKey>(MsgT::GetDescriptor());
    }
    else if constexpr (is_cpp_msg_v<MsgT>) {
        return reinterpret_cast<MsgTypeKey>(&MsgT::type_static());
    }
    else {
        static_assert(is_proto_msg_v<MsgT> || is_cpp_msg_v<MsgT>,
            "Unsupported message type.");
    }
}

MILLION_MSG_DEFINE_EMPTY(, SB)

class MILLION_API MsgPtr {
public:
    MsgPtr() = default;

    MsgPtr(std::nullptr_t) {}

    MsgPtr(MsgPtr&& other) noexcept 
        : msg_ptr_(std::move(other.msg_ptr_)){}

    void operator=(MsgPtr&& other) noexcept {
        msg_ptr_ = std::move(other.msg_ptr_);
    }

    template <typename T>
        requires is_proto_msg_v<T>
    MsgPtr(std::unique_ptr<T>&& other) noexcept
        : msg_ptr_(ProtoMsgUnique(std::move(other))) {}

    template <typename T>
        requires is_proto_msg_v<T>
    void operator=(std::unique_ptr<T>&& other) noexcept {
        msg_ptr_ = ProtoMsgUnique(std::move(other));
    }

    template <typename T>
        requires is_cpp_msg_v<T>
    MsgPtr(std::unique_ptr<T>&& other) noexcept
        : msg_ptr_(CppMsgUnique(std::move(other))) {}

    template <typename T>
        requires is_cpp_msg_v<T>
    void operator=(std::unique_ptr<T>&& other) noexcept {
        msg_ptr_ = CppMsgUnique(std::move(other));
    }

    template <typename T>
        requires is_proto_msg_v<T> || is_cpp_msg_v<T>
    MsgPtr(std::shared_ptr<const T>&& other) noexcept
        : msg_ptr_(std::move(other)){}

    template <typename T>
        requires is_proto_msg_v<T> || is_cpp_msg_v<T>
    void operator=(std::shared_ptr<const T>&& other) noexcept {
        msg_ptr_ = std::move(other);
    }

    template <typename T>
        requires is_proto_msg_v<T> || is_cpp_msg_v<T>
    MsgPtr(const std::shared_ptr<const T>& v) noexcept
        : msg_ptr_(v) {}

    template <typename T>
        requires is_proto_msg_v<T> || is_cpp_msg_v<T>
    void operator=(const std::shared_ptr<const T>& v) noexcept {
        msg_ptr_ = v;
    }


    bool IsProtoMsg() const {
        return IsProtoMsgUnique() || IsProtoMsgShared();
    }

    bool IsCppMsg() const {
        return IsCppMsgUnique() || IsCppMsgShared();
    }


    MsgTypeKey GetTypeKey() const {
        if (IsProtoMsg()) {
            return reinterpret_cast<MsgTypeKey>(GetProtoMsg()->GetDescriptor());
        }
        else if (IsCppMsg()) {
            return reinterpret_cast<MsgTypeKey>(&GetCppMsg()->type());
        }
        return 0;
    }

    template <typename MsgT>
    bool IsType() const {
        return GetTypeKey() == GetMsgTypeKey<MsgT>();
    }


    template<typename MsgT>
    const MsgT* GetMsg() const {
        assert(IsType<MsgT>());
        if constexpr (is_proto_msg_v<MsgT>) {
            return static_cast<const MsgT*>(GetProtoMsg());
        }
        else if constexpr (is_cpp_msg_v<MsgT>) {
            return static_cast<const MsgT*>(GetCppMsg());
        }
        else {
            static_assert(is_proto_msg_v<MsgT> || is_cpp_msg_v<MsgT>,
                "Unsupported message type.");
        }
    }

    template<typename MsgT>
    MsgT* GetMutMsg() const {
        assert(IsType<MsgT>());
        if constexpr (is_proto_msg_v<MsgT>) {
            return static_cast<MsgT*>(GetMutProtoMsg());
        }
        else if constexpr (is_cpp_msg_v<MsgT>) {
            return static_cast<MsgT*>(GetMutCppMsg());
        }
        else {
            static_assert(is_proto_msg_v<MsgT> || is_cpp_msg_v<MsgT>,
                "Unsupported message type.");
        }
    }

    const ProtoMsg* GetProtoMsg() const {
        if (IsProtoMsgUnique()) {
            return GetProtoMsgUnique().get();
        }
        else if (IsProtoMsgShared()) {
            return GetProtoMsgShared().get();
        }
        else {
            throw std::bad_variant_access();
        }
    }

    const CppMsg* GetCppMsg() const {
        if (IsCppMsgUnique()) {
            return GetCppMsgUnique().get();
        }
        else if (IsCppMsgShared()) {
            return GetCppMsgShared().get();
        }
        else {
            throw std::bad_variant_access();
        }
    }

    ProtoMsg* GetMutProtoMsg() const {
        return GetProtoMsgUnique().get();
    }

    CppMsg* GetMutCppMsg() const {
        return GetCppMsgUnique().get();
    }


    void* Release() {
        if (IsProtoMsgUnique()) {
            return GetProtoMsgUnique().release();
        }
        else if (IsCppMsgUnique()) {
            return GetCppMsgUnique().release();
        }
        throw std::bad_variant_access();
    }

    MsgPtr Copy() const {
        if (IsProtoMsgUnique()) {
            auto proto_msg = GetProtoMsg();
            auto new_msg = proto_msg->New();
            if (!new_msg) throw std::bad_alloc();
            new_msg->CopyFrom(*proto_msg);
            return MsgPtr(ProtoMsgUnique(new_msg));
        }
        else if (IsProtoMsgShared()) {
            return MsgPtr(GetProtoMsgShared());
        }
        else if (IsCppMsgUnique()) {
            auto cpp_msg = GetCppMsg();
            auto new_msg = cpp_msg->Copy();
            if (!new_msg) throw std::bad_alloc();
            return MsgPtr(CppMsgUnique(new_msg));
        }
        else if (IsCppMsgShared()) {
            return MsgPtr(GetCppMsgShared());
        }
        throw std::bad_variant_access();
    }


    operator bool() const {
        if (IsProtoMsg()) {
            return GetProtoMsg();
        }
        else if (IsCppMsg()) {
            return GetCppMsg();
        }
        throw std::bad_variant_access();
    }

private:
    bool IsProtoMsgUnique() const {
        return std::holds_alternative<ProtoMsgUnique>(msg_ptr_);
    }

    bool IsProtoMsgShared() const {
        return std::holds_alternative<ProtoMsgShared>(msg_ptr_);
    }

    bool IsCppMsgUnique() const {
        return std::holds_alternative<CppMsgUnique>(msg_ptr_);
    }

    bool IsCppMsgShared() const {
        return std::holds_alternative<CppMsgShared>(msg_ptr_);
    }

    const ProtoMsgUnique& GetProtoMsgUnique() const {
        return std::get<ProtoMsgUnique>(msg_ptr_);
    }

    const CppMsgUnique& GetCppMsgUnique() const {
        return std::get<CppMsgUnique>(msg_ptr_);
    }

    const ProtoMsgShared& GetProtoMsgShared() const {
        return std::get<ProtoMsgShared>(msg_ptr_);
    }

    const CppMsgShared& GetCppMsgShared() const {
        return std::get<CppMsgShared>(msg_ptr_);
    }

    ProtoMsgUnique& GetProtoMsgUnique() {
        return std::get<ProtoMsgUnique>(msg_ptr_);
    }

    CppMsgUnique& GetCppMsgUnique() {
        return std::get<CppMsgUnique>(msg_ptr_);
    }

    ProtoMsgShared& GetProtoMsgShared() {
        return std::get<ProtoMsgShared>(msg_ptr_);
    }

    CppMsgShared& GetCppMsgShared() {
        return std::get<CppMsgShared>(msg_ptr_);
    }

private:
    std::variant<ProtoMsgUnique
        , ProtoMsgShared
        , CppMsgUnique
        , CppMsgShared> msg_ptr_;
};

template <typename MsgT, typename... Args>
inline MsgPtr make_msg(Args&&... args) {
    if constexpr (is_proto_msg_v<MsgT>) {
        return MsgPtr(make_proto_msg<MsgT>(std::forward<Args>(args)...));
    }
    else if constexpr (is_cpp_msg_v<MsgT>) {
        return MsgPtr(make_cpp_msg<MsgT>(std::forward<Args>(args)...));
    }
    else {
        static_assert(is_proto_msg_v<MsgT> || is_cpp_msg_v<MsgT>,
            "Unsupported message type.");
    }
}

}// namespace million