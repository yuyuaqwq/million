#pragma once

#include <variant>

#include <million/proto_msg.h>
#include <million/cpp_msg.h>

namespace million {

using MsgTypeKey = uintptr_t;

class MsgUnique {
public:
    MsgUnique() = default;

    MsgUnique(std::nullptr_t) {}

    MsgUnique(ProtoMessage* msg)
        : msg_unique_(ProtoMsgUnique(msg)) {}

    MsgUnique(CppMessage* msg)
        : msg_unique_(CppMsgUnique(msg)) {}

    MsgUnique(MsgUnique&&) = default;

    void operator=(MsgUnique&& rv) noexcept {
        msg_unique_ = std::move(rv.msg_unique_);
    }

    template <typename T>
    MsgUnique(std::unique_ptr<T>&& rv) noexcept
        : msg_unique_(std::move(MsgUnique(rv.release()).msg_unique_)) {}

    template <typename T>
    void operator=(std::unique_ptr<T>&& rv) noexcept {
        msg_unique_ = std::move(MsgUnique(rv.release()).msg_unique_);
    }

    bool IsProtoMessage() const {
        return std::holds_alternative<ProtoMsgUnique>(msg_unique_);
    }

    bool IsCppMessage() const {
        return std::holds_alternative<CppMsgUnique>(msg_unique_);
    }

    const ProtoMsgUnique& GetProtoMessage() const {
        return std::get<ProtoMsgUnique>(msg_unique_);
    }

    const CppMsgUnique& GetCppMessage() const {
        return std::get<CppMsgUnique>(msg_unique_);
    }

    ProtoMsgUnique& GetProtoMessage() {
        return std::get<ProtoMsgUnique>(msg_unique_);
    }

    CppMsgUnique& GetCppMessage() {
        return std::get<CppMsgUnique>(msg_unique_);
    }

    MsgTypeKey GetTypeKey() const {
        if (IsProtoMessage()) {
            return reinterpret_cast<MsgTypeKey>(GetProtoMessage()->GetDescriptor());
        }
        else if (IsCppMessage()) {
            return reinterpret_cast<MsgTypeKey>(&GetCppMessage()->type());
        }
        return 0;
    }

    bool IsType(const protobuf::Descriptor* desc) const {
        return GetTypeKey() == reinterpret_cast<MsgTypeKey>(desc);
    }

    bool IsType(const type_info& type_info) const {
        return GetTypeKey() == reinterpret_cast<MsgTypeKey>(&type_info);
    }

    MsgUnique Copy() const {
        if (IsProtoMessage()) {
            auto& proto_msg = GetProtoMessage();
            auto new_msg = proto_msg->New();
            if (!new_msg) throw std::bad_alloc();
            new_msg->CopyFrom(*proto_msg);
            return MsgUnique(new_msg);
        }
        else if (IsCppMessage()) {
            auto& cpp_msg = GetCppMessage();
            auto new_msg = cpp_msg->Copy();
            if (!new_msg) throw std::bad_alloc();
            return MsgUnique(new_msg);
        }
        throw std::bad_variant_access();
    }


    operator bool() const {
        if (IsProtoMessage()) {
            return GetProtoMessage().get();
        }
        else if (IsCppMessage()) {
            return GetCppMessage().get();
        }
        throw std::bad_variant_access();
    }

    template<typename MsgT>
    MsgT* get() {
        if (IsProtoMessage()) {
            return reinterpret_cast<MsgT*>(GetProtoMessage().get());
        }
        else if (IsCppMessage()) {
            return reinterpret_cast<MsgT*>(GetCppMessage().get());
        }
        throw std::bad_variant_access();
    }

    void* release() {
        if (IsProtoMessage()) {
            return GetProtoMessage().release();
        }
        else if (IsCppMessage()) {
            return GetCppMessage().release();
        }
        throw std::bad_variant_access();
    }

private:
    std::variant<ProtoMsgUnique, CppMsgUnique> msg_unique_;
};

}// namespace million