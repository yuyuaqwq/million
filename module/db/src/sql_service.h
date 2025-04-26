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
    MILLION_SERVICE_DEFINE(SqlService);

public:
    using Base = IService;
    using Base::Base;

    static inline std::string name;
    static inline std::string host;
    static inline std::string user;
    static inline std::string password;

    virtual bool OnInit() override {
        // Set service name and enable separate worker
        imillion().SetServiceName(service_handle(), kSqlServiceName);
        imillion().EnableSeparateWorker(service_handle());

        // Read configuration
        const auto& settings = imillion().YamlSettings();
        const auto& db_settings = settings["db"];
        if (!db_settings) {
            logger().Err("cannot find 'db' configuration.");
            return false;
        }

        const auto& sql_settings = db_settings["sql"];
        if (!sql_settings) {
            logger().Err("cannot find 'db.sql' configuration.");
            return false;
        }

        // Get database configuration values
        const auto& db_host = sql_settings["host"];
        const auto& db_name = sql_settings["name"];
        const auto& db_user = sql_settings["user"];
        const auto& db_password = sql_settings["password"];

        if (!db_host || !db_name || !db_user || !db_password) {
            logger().Err("incomplete sql configuration.");
            return false;
        }

        // Assign values (note: this assumes the strings will persist)
        host = db_host.as<std::string>();
        name = db_name.as<std::string>();
        user = db_user.as<std::string>();
        password = db_password.as<std::string>();

        return true;
    }


    virtual Task<MsgPtr> OnStart(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) override {
        try {
            sql_ = soci::session(soci::mysql, std::format("db={} user={} password={} host={}", name, user, password, host));
        }
        catch (const soci::mysql_soci_error& e) {
            logger().Err("MySQL error : {}", e.what());
        }

        co_return nullptr;
    }

    virtual Task<MsgPtr> OnStop(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) override {
        sql_.close();
        co_return nullptr;
    }

    MILLION_MSG_HANDLE(SqlTableInitReq, msg) {
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
            // 新增字段对比逻辑
            std::unordered_set<std::string> existing_columns;
            soci::rowset<std::string> rs =
                (sql_.prepare << "SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS "
                    "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = :table_name",
                    soci::use(table_name));
            for (const auto& column : rs) {
                existing_columns.insert(column);
            }

            std::vector<const google::protobuf::FieldDescriptor*> missing_fields;
            for (int i = 0; i < desc.field_count(); ++i) {
                const auto* field = desc.field(i);
                if (!field || field->name().empty()) continue;
                if (!existing_columns.count(field->name())) {
                    missing_fields.push_back(field);
                }
            }

            if (missing_fields.empty()) {
                co_return make_msg<SqlTableInitResp>(true);
            }

            for (const auto* field : missing_fields) {
                // 复用类型映射逻辑（需提取为独立函数）
                std::string sql_type = MapProtoTypeToSql(table_name, field);

                std::string alter_sql = "ALTER TABLE " + table_name +
                    " ADD COLUMN ";

                // 处理字段选项（索引、约束等）
                ColumnSqlOptionsIndex index;
                ProcessFieldOptions(table_name, field, &sql_type, &index);

                alter_sql += field->name() + " " + sql_type;

                if (index == ColumnSqlOptionsIndex::PRIMARY_KEY) {
                    TaskAbort("Cannot add PRIMARY_KEY to existing table: {}.{} "
                        "(Modifying primary keys via incremental updates is not allowed)",
                        table_name, field->name());
                }
                else if (index == ColumnSqlOptionsIndex::UNIQUE) {
                    alter_sql += "    , ADD UNIQUE (" + field->name() + ")\n";
                }
                else if (index == ColumnSqlOptionsIndex::INDEX) {
                    alter_sql += "    , ADD INDEX (" + field->name() + ")\n";
                }
                else if (index == ColumnSqlOptionsIndex::FULLTEXT) {
                    alter_sql += "    , ADD FULLTEXT (" + field->name() + ")\n";
                }

                alter_sql += ";";

                sql_ << alter_sql;
            }
            co_return make_msg<SqlTableInitResp>(true);
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
            std::string sql_type;

            if (field->is_repeated()) {
                logger().Err("db repeated fields are not supported: {}.{}", table_name, field->name());
                continue;
            }
            else {
                sql_type = MapProtoTypeToSql(table_name, field);
            }

            ColumnSqlOptionsIndex index;
            ProcessFieldOptions(table_name, field, &sql_type, &index);

            if (index == ColumnSqlOptionsIndex::PRIMARY_KEY) {
                primary_keys.emplace_back(&field->name());
            }
            else if (index == ColumnSqlOptionsIndex::UNIQUE) {
                sql += "    UNIQUE (" + field->name() + "),\n";
            }
            else if (index == ColumnSqlOptionsIndex::INDEX) {
                sql += "    INDEX (" + field->name() + "),\n";
            }
            else if (index == ColumnSqlOptionsIndex::FULLTEXT) {
                sql += "    FULLTEXT (" + field->name() + "),\n";
            }

            sql += "    " + field_name + " " + sql_type;

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

        co_return make_msg<SqlTableInitResp>(true);
    }

    MILLION_MSG_HANDLE(SqlQueryReq, msg) {
        auto& proto_msg = msg->db_row.get();
        const auto& desc = msg->db_row.GetDescriptor();
        const auto& reflection = msg->db_row.GetReflection();

        const MessageOptionsTable& options = desc.options().GetExtension(table);
        if (options.has_sql()) {
            const TableSqlOptions& sql_options = options.sql();
        }
        auto& table_name = options.name();
        TaskAssert(!table_name.empty(), "table_name is empty.");

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

        const auto* key_field_desc = desc.FindFieldByNumber(msg->key_field_number);
        TaskAssert(key_field_desc,
            "FindFieldByNumber failed, key_field_number:{}.{}", table_name, msg->key_field_number);
        
        sql += " WHERE ";
        sql += key_field_desc->name() + " = :" + key_field_desc->name() + ";";

        // Prepare SQL statement and bind primary key value
        auto key = msg->key.ToString();
        soci::rowset<soci::row> rs = (sql_.prepare << sql, soci::use(key, key_field_desc->name()));

        auto it = rs.begin();
        if (it == rs.end()) {
            co_return make_msg<SqlQueryResp>(std::nullopt);
        }

        auto db_version = it->get<uint64_t>(0);
        msg->db_row.set_db_version(db_version);

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
                    if (row.get_indicator(i) != soci::i_null) {
                        google::protobuf::Message* sub_message = reflection.MutableMessage(&proto_msg, field);
                        sub_message->ParseFromString(row.get<std::string>(i));
                    }
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

        co_return make_msg<SqlQueryResp>(std::move(msg->db_row));
    }

    MILLION_MSG_HANDLE(SqlInsertReq, msg) {
        auto& db_row = msg->db_row;
        db_row.MarkDirty();


        const auto& proto_msg = db_row.get();
        const auto& desc = db_row.GetDescriptor();
        const auto& reflection = db_row.GetReflection();

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
        BindValuesToStatement(db_row, &values, stmt);
        stmt.define_and_bind();

        stmt.execute(true);
        auto rows = stmt.get_affected_rows();
        msg->success = rows > 0;

        co_return make_msg<SqlInsertResp>(true);
    }

    MILLION_MSG_HANDLE(SqlUpdateReq, msg) {
        auto& db_row = msg->db_row;

        const auto& proto_msg = db_row.get();
        const auto& desc = db_row.GetDescriptor();
        const auto& reflection = db_row.GetReflection();
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

        sql += std::format("__db_version__ = {},", msg->db_row.db_version());
        for (int i = 0; i < desc.field_count(); ++i) {
            const auto* field = desc.field(i);
            if (!db_row.IsDirtyFromFIeldIndex(field->index())) {
                continue;
            }
            sql += field->name() + " = :" + field->name() + ",";
        }

        if (sql.back() == ',') {
            sql.pop_back();
        }

        sql += " WHERE ";
        sql += std::format("__db_version__ = {} AND __db_version__ < {} AND ", msg->old_db_version, msg->db_row.db_version());

        const auto* primary_key_field_desc = desc.FindFieldByNumber(options.primary_key());
        TaskAssert(primary_key_field_desc,
            "FindFieldByNumber failed, options.primary_key:{}.{}", table_name, options.primary_key());

        sql += std::format("{} = :{}", primary_key_field_desc->name(), primary_key_field_desc->name());

        // 这里自动根据主键来设置条件进行Update
        //if (!condition.empty()) {
        //    sql += condition;
        //}
        sql += ";";

        stmt.alloc();
        stmt.prepare(sql);
        std::vector<std::any> values;
        BindValuesToStatement(db_row, &values, stmt);
        stmt.define_and_bind();

        stmt.execute(true);
        auto rows = stmt.get_affected_rows();

        co_return make_msg<SqlUpdateResp>(rows > 0);
    }

