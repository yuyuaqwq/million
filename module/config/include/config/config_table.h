#include <unordered_map>
#include <initializer_list>

#include <million/imillion.h>

#include <config/api.h>

namespace million {
namespace config {

class MILLION_CONFIG_API ConfigTableBase : public noncopyable {
public:
    // 使用variant来支持不同类型的键
    using IndexKey = std::variant<
        int32_t, uint32_t, int64_t, uint64_t,
        bool, float, double, std::string>;

    // 哈希函数为variant特化
    struct IndexKeyHash {
        size_t operator()(const IndexKey& key) const {
            return std::visit([](const auto& k) {
                return std::hash<std::decay_t<decltype(k)>>{}(k);
                }, key);
        }
    };

    // 比较函数为variant特化
    struct IndexKeyEqual {
        bool operator()(const IndexKey& lhs, const IndexKey& rhs) const {
            return lhs == rhs;
        }
    };

    using FieldIndex = std::unordered_map<IndexKey, int32_t, IndexKeyHash, IndexKeyEqual>;


    // 组合索引
    using CompositeKey = std::vector<IndexKey>;

    struct CompositeKeyHash {
        size_t operator()(const CompositeKey& keys) const {
            size_t seed = 0;
            for (const auto& key : keys) {
                std::visit([&seed](const auto& k) {
                    seed ^= std::hash<std::decay_t<decltype(k)>>{}(k)+0x9e3779b9 + (seed << 6) + (seed >> 2);
                    }, key);
            }
            return seed;
        }
    };

    struct CompositeKeyEqual {
        bool operator()(const CompositeKey& lhs, const CompositeKey& rhs) const {
            if (lhs.size() != rhs.size()) return false;
            for (size_t i = 0; i < lhs.size(); ++i) {
                if (lhs[i] != rhs[i]) return false;
            }
            return true;
        }
    };

    using CompositeIndex = std::unordered_map<CompositeKey, int32_t, CompositeKeyHash, CompositeKeyEqual>;

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

        FieldIndex index;
        auto reflection = table_->GetReflection();
        size_t row_count = GetRowCount();

        for (size_t i = 0; i < row_count; ++i) {
            const auto& row = reflection->GetRepeatedMessage(*table_, table_field_, i);
            const auto& field_reflection = *row.GetReflection();

            IndexKey key = GetFieldValue(field_reflection, row, field_desc);

            auto [it, inserted] = index.emplace(key, i);
            TaskAssert(inserted, "Duplicate key found when building index for field {}", field_desc->full_name());
        }

        field_index_.emplace(field_number, std::move(index));
    }


    const ProtoMsg* FindRowByIndex(int32_t field_number, const IndexKey& key) {
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
            CompositeKey composite_key;

            for (const auto* field_desc : field_descriptors) {
                const auto& field_reflection = *row.GetReflection();
                composite_key.push_back(GetFieldValue(field_reflection, row, field_desc));
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

        CompositeKey key;
        (key.emplace_back(std::forward<Args>(args)), ...);

        const auto& index = it->second;
        auto entry = index.find(key);
        if (entry == index.end()) {
            return nullptr;
        }

        return GetRowByIndex(entry->second);
    }

private:

    // 获取字段值的辅助函数
    IndexKey GetFieldValue(const google::protobuf::Reflection& reflection,
        const ProtoMsg& row,
        const google::protobuf::FieldDescriptor* field_desc) {
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
            TaskAssert(false, "Unsupported field type for indexing: {}", field_desc->cpp_type_name());
            return {};
        }
    }
    
private:
    ProtoMsgShared table_;
    const google::protobuf::FieldDescriptor* table_field_;

    // 字段索引
    std::unordered_map<int32_t, FieldIndex> field_index_;

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
class ConfigTable : public noncopyable {
public:
    ConfigTable(ProtoMsgShared table)
        : base_(std::move(table), ConfigMsgT::GetDescriptor()) {}
    
    ConfigTable(ConfigTableBase&& base)
        : base_(std::move(base)) {}


    ~ConfigTable() = default;

    ConfigTable(ConfigTable&& other) noexcept
        : base_(std::move(other.base_)) {}

    void operator=(ConfigTable&& other) noexcept {
        base_ = std::move(other.base_);
    }

public:
    const ConfigMsgT* FindRow(const std::function<bool(const ConfigMsgT&)>& predicate) {
        return static_cast<const ConfigMsgT*>(base_.FindRow([predicate](const ProtoMsg& row) -> bool {
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

    void BuildIndex(int32_t field_number) {
        base_.BuildIndex(field_number);
    }

    const ConfigMsgT* FindRowByIndex(int32_t field_number, const ConfigTableBase::IndexKey& key) {
        return static_cast<const ConfigMsgT*>(base_.FindRowByIndex(field_number, key));
    }

    void BuildCompositeIndex(std::initializer_list<int32_t> field_numbers) {
        base_.BuildCompositeIndex(std::move(field_numbers));
    }

    template <typename... Args>
    const ProtoMsg* FindRowByCompositeIndex(std::initializer_list<int32_t> field_numbers, Args&&... args) {
        return base_.FindRowByCompositeIndex(std::move(field_numbers), std::forward<Args>(args)...);
    }


private:
    ConfigTableBase base_;
};

} // namespace config
} // namespace million