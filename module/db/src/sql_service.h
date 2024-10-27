#pragma once

#include <iostream>
#include <queue>
#include <any>
#include <format>

#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>
#undef GetMessage

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <protogen/db/db_options.pb.h>

#include <million/imillion.h>
#include <million/proto_msg.h>

namespace million {
namespace db {

class SqlService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    static inline const std::string_view db = "pokemon_server";
    static inline const std::string_view host = "121.37.17.176";
    static inline const std::string_view user = "root";
    static inline const std::string_view password = "You_Yu666";

    virtual void OnInit() override {
        try {
            // 建立与 MySQL 数据库的连接
            sql_ = soci::session(soci::mysql, std::format("db={} user={} password={} host={}", db, user, password, host));
        }
        catch (const soci::mysql_soci_error& e) {
            std::cerr << "MySQL error: " << e.what() << std::endl;
        } 
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        run_ = true;
        thread_.emplace([this]() {
            while (run_) {
                try {
                    std::unique_lock guard(queue_mutex_);
                    while (run_ && queue_.empty()) {
                        queue_cv_.wait(guard);
                    }
                    if (!run_) return;

                    auto& msg = queue_.front();
                    queue_.pop();


                }
                catch (const soci::mysql_soci_error& e)
                {
                    std::cerr << "MySQL error: " << e.what() << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
            }
        });
    }

    virtual million::Task OnMsg(MsgUnique msg) override {
        auto guard = std::lock_guard(queue_mutex_);
        queue_.emplace(std::move(msg));
        co_return;
    }

    virtual void OnExit() override {
        queue_cv_.notify_one();
        run_ = false;
        thread_->join();
        sql_.close();
    }


    void ParseFromSql(google::protobuf::Message* msg) {
        const google::protobuf::Descriptor* descriptor = msg->GetDescriptor();
        const google::protobuf::Reflection* reflection = msg->GetReflection();
        auto& table = descriptor->name();

        std::string sql = "SELECT";
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* const field = descriptor->field(i);
            sql += " " + field->name() + ",";
        }
        if (sql.back() == ',') {
            sql.pop_back();
        }

        sql += "FROM ";
        sql += table;
        sql += ";";

        // 查询数据
        soci::rowset<soci::row> rs = (sql_.prepare << sql);
        int i = -1;
        for (const soci::row& row : rs) {
            ++i;
            const google::protobuf::FieldDescriptor* const field = descriptor->field(i);
            if (field->is_repeated()) {
                google::protobuf::Message* sub_message = reflection->MutableMessage(msg, field);
                sub_message->ParseFromString(row.get<std::string>(i));
            }
            else {
                auto type = field->type();
                switch (type) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                    reflection->SetDouble(msg, field, row.get<double>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                    reflection->SetFloat(msg, field, static_cast<float>(row.get<double>(i)));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FIXED64:
                case google::protobuf::FieldDescriptor::TYPE_UINT64: {
                    reflection->SetUInt64(msg, field, row.get<uint64_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_SINT64:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
                case google::protobuf::FieldDescriptor::TYPE_INT64: {
                    reflection->SetInt64(msg, field, row.get<int64_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FIXED32:
                case google::protobuf::FieldDescriptor::TYPE_UINT32: {
                    reflection->SetUInt32(msg, field, row.get<uint32_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_SINT32:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
                case google::protobuf::FieldDescriptor::TYPE_INT32: {
                    reflection->SetInt32(msg, field, row.get<int32_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                    reflection->SetBool(msg, field, static_cast<bool>(row.get<int8_t>(i)));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BYTES:
                case google::protobuf::FieldDescriptor::TYPE_STRING: {
                    reflection->SetString(msg, field, row.get<std::string>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                    google::protobuf::Message* sub_message = reflection->MutableMessage(msg, field);
                    sub_message->ParseFromString(row.get<std::string>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                    int enum_value = row.get<int>(i);
                    const google::protobuf::EnumValueDescriptor* enum_desc = field->enum_type()->FindValueByNumber(enum_value);
                    reflection->SetEnum(msg, field, enum_desc);
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported field type.");
                }
            }
        }
    }

    void SerializeToSqlForUpdate(const google::protobuf::Message& msg, std::string_view condition) {
        const google::protobuf::Descriptor* descriptor = msg.GetDescriptor();
        const google::protobuf::Reflection* reflection = msg.GetReflection();
        auto& table = descriptor->name();

        std::string sql = "UPDATE ";
        sql += table;
        sql += " SET ";

        soci::statement stmt(sql_);

        // 构造 SQL 语句
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = descriptor->field(i);
            sql += field->name() + " = :" + field->name() + ",";
        }

        if (sql.back() == ',') {
            sql.pop_back();
        }

        if (!condition.empty()) {
            sql += " WHERE ";
            sql += condition;
        }
        sql += ";";

        stmt.alloc();
        stmt.prepare(sql);
        std::vector<std::any> values;
        BindValuesToStatement(msg, &values, stmt);  // 绑定值
        stmt.define_and_bind();
        stmt.execute(true);
    }

    void SerializeToSqlForInsert(const google::protobuf::Message& msg) {
        const google::protobuf::Descriptor* descriptor = msg.GetDescriptor();
        const google::protobuf::Reflection* reflection = msg.GetReflection();
        auto& table = descriptor->name();

        std::string sql = "INSERT INTO ";
        sql += table;
        sql += " (";
        std::string values_placeholder = " VALUES (";

        soci::statement stmt(sql_);

        // 构造 SQL 语句
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = descriptor->field(i);
            sql += field->name() + ",";
            values_placeholder += ":" + field->name() + ",";
        }

        if (sql.back() == ',') {
            sql.pop_back();
        }
        if (values_placeholder.back() == ',') {
            values_placeholder.pop_back();
        }
        
        sql += ")";
        values_placeholder += ")";
        sql += values_placeholder + ";";

        stmt.alloc();
        stmt.prepare(sql);
        std::vector<std::any> values;
        BindValuesToStatement(msg, &values, stmt);  // 绑定值
        stmt.define_and_bind();
        stmt.execute(true);
    }

    void BindValuesToStatement(const google::protobuf::Message& msg, std::vector<std::any>* values, soci::statement& stmt) {
        const google::protobuf::Descriptor* descriptor = msg.GetDescriptor();
        const google::protobuf::Reflection* reflection = msg.GetReflection();
        auto& table = descriptor->name();

        values->reserve(descriptor->field_count());

        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = descriptor->field(i);

            if (field->is_repeated()) {
                const google::protobuf::Message& sub_message = reflection->GetMessage(msg, field);
                values->push_back(sub_message.SerializeAsString());
                stmt.exchange(soci::use(std::any_cast<const std::string&>(values->back()), field->name()));
            }
            else {
                switch (field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                    values->push_back(reflection->GetDouble(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const double&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                    // 不直接支持float
                    values->push_back(static_cast<double>(reflection->GetFloat(msg, field)));
                    stmt.exchange(soci::use(std::any_cast<const double&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_INT64:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
                case google::protobuf::FieldDescriptor::TYPE_SINT64: {
                    values->push_back(reflection->GetInt64(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const int64_t&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_UINT64:
                case google::protobuf::FieldDescriptor::TYPE_FIXED64: {
                    values->push_back(reflection->GetUInt64(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const uint64_t&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_INT32:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
                case google::protobuf::FieldDescriptor::TYPE_SINT32: {
                    values->push_back(reflection->GetInt32(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const int32_t&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_UINT32:
                case google::protobuf::FieldDescriptor::TYPE_FIXED32: {
                    values->push_back(reflection->GetUInt32(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const uint32_t&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                    // 不直接支持bool
                    values->push_back(static_cast<int8_t>(reflection->GetBool(msg, field)));
                    stmt.exchange(soci::use(std::any_cast<const int8_t&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_STRING:
                case google::protobuf::FieldDescriptor::TYPE_BYTES: {
                    values->push_back(reflection->GetString(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const std::string&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                    const google::protobuf::Message& sub_message = reflection->GetMessage(msg, field);
                    values->push_back(sub_message.SerializeAsString());
                    stmt.exchange(soci::use(std::any_cast<const std::string&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                    values->push_back(reflection->GetEnumValue(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const int&>(values->back()), field->name()));
                    break;
                }
                default:
                    std::cerr << "unsupported field type:" << field->type() << std::endl;
                }
            }
        }
    }

private:
    soci::session sql_;
    std::optional<std::jthread> thread_;
    bool run_;

    std::queue<million::MsgUnique> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

} // namespace db
} // namespace million