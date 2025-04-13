#include <million/imillion.h>

#include <config/api.h>

namespace million {
namespace config {

class MILLION_CONFIG_API ConfigTableBase : public noncopyable {
public:
    ConfigTableBase(ProtoMsgShared table, const google::protobuf::Descriptor* config_descriptor)
        : table_(std::move(table))
        , table_field_(nullptr)
    {
        const auto* descriptor = table_->GetDescriptor();
        const auto* reflection = table_->GetReflection();
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = descriptor->field(i);
            if (!field->is_repeated()) {
                continue;
            }

            auto sub_table_descriptor_ = field->message_type();

            if (!sub_table_descriptor_) {
                continue;
            }

            if (config_descriptor == sub_table_descriptor_) {
                table_field_ = field;
            }
        }
        TaskAssert(table_field_, "There are no matching table.");
    }
    ~ConfigTableBase() = default;

    ConfigTableBase(ConfigTableBase&& other)
        : table_(std::move(other.table_))
        , table_field_(std::move(other.table_field_)) {}

    void operator=(ConfigTableBase&& other) {
        table_ = std::move(other.table_);
        table_field_ = std::move(other.table_field_);
    }

public:
    const ProtoMsg* FindRowByField(const std::function<bool(const ProtoMsg& row)>& predicate) {
        auto reflection = table_->GetReflection();
        size_t row_count = GetRowCount();

        for (size_t i = 0; i < row_count; ++i) {
            const auto& row = reflection->GetRepeatedMessage(*table_, table_field_, i);
            if (predicate(row)) {
                return &row;
            }
        }

        return nullptr;
    }

    const ProtoMsg* GetRowByIndex(size_t idx) {
        TaskAssert(idx < GetRowCount(), "Access row out of bounds: {}.", table_field_->message_type()->full_name());

        auto reflection = table_->GetReflection();
        const auto& row = reflection->GetRepeatedMessage(*table_, table_field_, idx);
        return &row;
    }

    size_t GetRowCount() {
        auto reflection = table_->GetReflection();
        return reflection->FieldSize(*table_, table_field_);
    }

    std::string DebugString() {
        return table_->DebugString();
    }

private:
    ProtoMsgShared table_;
    const google::protobuf::FieldDescriptor* table_field_;
};

template <typename ConfigMsgT>
class ConfigTable : public noncopyable {
public:
    ConfigTable(ProtoMsgShared table)
        : base_(std::move(table), ConfigMsgT::GetDescriptor()) {}
    
    ~ConfigTable() = default;

    ConfigTable(ConfigTable&& other)
        : base_(std::move(other.base_)) {}

    void operator=(ConfigTable&& other) {
        base_ = std::move(other.base_);
    }

public:
    const ConfigMsgT* FindRowByField(const std::function<bool(const ConfigMsgT&)>& predicate) {
        return static_cast<const ConfigMsgT*>(base_.FindRowByField([predicate](const ProtoMsg& row) -> bool {
            return predicate(static_cast<const ConfigMsgT&>(row));
        }));
    }

    const ConfigMsgT* GetRowByIndex(size_t idx) {
        return static_cast<const ConfigMsgT*>(base_.GetRowByIndex(idx));
    }

    size_t GetRowCount() {
        return base_.GetRowCount();
    }

    std::string DebugString() {
        return base_.DebugString();
    }

private:
    ConfigTableBase base_;
};

} // namespace config
} // namespace million