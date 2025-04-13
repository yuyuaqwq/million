#pragma once

#include <vector>

#include <million/imillion.h>

#include <db/api.h>

namespace million {
namespace db {

class MILLION_DB_API DBRow {
public:
    DBRow(ProtoMsgUnique proto_msg);
    DBRow(const DBRow& rv);
    DBRow(DBRow&& rv) noexcept;

    void operator=(const DBRow& rv);
    void operator=(DBRow&& rv) noexcept;

    template <typename T>
    const T& get() const { return static_cast<T&>(get()); }

    template <typename T>
    T& get() { return static_cast<T&>(get()); }

    google::protobuf::Message& get();
    const google::protobuf::Message& get() const;

    const protobuf::Descriptor& GetDescriptor() const;
    const protobuf::Reflection& GetReflection() const;

    DBRow CopyDirtyTo(bool copy_primary_key);
    void CopyFromDirty(const DBRow& db_row, bool copy_primary_key);
    
    bool IsDirty() const;
    bool IsDirtyFromFIeldNum(int32_t field_number) const;
    bool IsDirtyFromFIeldIndex(int32_t field_index) const;

    void MarkDirty();
    void MarkDirtyByFieldNum(int32_t field_number);
    void MarkDirtyByFieldIndex(int32_t field_index);

    void ClearDirty();
    void ClearDirtyByFieldNum(int32_t field_number);
    void ClearDirtyByFieldIndex(int32_t field_index);

    Task<> Commit(IService* self, const ServiceHandle& db);

    uint64_t db_version() const { return db_version_; }

    // 应该只给Db服务内部用，未来看看改私有+友元
    void set_db_version(uint64_t db_version) { db_version_ = db_version; }

private:
    const google::protobuf::FieldDescriptor& GetFieldByNumber(int32_t field_number) const;
    const google::protobuf::FieldDescriptor& GetFieldByIndex(int32_t field_index) const;

    void CopyField(const google::protobuf::Message& proto_msg1, const google::protobuf::FieldDescriptor& field_desc1
        , google::protobuf::Message* proto_msg2, const google::protobuf::FieldDescriptor& field_desc2);

private:
    uint64_t db_version_ = 0;
    ProtoMsgUnique proto_msg_;
    std::vector<bool> dirty_fields_;
};


void SetField(google::protobuf::Message* proto_msg, const google::protobuf::FieldDescriptor& field, const std::string& value);
std::string GetField(const google::protobuf::Message& proto_msg, const google::protobuf::FieldDescriptor& field);

} // namespace db
} // namespace million