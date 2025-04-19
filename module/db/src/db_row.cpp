#include <db/db_row.h>

#include <db/db.h>

namespace million {
namespace db {

DBRow::DBRow(ProtoMsgUnique proto_msg)
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

DBRow::DBRow(const DBRow& rv) {
    operator=(rv);
}

DBRow::DBRow(DBRow&& rv) noexcept {
    operator=(std::move(rv));
}

void DBRow::operator=(const DBRow& rv) {
    auto* proto_msg = rv.proto_msg_->New();
    if (proto_msg == nullptr) {
        throw std::bad_alloc();
    }
    proto_msg->CopyFrom(*rv.proto_msg_);
    db_version_ = rv.db_version_;
    proto_msg_.reset(proto_msg);
    dirty_fields_ = rv.dirty_fields_;
}

void DBRow::operator=(DBRow&& rv) noexcept {
    db_version_ = rv.db_version_;
    proto_msg_ = std::move(rv.proto_msg_);
    dirty_fields_ = std::move(rv.dirty_fields_);
}

const google::protobuf::Message& DBRow::get() const { 
    return *proto_msg_.get();
}

google::protobuf::Message& DBRow::get() { 
    return *proto_msg_.get();
}


const protobuf::Descriptor& DBRow::GetDescriptor() const { return *proto_msg_->GetDescriptor(); }

const protobuf::Reflection& DBRow::GetReflection() const { return *proto_msg_->GetReflection(); }


DBRow DBRow::CopyDirtyTo(bool copy_primary_key) {
    auto* proto_msg = proto_msg_->New();
    if (proto_msg == nullptr) {
        throw std::bad_alloc();
    }
    auto row = DBRow(ProtoMsgUnique(proto_msg));
    row.dirty_fields_ = dirty_fields_;

    row.CopyFromDirty(*this, copy_primary_key);
    return row;
}

void DBRow::CopyFromDirty(const DBRow& target, bool copy_primary_key) {
    auto desc = target.proto_msg_->GetDescriptor();
    auto primary_key = 0;
    if (copy_primary_key) {
        TaskAssert(desc->options().HasExtension(table), "{} hasExtension table failed.", desc->name());
        const MessageOptionsTable& options = desc->options().GetExtension(table);
        primary_key = options.primary_key();
    }

    const auto* reflection = proto_msg_->GetReflection();
    for (size_t i = 0; i < dirty_fields_.size(); ++i) {
        const auto* field = desc->field(i);
        TaskAssert(field, "{} field({}) is nullptr.", desc->name(), i);
        
        if (copy_primary_key && primary_key == field->number()
            || target.dirty_fields_[i]) {
            CopyField(*target.proto_msg_, target.GetFieldByIndex(i), proto_msg_.get(), GetFieldByIndex(i));
            dirty_fields_.at(i) = true;
        }
    }
    db_version_ = target.db_version_;
}

bool DBRow::IsDirty() const {
    for (size_t i = 0; i < dirty_fields_.size(); ++i) {
        if (dirty_fields_[i]) {
            return true;
        }
    }
    return false;
}


bool DBRow::IsDirtyFromFIeldNum(int32_t field_number) const {
    return dirty_fields_.at(GetFieldByNumber(field_number).index());
}

bool DBRow::IsDirtyFromFIeldIndex(int32_t field_index) const {
    return dirty_fields_.at(field_index);
}

void DBRow::MarkDirty() {
    for (size_t i = 0; i < dirty_fields_.size(); ++i) {
        dirty_fields_[i] = true;
    }
}

void DBRow::MarkDirtyByFieldNum(int32_t field_number) {
    dirty_fields_.at(GetFieldByNumber(field_number).index()) = true;
}

void DBRow::MarkDirtyByFieldIndex(int32_t field_index) {
    dirty_fields_.at(field_index) = true;
}

void DBRow::ClearDirty() {
    for (size_t i = 0; i < dirty_fields_.size(); ++i) {
        dirty_fields_[i] = false;
    }
}

void DBRow::ClearDirtyByFieldNum(int32_t field_number) {
    dirty_fields_.at(GetFieldByNumber(field_number).index()) = false;
}

void DBRow::ClearDirtyByFieldIndex(int32_t field_index) {
    dirty_fields_.at(field_index) = false;
}

Task<> DBRow::Commit(IService* self, const ServiceHandle& db) {
    auto old_db_version = db_version_++;
    co_await self->Call<DBRowUpdateReq>(db, *this, old_db_version);
}

const google::protobuf::FieldDescriptor& DBRow::GetFieldByNumber(int32_t field_number) const {
    auto field_desc = GetDescriptor().FindFieldByNumber(field_number);
    TaskAssert(field_desc, "msg:{}, desc_.FindFieldByNumber:{}", GetDescriptor().name(), field_number);
    return *field_desc;
}

const google::protobuf::FieldDescriptor& DBRow::GetFieldByIndex(int32_t field_index) const {
    auto field_desc = GetDescriptor().field(field_index);
    TaskAssert(field_desc, "msg:{}, desc_.field:{}", GetDescriptor().name(), field_index);
    return *field_desc;
}

void DBRow::CopyField(const google::protobuf::Message& proto_msg1, const google::protobuf::FieldDescriptor& field_desc1
    , google::protobuf::Message* proto_msg2, const google::protobuf::FieldDescriptor& field_desc2) {

    const auto* reflection1 = proto_msg1.GetReflection();
    const auto* reflection2 = proto_msg2->GetReflection();

    std::string value;
    if (field_desc1.is_repeated()) {
        TaskAbort("CopyField failed, db repeated fields are not supported: {}, {}.", GetDescriptor().name(), field_desc1.name());
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
            TaskAbort("CopyField failed, Unsupported field type: {}, {}.", GetDescriptor().name(), field_desc1.name());
        }
        }
    }
}

} // namespace db
} // namespace million