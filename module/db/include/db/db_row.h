#pragma once

#include <vector>

#include <million/imillion.h>
#include <million/proto.h>

#include <db/api.h>

namespace million {
namespace db {

class MILLION_DB_API DbRow {
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
        proto_msg_ = std::move(rv.proto_msg_);
        dirty_fields_ = std::move(rv.dirty_fields_);
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

    DbRow CopyDirtyTo() {
        auto* proto_msg = proto_msg_->New();
        if (proto_msg == nullptr) {
            throw std::bad_alloc();
        }
        auto copy = DbRow(ProtoMsgUnique(proto_msg));
        copy.dirty_fields_ = dirty_fields_;

        const auto* reflection = proto_msg_->GetReflection();
        for (size_t i = 0; i < dirty_fields_.size(); ++i) {
            if (copy.dirty_fields_[i]) {
                copy.CopyField(*proto_msg_, GetFieldByIndex(i), copy.proto_msg_.get(), copy.GetFieldByIndex(i));
            }
        }
    }

    void CopyFromDirty(const DbRow& db_row) {
        const auto* reflection = proto_msg_->GetReflection();
        for (size_t i = 0; i < dirty_fields_.size(); ++i) {
            if (db_row.dirty_fields_[i]) {
                CopyField(*db_row.proto_msg_, db_row.GetFieldByIndex(i), proto_msg_.get(), GetFieldByIndex(i));
                dirty_fields_.at(i) = true;
            }
        }
    }


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

    void CopyField(const google::protobuf::Message& proto_msg1, const google::protobuf::FieldDescriptor& field_desc1
        , google::protobuf::Message* proto_msg2, const google::protobuf::FieldDescriptor& field_desc2) {

        const auto* reflection1 = proto_msg1.GetReflection();
        const auto* reflection2 = proto_msg2->GetReflection();

        std::string value;
        if (field_desc1.is_repeated()) {
            THROW_TaskAbortException("CopyField failed, db repeated fields are not supported: {}, {}.", GetDescriptor().name(), field_desc1.name());
        }
        else {
            switch (field_desc1.type()) {
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                reflection2->SetDouble(proto_msg2, &field_desc1, reflection1->GetDouble(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                reflection2->SetFloat(proto_msg2, &field_desc1, reflection1->GetFloat(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            case google::protobuf::FieldDescriptor::TYPE_SINT64: {
                reflection2->SetInt64(proto_msg2, &field_desc1, reflection1->GetInt64(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
                reflection2->SetUInt64(proto_msg2, &field_desc1, reflection1->GetUInt64(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SINT32: {
                reflection2->SetInt32(proto_msg2, &field_desc1, reflection1->GetInt32(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
                reflection2->SetUInt32(proto_msg2, &field_desc1, reflection1->GetUInt32(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                reflection2->SetBool(proto_msg2, &field_desc1, reflection1->GetBool(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_STRING:
            case google::protobuf::FieldDescriptor::TYPE_BYTES: {
                reflection2->SetString(proto_msg2, &field_desc1, reflection1->GetString(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                reflection2->SetEnumValue(proto_msg2, &field_desc1, reflection1->GetEnumValue(proto_msg1, &field_desc2));
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                auto sub_msg2 = reflection2->MutableMessage(proto_msg2, &field_desc1);
                const auto& sub_msg1 = reflection1->GetMessage(proto_msg1, &field_desc2);
                sub_msg2->CopyFrom(sub_msg1);
                break;
            }
            default: {
                THROW_TaskAbortException("CopyField failed, Unsupported field type: {}, {}.", GetDescriptor().name(), field_desc1.name());
            }
            }
        }
    }



private:
    ProtoMsgUnique proto_msg_;
    std::vector<bool> dirty_fields_;
};


} // namespace db
} // namespace million