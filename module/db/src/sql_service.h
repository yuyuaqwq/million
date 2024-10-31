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

                    auto msg = std::move(queue_.front());
                    queue_.pop();

                    // 自行分发消息，因为没有调度器，不能使用co_await
                    auto task = MsgDispatch(std::move(msg));
                    task.rethrow_if_exception();
                    if (!task.handle.done()) {
                        std::cerr << "Message processing is waiting." << std::endl;
                    }
                }
                catch (const soci::soci_error& e)
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

    virtual Task OnMsg(MsgUnique msg) override {
        {
            auto guard = std::lock_guard(queue_mutex_);
            queue_.emplace(std::move(msg));
        }
        queue_cv_.notify_one();
        co_return;
    }

    virtual void OnExit() override {
        queue_cv_.notify_one();
        run_ = false;
        thread_->join();
        sql_.close();
    }

    MILLION_MSG_DISPATCH(SqlService, SqlMsgBase);

    MILLION_MSG_HANDLE(SqlCreateTableMsg, msg) {
        const google::protobuf::Descriptor* desc = msg->desc;
        const Db::MessageOptionsTable& options = desc->options().GetExtension(Db::table);
        auto& table_name = options.name();

        std::string sql = "CREATE TABLE " + table_name + " (\n";

        std::unordered_map<std::string, std::vector<const std::string*>> multi_column_map_;
        std::vector<const std::string*> primary_keys;
        for (int i = 0; i < desc->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = desc->field(i);
            const std::string& field_name = field->name();
            std::string field_type;


            if (field->is_repeated()) {
                field_type = "LONGBLOB";
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
                    field_type = "LONGBLOB";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                    field_type = "LONGBLOB"; // Using TEXT for simplicity
                    break;
                case google::protobuf::FieldDescriptor::TYPE_ENUM:
                    field_type = "INTEGER"; // Enum can be stored as INTEGER
                    break;
                }
            }



            // 处理字段选项
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
                    
                    if (sql_options.index() != Db::ColumnSqlOptionsIndex::NONE) {
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
                        else if (sql_options.index() == Db::ColumnSqlOptionsIndex::MULTI_COLUMN && !sql_options.multi_column_name().empty()) {
                            auto iter = multi_column_map_.find(sql_options.multi_column_name());
                            if (iter == multi_column_map_.end()) {
                                multi_column_map_[sql_options.multi_column_name()] = { &field_name };
                            }
                            else {
                                iter->second.emplace_back(&field_name);
                            }
                        }
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
                        field_type += " COMMENT '" + sql_options.comment() + "'"; // 添加注释
                    }
                }
            }

            sql += "    " + field_name + " " + field_type;

            sql += ",\n"; // 注意换行符以便于可读性
        }

        for (const auto& multi_column_info : multi_column_map_) {
            sql += "    INDEX " + multi_column_info.first +" (";
            for (const auto& field_name : multi_column_info.second) {
                sql += *field_name + ",";
            }
            sql.pop_back();
            sql += "),\n";
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

        // 添加字符集和引擎选项
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

        try {
            sql_ << sql;
        }
        catch (const soci::soci_error& e)
        {
            std::cerr << "MySQL error: " << e.what() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        Reply(std::move(msg));
        co_return;
    }

    MILLION_MSG_HANDLE(ParseFromSqlMsg, msg) {
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

        sql += "FROM ";
        sql += table_name;

        const google::protobuf::FieldDescriptor* field_desc = desc->FindFieldByNumber(options.primary_key());
        if (!field_desc) {
            co_return;
        }
        sql += "WHERE ";
        sql += field_desc->name() + " = ";
        sql += msg->primary_key;

        sql += ";";

        // 查询数据
        soci::rowset<soci::row> rs = (sql_.prepare << sql);
        if (rs.begin() == rs.end()) {
            msg->success = false;
            Reply(std::move(msg));
            co_return;
        }

        int i = -1;
        for (const soci::row& row : rs) {
            ++i;
            const google::protobuf::FieldDescriptor* const field = desc->field(i);
            if (field->is_repeated()) {
                google::protobuf::Message* sub_message = reflection->MutableMessage(proto_msg, field);
                sub_message->ParseFromString(row.get<std::string>(i));
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

    std::queue<MsgUnique> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

} // namespace db
} // namespace million