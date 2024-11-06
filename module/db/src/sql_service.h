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

#include <db/sql_msg.h>

namespace million {
namespace db {

class SqlService : public IService {
public:
    using Base = IService;
    using Base::Base;

    static inline const std::string_view db = "pokemon_server";
    static inline const std::string_view host = "121.37.17.176";
    static inline const std::string_view user = "root";
    static inline const std::string_view password = "You_Yu666";

    virtual void OnInit() override {
        try {
            // ������ MySQL ���ݿ������
            sql_ = soci::session(soci::mysql, std::format("db={} user={} password={} host={}", db, user, password, host));
        }
        catch (const soci::mysql_soci_error& e) {
            std::cerr << "MySQL error: " << e.what() << std::endl;
        } 
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        EnableSeparateWorker();
    }

    virtual void OnExit() override {
        sql_.close();
    }

    MILLION_MSG_DISPATCH(SqlService);

    MILLION_MSG_HANDLE(SqlCreateTableMsg, msg) {
        const google::protobuf::Descriptor* desc = msg->desc;
        const Db::MessageOptionsTable& options = desc->options().GetExtension(Db::table);
        auto& table_name = options.name();

        std::string sql = "CREATE TABLE " + table_name + " (\n";

        std::vector<const std::string*> primary_keys;
        for (int i = 0; i < desc->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = desc->field(i);
            const std::string& field_name = field->name();
            std::string field_type;

            if (field->is_repeated()) {
                throw std::runtime_error(std::format("db repeated fields are not supported: {}.{}", table_name, field->name()));
            }
            else {
                switch (field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                    field_type = "DOUBLE";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_FLOAT:
                    field_type = "FLOAT";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_INT32:
                case google::protobuf::FieldDescriptor::TYPE_SINT32:
                case google::protobuf::FieldDescriptor::TYPE_FIXED32:
                    field_type = "INTEGER";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_INT64:
                case google::protobuf::FieldDescriptor::TYPE_SINT64:
                case google::protobuf::FieldDescriptor::TYPE_FIXED64:
                    field_type = "BIGINT";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_BOOL:
                    field_type = "BOOLEAN";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_STRING:
                    field_type = "LONGTEXT";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_BYTES:
                    field_type = "LONGTEXT";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                    field_type = "LONGTEXT"; // Using TEXT for simplicity
                    break;
                case google::protobuf::FieldDescriptor::TYPE_ENUM:
                    field_type = "INTEGER"; // Enum can be stored as INTEGER
                    break;
                }
            }

            // �����ֶ�ѡ��
            if (field->options().HasExtension(Db::column)) {
                const Db::FieldOptionsColumn& field_options = field->options().GetExtension(Db::column);
                if (field_options.has_sql()) {
                    const Db::ColumnSqlOptions& sql_options = field_options.sql();
                    if (sql_options.str_len()) {
                        if (sql_options.str_len() > 0) {
                            field_type = "VARCHAR(";
                        }
                        else {
                            field_type = "CHAR(";
                        }
                        field_type += std::to_string(std::abs(sql_options.str_len()));
                        field_type += ")";
                    }
                    
                    if (sql_options.index() == Db::ColumnSqlOptionsIndex::PRIMARY_KEY) {
                        primary_keys.emplace_back(&field_name);
                    }
                    else if (sql_options.index() == Db::ColumnSqlOptionsIndex::UNIQUE) {
                        sql += "    UNIQUE (" + field_name + "),\n";
                    }
                    else if (sql_options.index() == Db::ColumnSqlOptionsIndex::INDEX) {
                        sql += "    INDEX (" + field_name + "),\n";
                    }
                    else if (sql_options.index() == Db::ColumnSqlOptionsIndex::FULLTEXT) {
                        sql += "    FULLTEXT (" + field_name + "),\n";
                    }


                    if (sql_options.auto_increment()) {
                        field_type += " AUTO_INCREMENT";
                    }
                    if (sql_options.not_null()) {
                        field_type += " NOT NULL";
                    }
                    
                    if (!sql_options.default_value().empty()) {
                        field_type += " DEFAULT " + sql_options.default_value();
                    }
                    
                    if (!sql_options.comment().empty()) {
                        field_type += " COMMENT '" + sql_options.comment() + "'";
                    }
                }
            }

            sql += "    " + field_name + " " + field_type;

            sql += ",\n"; 
        }

        if (options.has_sql()) {
            const Db::TableSqlOptions& sql_options = options.sql();
            if (sql_options.multi_indexs_size() > 0) {
                for (int i = 0; i < sql_options.multi_indexs_size(); ++i) {
                    const auto& multi_index = sql_options.multi_indexs(i);
                    if (multi_index.index() == Db::ColumnSqlOptionsIndex::INDEX) {
                        sql += "    INDEX " + multi_index.index_name() + " (";
                    }
                    else if (multi_index.index() == Db::ColumnSqlOptionsIndex::UNIQUE) {
                        sql += "    UNIQUE INDEX " + multi_index.index_name() + " (";
                    }
                    else if (multi_index.index() == Db::ColumnSqlOptionsIndex::FULLTEXT) {
                        sql += "    FULLTEXT INDEX " + multi_index.index_name() + " (";
                    }
                    else {
                        continue;
                    }
                    for (int i = 0; i < multi_index.field_numbers_size(); ++i) {
                        const google::protobuf::FieldDescriptor* field = desc->FindFieldByNumber(multi_index.field_numbers(i));
                        sql += field->name() + ",";
                    }
                    sql.pop_back();
                    sql += "),\n";
                }
            }
        }

        if (!primary_keys.empty()) {
            sql += "    PRIMARY KEY (";
            for (const auto& field_name : primary_keys) {
                sql += *field_name + ",";
            }
            sql.pop_back();
            sql += "),\n";
        }

        if (desc->field_count() > 0) {
            sql.pop_back(); // Remove the last comma
            sql.pop_back(); // Remove the last newline
        }

        sql += "\n) ";

        // ����ַ���������ѡ��
        if (options.has_sql()) {
            const Db::TableSqlOptions& sql_options = options.sql();
            if (!sql_options.charset().empty()) {
                sql += " DEFAULT CHARSET=" + sql_options.charset();
            }
            if (!sql_options.engine().empty()) {
                sql += " ENGINE=" + sql_options.engine();
            }
        }

        sql += ";";

        sql_ << sql;
        
        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(SqlQueryMsg, msg) {
        google::protobuf::Message* proto_msg = msg->proto_msg;
        const google::protobuf::Descriptor* desc = proto_msg->GetDescriptor();
        const google::protobuf::Reflection* reflection = proto_msg->GetReflection();

        const Db::MessageOptionsTable& options = desc->options().GetExtension(Db::table);
        if (options.has_sql()) {
            const Db::TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();

        std::string sql = "SELECT";
        for (int i = 0; i < desc->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* const field = desc->field(i);
            sql += " " + field->name() + ",";
        }
        if (sql.back() == ',') {
            sql.pop_back();
        }

        sql += " FROM ";
        sql += table_name;

        const google::protobuf::FieldDescriptor* field_desc = desc->FindFieldByNumber(options.primary_key());
        if (!field_desc) {
            co_return;
        }

        sql += " WHERE ";
        sql += field_desc->name() + " = :" + field_desc->name() + ";";

        // Prepare SQL statement and bind primary key value
        soci::rowset<soci::row> rs = (sql_.prepare << sql, soci::use(msg->primary_key, field_desc->name()));

        auto it = rs.begin();
        if (it == rs.end()) {
            msg->success = false;
            Reply(std::move(msg));
            co_return;
        }
        const auto& row = *it;
        for (int i = 0; i < desc->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* const field = desc->field(i);
            if (field->is_repeated()) {
                throw std::runtime_error(std::format("db repeated fields are not supported: {}.{}", table_name, field->name()));
            }
            else {
                auto type = field->type();
                switch (type) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                    reflection->SetDouble(proto_msg, field, row.get<double>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                    reflection->SetFloat(proto_msg, field, static_cast<float>(row.get<double>(i)));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FIXED64:
                case google::protobuf::FieldDescriptor::TYPE_UINT64: {
                    reflection->SetUInt64(proto_msg, field, row.get<uint64_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_SINT64:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
                case google::protobuf::FieldDescriptor::TYPE_INT64: {
                    reflection->SetInt64(proto_msg, field, row.get<int64_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FIXED32:
                case google::protobuf::FieldDescriptor::TYPE_UINT32: {
                    reflection->SetUInt32(proto_msg, field, row.get<uint32_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_SINT32:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
                case google::protobuf::FieldDescriptor::TYPE_INT32: {
                    reflection->SetInt32(proto_msg, field, row.get<int32_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                    reflection->SetBool(proto_msg, field, static_cast<bool>(row.get<int8_t>(i)));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BYTES:
                case google::protobuf::FieldDescriptor::TYPE_STRING: {
                    reflection->SetString(proto_msg, field, row.get<std::string>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                    google::protobuf::Message* sub_message = reflection->MutableMessage(proto_msg, field);
                    sub_message->ParseFromString(row.get<std::string>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                    int enum_value = row.get<int>(i);
                    const google::protobuf::EnumValueDescriptor* enum_desc = field->enum_type()->FindValueByNumber(enum_value);
                    reflection->SetEnum(proto_msg, field, enum_desc);
                    break;
                }
                default:
                    std::cerr << "unsupported field type:" << field->type() << std::endl;
                }
            }
            msg->dirty_bits->operator[](i) = false;
        }

        msg->success = true;
        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(SqlInsertMsg, msg) {
        google::protobuf::Message* proto_msg = msg->proto_msg;

        const google::protobuf::Descriptor* desc = proto_msg->GetDescriptor();
        const google::protobuf::Reflection* reflection = proto_msg->GetReflection();
        const Db::MessageOptionsTable& options = desc->options().GetExtension(Db::table);
        if (options.has_sql()) {
            const Db::TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();

        std::string sql = "INSERT INTO ";
        sql += table_name;
        sql += " (";
        std::string values_placeholder = " VALUES (";

        soci::statement stmt(sql_);

        // ���� SQL ���
        for (int i = 0; i < desc->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = desc->field(i);
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
        BindValuesToStatement(*proto_msg, &values, stmt);  // ��ֵ
        stmt.define_and_bind();

        stmt.execute(true);

        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(SqlUpdateMsg, msg) {
        google::protobuf::Message* proto_msg = msg->proto_msg;

        const google::protobuf::Descriptor* desc = proto_msg->GetDescriptor();
        const google::protobuf::Reflection* reflection = proto_msg->GetReflection();
        const Db::MessageOptionsTable& options = desc->options().GetExtension(Db::table);
        if (options.has_sql()) {
            const Db::TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();

        std::string sql = "UPDATE ";
        sql += table_name;
        sql += " SET ";

        soci::statement stmt(sql_);

        for (int i = 0; i < desc->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = desc->field(i);
            sql += field->name() + " = :" + field->name() + ",";
        }

        if (sql.back() == ',') {
            sql.pop_back();
        }

        // �����Զ�����������������������Update
        //if (!condition.empty()) {
        //    sql += " WHERE ";
        //    sql += condition;
        //}
        sql += ";";

        stmt.alloc();
        stmt.prepare(sql);
        std::vector<std::any> values;
        BindValuesToStatement(*proto_msg, &values, stmt);  // ��ֵ
        stmt.define_and_bind();

        try {
            stmt.execute(true);
        }
        catch (...)
        {
            Reply(std::move(msg));
            throw;
        }
        Reply(std::move(msg));
        co_return;
    }

    void BindValuesToStatement(const google::protobuf::Message& msg, std::vector<std::any>* values, soci::statement& stmt) {
        const google::protobuf::Descriptor* desc = msg.GetDescriptor();
        const google::protobuf::Reflection* reflection = msg.GetReflection();
        const Db::MessageOptionsTable& options = desc->options().GetExtension(Db::table);
        if (options.has_sql()) {
            const Db::TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();

        values->reserve(desc->field_count());

        for (int i = 0; i < desc->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = desc->field(i);

            if (field->is_repeated()) {
                throw std::runtime_error(std::format("db repeated fields are not supported: {}.{}", table_name, field->name()));
            }
            else {
                switch (field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                    values->push_back(reflection->GetDouble(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const double&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                    // ��ֱ��֧��float
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
                    // ��ֱ��֧��bool
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
};

} // namespace db
} // namespace million