private:
    const char* MapProtoTypeToSql(const std::string& table_name, const google::protobuf::FieldDescriptor* field) {
        switch (field->type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            return "DOUBLE";
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            return "FLOAT";
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            return "INTEGER";
            break;
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            return "INT UNSIGNED";
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            return "BIGINT";
            break;
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            return "BIGINT UNSIGNED";
            break;
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            return "BOOLEAN";
            break;
        case google::protobuf::FieldDescriptor::TYPE_STRING:
            return "LONGTEXT";
            break;
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
            return "LONGTEXT";
            break;
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            return "LONGTEXT"; // Using TEXT for simplicity
            break;
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
            return "INTEGER"; // Enum can be stored as INTEGER
            break;
        default:
            TaskAbort("Cannot be converted to SQL type: {}.{}.", table_name, field->name());
        }
    }

    void ProcessFieldOptions(const std::string& table_name, const google::protobuf::FieldDescriptor* field,
        std::string* sql_type, ColumnSqlOptionsIndex* index) {

        *index = ColumnSqlOptionsIndex::NONE;

        // 处理字段选项
        if (!field->options().HasExtension(column)) {
            return;
        }

        const FieldOptionsColumn& field_options = field->options().GetExtension(column);
        if (!field_options.has_sql()) {
            return;
        }

        const ColumnSqlOptions& sql_options = field_options.sql();
        if (sql_options.str_len()) {
            if (sql_options.str_len() > 0) {
                *sql_type = "VARCHAR(";
            }
            else {
                *sql_type = "CHAR(";
            }
            *sql_type += std::to_string(std::abs(sql_options.str_len()));
            *sql_type += ")";
        }

        if (sql_options.auto_increment()) {
            *sql_type += " AUTO_INCREMENT";
        }
        if (sql_options.not_null()) {
            *sql_type += " NOT NULL";
        }

        if (!sql_options.default_value().empty()) {
            *sql_type += " DEFAULT " + sql_options.default_value();
        }

        if (!sql_options.comment().empty()) {
            *sql_type += " COMMENT '" + sql_options.comment() + "'";
        }

        *index = sql_options.index();
    }

    void BindValuesToStatement(const DBRow& db_row, std::vector<std::any>* values, soci::statement& stmt) {
        auto& msg = db_row.get();

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
            if (field->number() != options.primary_key() && !db_row.IsDirtyFromFIeldIndex(field->index())) {
                continue;
            }

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