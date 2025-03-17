#pragma once

#include <iostream>
#include <queue>
#include <any>
#include <format>

#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>
#ifdef WIN32
#undef GetMessage
#endif

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <db/db_options.pb.h>

#include <million/imillion.h>
#include <million/msg.h>

#include <db/sql.h>

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

    virtual bool OnInit() override {
        imillion().SetServiceName(service_handle(), kSqlServiceName);
        imillion().EnableSeparateWorker(service_handle());
        return true;
    }

    virtual Task<MsgPtr> OnStart(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) override {
        try {
            sql_ = soci::session(soci::mysql, std::format("db={} user={} password={} host={}", db, user, password, host));
        }
        catch (const soci::mysql_soci_error& e) {
            logger().Err("MySQL error : {}", e.what());
        }

        co_return nullptr;
    }

    virtual void OnStop(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) override {
        sql_.close();
    }

    MILLION_MSG_DISPATCH(SqlService);

    MILLION_MSG_HANDLE(SqlTableInitMsg, msg) {
        const auto& desc = msg->desc;
        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");

        int32_t count = 0;
        // Query to check if the table exists in the current database
        sql_ << "SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLES "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = :table_name",
            soci::use(table_name), soci::into(count);
        if (count > 0) {
            co_return std::move(msg_);
        }

        std::string sql = "CREATE TABLE " + table_name + " (\n";

        sql += "    __db_version__ BIGINT  NOT NULL,\n";

        std::vector<const std::string*> primary_keys;
        for (int i = 0; i < desc.field_count(); ++i) {
            const auto* field = desc.field(i);
            if (!field) {
                logger().Err("field({}) is null.", i);
                continue;
            }
            const std::string& field_name = field->name();
            std::string field_type;

            if (field->is_repeated()) {
                logger().Err("db repeated fields are not supported: {}.{}", table_name, field->name());
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

            // 处理字段选项
            if (field->options().HasExtension(column)) {
                const FieldOptionsColumn& field_options = field->options().GetExtension(column);
                if (field_options.has_sql()) {
                    const ColumnSqlOptions& sql_options = field_options.sql();
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
                    
                    if (sql_options.index() == ColumnSqlOptionsIndex::PRIMARY_KEY) {
                        primary_keys.emplace_back(&field_name);
                    }
                    else if (sql_options.index() == ColumnSqlOptionsIndex::UNIQUE) {
                        sql += "    UNIQUE (" + field_name + "),\n";
                    }
                    else if (sql_options.index() == ColumnSqlOptionsIndex::INDEX) {
                        sql += "    INDEX (" + field_name + "),\n";
                    }
                    else if (sql_options.index() == ColumnSqlOptionsIndex::FULLTEXT) {
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
            const TableSqlOptions& sql_options = options.sql();
            if (sql_options.multi_indexs_size() > 0) {
                for (int i = 0; i < sql_options.multi_indexs_size(); ++i) {
                    const auto& multi_index = sql_options.multi_indexs(i);
                    if (multi_index.index() == ColumnSqlOptionsIndex::INDEX) {
                        sql += "    INDEX " + multi_index.index_name() + " (";
                    }
                    else if (multi_index.index() == ColumnSqlOptionsIndex::UNIQUE) {
                        sql += "    UNIQUE INDEX " + multi_index.index_name() + " (";
                    }
                    else if (multi_index.index() == ColumnSqlOptionsIndex::FULLTEXT) {
                        sql += "    FULLTEXT INDEX " + multi_index.index_name() + " (";
                    }
                    else {
                        continue;
                    }
                    for (int i = 0; i < multi_index.field_numbers_size(); ++i) {
                        const auto* field = desc.FindFieldByNumber(multi_index.field_numbers(i));
                        if (!field) {
                            logger().Err("FindFieldByNumber failed: {}.{}", multi_index.field_numbers(i), i);
                            continue;
                        }

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

        if (desc.field_count() > 0) {
            sql.pop_back(); // Remove the last comma
            sql.pop_back(); // Remove the last newline
        }

        sql += "\n) ";

        // 添加字符集和引擎选项
        if (options.has_sql()) {
            const TableSqlOptions& sql_options = options.sql();
            if (!sql_options.charset().empty()) {
                sql += " DEFAULT CHARSET=" + sql_options.charset();
            }
            if (!sql_options.engine().empty()) {
                sql += " ENGINE=" + sql_options.engine();
            }
        }

        sql += ";";

        sql_ << sql;

        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(SqlQueryMsg, msg) {
        auto& proto_msg = msg->db_row->get();
        const auto& desc = msg->db_row->GetDescriptor();
        const auto& reflection = msg->db_row->GetReflection();

        const MessageOptionsTable& options = desc.options().GetExtension(table);
        if (options.has_sql()) {
            const TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();
        if (table_name.empty()) {
            TaskAbort("table_name is empty.");
        }

        std::string sql = "SELECT";
        sql += " __db_version__,";

        for (int i = 0; i < desc.field_count(); ++i) {
            const auto* const field = desc.field(i);
            sql += " " + field->name() + ",";
        }
        if (sql.back() == ',') {
            sql.pop_back();
        }

        sql += " FROM ";
        sql += table_name;

        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        TaskAssert(primary_key_field_desc, 
            "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());
        
        sql += " WHERE ";
        sql += primary_key_field_desc->name() + " = :" + primary_key_field_desc->name() + ";";

        // Prepare SQL statement and bind primary key value
        soci::rowset<soci::row> rs = (sql_.prepare << sql, soci::use(msg->primary_key, primary_key_field_desc->name()));

        auto it = rs.begin();
        if (it == rs.end()) {
            msg->success = false;
            co_return std::move(msg_);
        }

        auto db_version = it->get<uint64_t>(0);
        msg->db_row->set_db_version(db_version);

        const auto& row = *it;
        for (int i = 1; i < desc.field_count() + 1; ++i) {
            const auto* const field = desc.field(i - 1);
            if (!field) {
                logger().Err("desc.field failed: {}.{}.", table_name, i);
                continue;
            }
            if (field->is_repeated()) {
                logger().Err("db repeated fields are not supported: {}.{}", table_name, field->name());
            }
            else {
                auto type = field->type();
                switch (type) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                    reflection.SetDouble(&proto_msg, field, row.get<double>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
                    reflection.SetFloat(&proto_msg, field, static_cast<float>(row.get<double>(i)));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FIXED64:
                case google::protobuf::FieldDescriptor::TYPE_UINT64: {
                    reflection.SetUInt64(&proto_msg, field, row.get<uint64_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_SINT64:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
                case google::protobuf::FieldDescriptor::TYPE_INT64: {
                    reflection.SetInt64(&proto_msg, field, row.get<int64_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FIXED32:
                case google::protobuf::FieldDescriptor::TYPE_UINT32: {
                    reflection.SetUInt32(&proto_msg, field, row.get<uint32_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_SINT32:
                case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
                case google::protobuf::FieldDescriptor::TYPE_INT32: {
                    reflection.SetInt32(&proto_msg, field, row.get<int32_t>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BOOL: {
                    reflection.SetBool(&proto_msg, field, static_cast<bool>(row.get<int8_t>(i)));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_BYTES:
                case google::protobuf::FieldDescriptor::TYPE_STRING: {
                    reflection.SetString(&proto_msg, field, row.get<std::string>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                    google::protobuf::Message* sub_message = reflection.MutableMessage(&proto_msg, field);
                    sub_message->ParseFromString(row.get<std::string>(i));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_ENUM: {
                    int enum_value = row.get<int>(i);
                    const google::protobuf::EnumValueDescriptor* enum_desc = field->enum_type()->FindValueByNumber(enum_value);
                    reflection.SetEnum(&proto_msg, field, enum_desc);
                    break;
                }
                default:
                    logger().Err("unsupported field type:{}", field->name());
                }
            }
        }

        msg->success = true;
        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(SqlInsertMsg, msg) {
        const auto& proto_msg = msg->db_row.get();
        const auto& desc = msg->db_row.GetDescriptor();
        const auto& reflection = msg->db_row.GetReflection();

        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        if (options.has_sql()) {
            const TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");

        std::string sql = "INSERT INTO ";
        sql += table_name;
        sql += " (";
        sql += "__db_version__, ";

        std::string values_placeholder = " VALUES (";
        values_placeholder += "1, ";

        soci::statement stmt(sql_);

        // 构造 SQL 语句
        for (int i = 0; i < desc.field_count(); ++i) {
            const auto* field = desc.field(i);
            if (!field) {
                logger().Err("desc.field failed: {}.{}.", table_name, i);
                continue;
            }
            
            if (field->options().HasExtension(column)) {
                const FieldOptionsColumn& field_options = field->options().GetExtension(column);
                if (field_options.has_sql()) {
                    const ColumnSqlOptions& sql_options = field_options.sql();
                    if (sql_options.auto_increment()) {
                        continue;
                    }
                }
            }
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
        BindValuesToStatement(proto_msg, &values, stmt);
        stmt.define_and_bind();

        stmt.execute(true);
        auto rows = stmt.get_affected_rows();
        msg->success = rows > 0;

        co_return std::move(msg_);
    }

    MILLION_MUT_MSG_HANDLE(SqlUpdateMsg, msg) {
        const auto& proto_msg = msg->db_row.get();
        const auto& desc = msg->db_row.GetDescriptor();
        const auto& reflection = msg->db_row.GetReflection();
        TaskAssert(desc.options().HasExtension(table), "HasExtension table failed.");
        const MessageOptionsTable& options = desc.options().GetExtension(table);
        if (options.has_sql()) {
            const TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");

        std::string sql = "UPDATE ";
        sql += table_name;
        sql += " SET ";

        soci::statement stmt(sql_);

        sql += std::format("__db_version__ = '{}',", msg->db_row.db_version());
        for (int i = 0; i < desc.field_count(); ++i) {
            const auto* field = desc.field(i);
            sql += field->name() + " = :" + field->name() + ",";
        }

        if (sql.back() == ',') {
            sql.pop_back();
        }

        sql += " WHERE ";
        sql += std::format("__db_version__ = '{}' AND __db_version__ < {} ", msg->old_db_version, msg->db_row.db_version());

        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        TaskAssert(primary_key_field_desc,
            "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());

        sql += std::format("AND {} = :{}", primary_key_field_desc->name(), primary_key_field_desc->name());

        // 这里自动根据主键来设置条件进行Update
        //if (!condition.empty()) {
        //    sql += condition;
        //}
        sql += ";";

        stmt.alloc();
        stmt.prepare(sql);
        std::vector<std::any> values;
        BindValuesToStatement(proto_msg, &values, stmt);
        stmt.define_and_bind();

        stmt.execute(true);
        auto rows = stmt.get_affected_rows();
        msg->success = rows > 0;

        co_return std::move(msg_);
    }

private:
    void BindValuesToStatement(const google::protobuf::Message& msg, std::vector<std::any>* values, soci::statement& stmt) {
        const google::protobuf::Descriptor* desc = msg.GetDescriptor();
        const google::protobuf::Reflection* reflection = msg.GetReflection();
        const MessageOptionsTable& options = desc->options().GetExtension(table);
        if (options.has_sql()) {
            const TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();

        values->reserve(desc->field_count());

        for (int i = 0; i < desc->field_count(); ++i) {
            const auto* field = desc->field(i);

            if (field->is_repeated()) {
                logger().Err("db repeated fields are not supported: {}.{}", table_name, field->name());
            }
            else {
                switch (field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_DOUBLE: {
                    values->push_back(reflection->GetDouble(msg, field));
                    stmt.exchange(soci::use(std::any_cast<const double&>(values->back()), field->name()));
                    break;
                }
                case google::protobuf::FieldDescriptor::TYPE_FLOAT: {
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
                    logger().Err("unsupported field type:{}", field->name());
                }
            }
        }
    }

private:
    soci::session sql_;
};

} // namespace db
} // namespace million