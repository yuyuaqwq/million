#include <db/db_row.h>

#include <db/db.h>

namespace million {
namespace db {

DbRow::DbRow(ProtoMsgUnique proto_msg)
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

DbRow::DbRow(const DbRow& rv) {
    operator=(rv);
}

DbRow::DbRow(DbRow&& rv) noexcept {
    operator=(std::move(rv));
}

void DbRow::operator=(const DbRow& rv) {
    auto* proto_msg = rv.proto_msg_->New();
    if (proto_msg == nullptr) {
        throw std::bad_alloc();
    }
    proto_msg->CopyFrom(*rv.proto_msg_);
    db_version_ = rv.db_version_;
    proto_msg_.reset(proto_msg);
    dirty_fields_ = rv.dirty_fields_;
}

void DbRow::operator=(DbRow&& rv) noexcept {
    db_version_ = rv.db_version_;
    proto_msg_ = std::move(rv.proto_msg_);
    dirty_fields_ = std::move(rv.dirty_fields_);
}

const google::protobuf::Message& DbRow::get() const { return *proto_msg_.get(); }

google::protobuf::Message& DbRow::get() { return *proto_msg_.get(); }


const protobuf::Descriptor& DbRow::GetDescriptor() const { return *proto_msg_->GetDescriptor(); }

const protobuf::Reflection& DbRow::GetReflection() const { return *proto_msg_->GetReflection(); }

//void DbRow::MoveFrom(DbRow* target) {
//    std::swap(proto_msg_, target->proto_msg_);
//    std::swap(dirty_fields_, target->dirty_fields_);
//
//    // target->proto_msg_->Clear();
//    target->ClearDirty();
//}

//DbRow DbRow::MoveTo() {
//    auto* proto_msg = proto_msg_->New();
//    if (proto_msg == nullptr) {
//        throw std::bad_alloc();
//    }
//    auto row = DbRow(ProtoMsgUnique(proto_msg));
//    row.MoveFrom(this);
//    return row;
//}

//void DbRow::CopyFrom(const google::protobuf::Message& msg) {
//    proto_msg_->CopyFrom(msg);
//    dirty_ = false;
//    dirty_fields_.clear();
//}

DbRow DbRow::CopyDirtyTo() {
    auto* proto_msg = proto_msg_->New();
    if (proto_msg == nullptr) {
        throw std::bad_alloc();
    }
    auto row = DbRow(ProtoMsgUnique(proto_msg));
    row.dirty_fields_ = dirty_fields_;

    row.CopyFromDirty(*this);
    return row;
}

void DbRow::CopyFromDirty(const DbRow& target) {
    const auto* reflection = proto_msg_->GetReflection();
    for (size_t i = 0; i < dirty_fields_.size(); ++i) {
        if (target.dirty_fields_[i]) {
            CopyField(*target.proto_msg_, target.GetFieldByIndex(i), proto_msg_.get(), GetFieldByIndex(i));
            dirty_fields_.at(i) = true;
        }
    }
    set_db_version(target.db_version());
}

bool DbRow::IsDirty() const {
    for (size_t i = 0; i < dirty_fields_.size(); ++i) {
        if (dirty_fields_[i]) {
            return true;
        }
    }
    return false;
}


bool DbRow::IsDirtyFromFIeldNum(int32_t field_number) const {
    return dirty_fields_.at(GetFieldByNumber(field_number).index());
}

bool DbRow::IsDirtyFromFIeldIndex(int32_t field_index) const {
    return dirty_fields_.at(field_index);
}

void DbRow::MarkDirty() {
    for (size_t i = 0; i < dirty_fields_.size(); ++i) {
        dirty_fields_[i] = true;
    }
}

void DbRow::MarkDirtyByFieldNum(int32_t field_number) {
    dirty_fields_.at(GetFieldByNumber(field_number).index()) = true;
}

void DbRow::MarkDirtyByFieldIndex(int32_t field_index) {
    dirty_fields_.at(field_index) = true;
}

void DbRow::ClearDirty() {
    for (size_t i = 0; i < dirty_fields_.size(); ++i) {
        dirty_fields_[i] = false;
    }
}

void DbRow::ClearDirtyByFieldNum(int32_t field_number) {
    dirty_fields_.at(GetFieldByNumber(field_number).index()) = false;
}

void DbRow::ClearDirtyByFieldIndex(int32_t field_index) {
    dirty_fields_.at(field_index) = false;
}

Task<> DbRow::Commit(IService* self, const ServiceHandle& db) {
    co_await self->Call<DbRowUpdateMsg>(db, this);
}

const google::protobuf::FieldDescriptor& DbRow::GetFieldByNumber(int32_t field_number) const {
    auto field_desc = GetDescriptor().FindFieldByNumber(field_number);
    TaskAssert(field_desc, "msg:{}, desc_.FindFieldByNumber:{}", GetDescriptor().name(), field_number);
    return *field_desc;
}

const google::protobuf::FieldDescriptor& DbRow::GetFieldByIndex(int32_t field_index) const {
    auto field_desc = GetDescriptor().field(field_index);
    TaskAssert(field_desc, "msg:{}, desc_.field:{}", GetDescriptor().name(), field_index);
    return *field_desc;
}

void DbRow::CopyField(const google::protobuf::Message& proto_msg1, const google::protobuf::FieldDescriptor& field_desc1
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




void SetField(google::protobuf::Message* proto_msg, const google::protobuf::FieldDescriptor& field, const std::string& value) {
    const auto* desc = proto_msg->GetDescriptor();
    const auto* reflection = proto_msg->GetReflection();

    const MessageOptionsTable& options = desc->options().GetExtension(table);
    const auto& table_name = options.name();

    TaskAssert(!field.is_repeated(),
        "db repeated fields are not supported: {}.{}", table_name, field.name());

    switch (field.type()) {
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
        reflection->SetDouble(proto_msg, &field, std::stod(value));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
        reflection->SetFloat(proto_msg, &field, std::stof(value));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_INT64:
    case google::protobuf::FieldDescriptor::TYPE_SINT64:
    case google::protobuf::FieldDescriptor::TYPE_SFIXED64: {
        reflection->SetInt64(proto_msg, &field, std::stoll(value));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
    case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
        reflection->SetUInt64(proto_msg, &field, std::stoull(value));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_INT32:
    case google::protobuf::FieldDescriptor::TYPE_SINT32:
    case google::protobuf::FieldDescriptor::TYPE_SFIXED32: {
        reflection->SetInt32(proto_msg, &field, std::stoi(value));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
    case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
        reflection->SetUInt32(proto_msg, &field, static_cast<uint32_t>(std::stoul(value)));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_BOOL: {
        reflection->SetBool(proto_msg, &field, value == "1" || value == "true");
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_STRING:
    case google::protobuf::FieldDescriptor::TYPE_BYTES: {
        reflection->SetString(proto_msg, &field, value);
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_ENUM: {
        const google::protobuf::EnumValueDescriptor* enum_value =
            field.enum_type()->FindValueByName(value);
        reflection->SetEnum(proto_msg, &field, enum_value);
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
        google::protobuf::Message* sub_message = reflection->MutableMessage(proto_msg, &field);
        sub_message->ParseFromString(value);
        break;
    }
    default: {
        TaskAbort("Unsupported field type: {}", field.name());
    }
    }

}

std::string GetField(const google::protobuf::Message& proto_msg, const google::protobuf::FieldDescriptor& field) {
    const auto* desc = proto_msg.GetDescriptor();
    const auto* reflection = proto_msg.GetReflection();

    const MessageOptionsTable& options = desc->options().GetExtension(table);
    const auto& table_name = options.name();

    std::string value;
    TaskAssert(!field.is_repeated(),
        "db repeated fields are not supported: {}.{}", table_name, field.name());

    switch (field.type()) {
    case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
        value = std::to_string(reflection->GetDouble(proto_msg, &field));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
        value = std::to_string(reflection->GetFloat(proto_msg, &field));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_INT64:
    case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
    case google::protobuf::FieldDescriptor::TYPE_SINT64: {
        value = std::to_string(reflection->GetInt64(proto_msg, &field));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_UINT64:
    case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
        value = std::to_string(reflection->GetUInt64(proto_msg, &field));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_INT32:
    case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
    case google::protobuf::FieldDescriptor::TYPE_SINT32: {
        value = std::to_string(reflection->GetInt32(proto_msg, &field));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_UINT32:
    case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
        value = std::to_string(reflection->GetUInt32(proto_msg, &field));
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_BOOL: {
        value = reflection->GetBool(proto_msg, &field) ? "true" : "false";
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_STRING:
    case google::protobuf::FieldDescriptor::TYPE_BYTES: {
        value = reflection->GetString(proto_msg, &field);
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_ENUM: {
        int enum_value = reflection->GetEnumValue(proto_msg, &field);
        value = std::to_string(enum_value);
        break;
    }
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
        const google::protobuf::Message& sub_message = reflection->GetMessage(proto_msg, &field);
        value = sub_message.SerializeAsString();
        break;
    }
    default: {
        TaskAbort("Unsupported field type: {}", field.name());
    }
    }

    return value;
}


} // namespace db
} // namespace million