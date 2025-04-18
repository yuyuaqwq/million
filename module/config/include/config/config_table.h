#include <variant>
#include <unordered_map>
#include <initializer_list>

#include <million/imillion.h>
#include <million/proto_field_any.h>

#include <config/api.h>

namespace million {
namespace config {

using Index = std::unordered_map<ProtoFieldAny, int32_t
    , ProtoFieldAnyHash, ProtoFieldAnyEqual>;

using CompositeIndex = std::unordered_map<CompositeProtoFieldAny
    , int32_t, CompositeProtoFieldAnyHash, CompositeProtoFieldAnyEqual>;

class MILLION_CONFIG_API ConfigTableBase : public noncopyable {
public:
    
public:
    ConfigTableBase(ProtoMsgUnique table, const google::protobuf::Descriptor* config_descriptor)
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

    ConfigTableBase(ConfigTableBase&& other) noexcept
        : table_(std::move(other.table_))
        , table_field_(std::move(other.table_field_)) {}

    void operator=(ConfigTableBase&& other) noexcept {
        table_ = std::move(other.table_);
        table_field_ = std::move(other.table_field_);
    }

public:
    const ProtoMsg* FindRow(const std::function<bool(const ProtoMsg& row)>& predicate) {
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

    // 索引
    void BuildIndex(int32_t field_number) {
        const auto* row_descriptor = table_field_->message_type();
        const auto* field_desc = row_descriptor->FindFieldByNumber(field_number);
        TaskAssert(field_desc, "Field number {} not found in row descriptor", field_number);

        Index index;
        auto reflection = table_->GetReflection();
        size_t row_count = GetRowCount();

        for (size_t i = 0; i < row_count; ++i) {
            const auto& row = reflection->GetRepeatedMessage(*table_, table_field_, i);
            const auto& field_reflection = *row.GetReflection();

            ProtoFieldAny key = ProtoMsgGetFieldAny(field_reflection, row, *field_desc);

            auto [it, inserted] = index.emplace(key, i);
            TaskAssert(inserted, "Duplicate key found when building index for field {}", field_desc->full_name());
        }

        field_index_.emplace(field_number, std::move(index));
    }


    const ProtoMsg* FindRowByIndex(int32_t field_number, const ProtoFieldAny& key) {
        auto it = field_index_.find(field_number);
        if (it == field_index_.end()) {
            return nullptr;
        }

        const auto& index = it->second;
        auto entry = index.find(key);
        if (entry == index.end()) {
            return nullptr;
        }

        return GetRowByIndex(entry->second);
    }

    // 组合索引
    void BuildCompositeIndex(std::initializer_list<int32_t> field_numbers) {
        CompositeIndex index;
        auto reflection = table_->GetReflection();
        size_t row_count = GetRowCount();
        const auto* row_descriptor = table_field_->message_type();

        // 验证所有字段都存在
        std::vector<const google::protobuf::FieldDescriptor*> field_descriptors;
        for (auto field_number : field_numbers) {
            const auto* field_desc = row_descriptor->FindFieldByNumber(field_number);
            TaskAssert(field_desc, "Field number {} not found in row descriptor", field_number);
            field_descriptors.push_back(field_desc);
        }

        // 构建索引
        for (size_t i = 0; i < row_count; ++i) {
            const auto& row = reflection->GetRepeatedMessage(*table_, table_field_, i);
            CompositeProtoFieldAny composite_key;

            for (const auto* field_desc : field_descriptors) {
                const auto& field_reflection = *row.GetReflection();
                composite_key.push_back(ProtoMsgGetFieldAny(field_reflection, row, *field_desc));
            }

            auto [it, inserted] = index.emplace(std::move(composite_key), i);
            TaskAssert(inserted, "Duplicate composite key found when building index");
        }

        // 存储索引，使用字段号的组合作为key
        auto field_numbers_vec = std::vector<int32_t>(field_numbers);
        composite_indices_.emplace(std::move(field_numbers_vec), std::move(index));
    }

    template <typename... Args>
    const ProtoMsg* FindRowByCompositeIndex(std::initializer_list<int32_t> field_numbers, Args&&... args) {
        static_assert(sizeof...(Args) > 1, "Composite index requires at least 2 fields");

        auto it = composite_indices_.find(field_numbers);
        if (it == composite_indices_.end()) {
            return nullptr;
        }

        CompositeProtoFieldAny key;
        (key.emplace_back(std::forward<Args>(args)), ...);

        const auto& index = it->second;
        auto entry = index.find(key);
        if (entry == index.end()) {
            return nullptr;
        }

        return GetRowByIndex(entry->second);
    }

private:


private:
    ProtoMsgUnique table_;
    const google::protobuf::FieldDescriptor* table_field_;

    // 字段索引
    std::unordered_map<int32_t, Index> field_index_;

    // 组合索引
    struct VectorIntHash {
        size_t operator()(const std::vector<int32_t>& vec) const {
            size_t seed = vec.size();
            for (const auto& num : vec) {
                seed ^= std::hash<int32_t>{}(num)+0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };

    struct VectorIntEqual {
        bool operator()(const std::vector<int32_t>& lhs, const std::vector<int32_t>& rhs) const {
            return lhs == rhs;
        }
    };

    std::unordered_map<
        std::vector<int32_t>,
        CompositeIndex,
        VectorIntHash,
        VectorIntEqual
    > composite_indices_;
};

template <typename ConfigMsgT>
class ConfigTable : public ConfigTableBase {
public:
    using ConfigTableBase::ConfigTableBase;

public:
    const ConfigMsgT* FindRow(const std::function<bool(const ConfigMsgT&)>& predicate) {
        return static_cast<const ConfigMsgT*>(ConfigTableBase::FindRow([predicate](const ProtoMsg& row) -> bool {
            return predicate(static_cast<const ConfigMsgT&>(row));
        }));
    }

    const ConfigMsgT* GetRowByIndex(size_t idx) {
        return static_cast<const ConfigMsgT*>(ConfigTableBase::GetRowByIndex(idx));
    }


    const ConfigMsgT* FindRowByIndex(int32_t field_number, const ProtoFieldAny& key) {
        return static_cast<const ConfigMsgT*>(ConfigTableBase::FindRowByIndex(field_number, key));
    }

    template <typename... Args>
    const ConfigMsgT* FindRowByCompositeIndex(std::initializer_list<int32_t> field_numbers, Args&&... args) {
        return static_cast<const ConfigMsgT*>(ConfigTableBase::FindRowByCompositeIndex(std::move(field_numbers), std::forward<Args>(args)...));
    }
};

using ConfigTableShared = std::shared_ptr<ConfigTableBase>;

} // namespace config
} // namespace million