#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto.h>

#include <db/api.h>

namespace million {
namespace db {

class DB_CLASS_API DbRow {
public:
    DbRow(ProtoMsgUnique proto_msg)
        : proto_msg_(std::move(proto_msg))
    {
        auto desc = proto_msg_->GetDescriptor();
        if (!desc) {
            throw std::invalid_argument("DbRow GetDescriptor is null.");
        }
        auto reflection = proto_msg_->GetReflection();
        if (!reflection) {
            throw std::invalid_argument("DbRow GetReflection is null.");
        }
        dirty_fields_.resize(desc->field_count(), false);
    }

    DbRow(const DbRow& rv) {
        operator=(rv);
    }

    DbRow(DbRow&& rv) noexcept {
        operator=(std::move(rv));
    }

    void operator=(const DbRow& rv) {
        auto* proto_msg = rv.proto_msg_->New();
        if (proto_msg == nullptr) {
            throw std::bad_alloc();
        }
        proto_msg->CopyFrom(*rv.proto_msg_);
        proto_msg_.reset(proto_msg);
        dirty_fields_ = rv.dirty_fields_;
    }

    void operator=(DbRow&& rv) noexcept {
        proto_msg_ = std::move(proto_msg_);
        dirty_fields_ = std::move(dirty_fields_);
    }

    template <typename T>
    T& get() const { return static_cast<T&>(get()); }

    google::protobuf::Message& get() const { return *proto_msg_.get(); }

    const protobuf::Descriptor& GetDescriptor() const { return *proto_msg_->GetDescriptor(); }

    const protobuf::Reflection& GetReflection() const { return *proto_msg_->GetReflection(); }

    //void CopyFrom(const google::protobuf::Message& msg) {
    //    proto_msg_->CopyFrom(msg);
    //    dirty_ = false;
    //    dirty_fields_.clear();
    //}

    bool IsDirty() const {
        for (size_t i = 0; i < dirty_fields_.size(); ++i) {
            if (dirty_fields_[i]) {
                return true;
            }
        }
        return false;
    }

    void MarkDirty() {
        for (size_t i = 0; i < dirty_fields_.size(); ++i) {
            dirty_fields_[i] = true;
        }
    }

    void ClearDirty() {
        for (size_t i = 0; i < dirty_fields_.size(); ++i) {
            dirty_fields_[i] = false;
        }
    }

    bool IsDirtyFromFIeldNum(int32_t field_number) const {
        return dirty_fields_.at(GetFieldByNumber(field_number).index());
    }

    bool IsDirtyFromFIeldIndex(int32_t field_index) const {
        return dirty_fields_.at(field_index);
    }

    void MarkDirtyByFieldNum(int32_t field_number) {
        dirty_fields_.at(GetFieldByNumber(field_number).index()) = true;
    }

    void MarkDirtyByFieldIndex(int32_t field_index) {
        dirty_fields_.at(field_index) = true;
    }

    void ClearDirtyByFieldNum(int32_t field_number) {
        dirty_fields_.at(GetFieldByNumber(field_number).index()) = false;
    }

    void ClearDirtyByFieldIndex(int32_t field_index) {
        dirty_fields_.at(field_index) = false;
    }

    Task<> Commit() {
        // 向dbservice提交当前DbRow的修改
        co_return;
    }

private:
    const google::protobuf::FieldDescriptor& GetFieldByNumber(int32_t field_number) const {
        auto field_desc = GetDescriptor().FindFieldByNumber(field_number);
        if (!field_desc) {
            THROW_TaskAbortException("msg:{}, desc_.FindFieldByNumber:{}", GetDescriptor().name(), field_number);
        }
        return *field_desc;
    }

    const google::protobuf::FieldDescriptor& GetFieldByIndex(int32_t field_index) const {
        auto field_desc = GetDescriptor().field(field_index);
        if (!field_desc) {
            THROW_TaskAbortException("msg:{}, desc_.field:{}", GetDescriptor().name(), field_index);
        }
        return *field_desc;
    }

private:
    ProtoMsgUnique proto_msg_;
    std::vector<bool> dirty_fields_;
};


} // namespace db
} // namespace million