#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object_impl/cpp_module_object.h>

#include <db/db_row.h>
#include <config/config.h>

#include <million/imillion.h>

namespace million {
namespace jssvr {

enum class CustomClassId {
    kDBRowObject = mjs::ClassId::kCustom,
    kConfigTableObject,
    kConfigTableWeakObject,
};


class MillionModuleObject : public mjs::CppModuleObject {
private:
    MillionModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value NewService(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value MakeMsg(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static MillionModuleObject* New(mjs::Runtime* runtime) {
        return new MillionModuleObject(runtime);
    }
};

class ServiceModuleObject : public mjs::CppModuleObject {
private:
    ServiceModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Send(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Call(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static ServiceModuleObject* New(mjs::Runtime* runtime) {
        return new ServiceModuleObject(runtime);
    }
};

class LoggerModuleObject : public mjs::CppModuleObject {
private:
    LoggerModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Debug(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Info(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Error(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static LoggerModuleObject* New(mjs::Runtime* runtime) {
        return new LoggerModuleObject(runtime);
    }

private:
    static million::SourceLocation GetSourceLocation(const mjs::StackFrame& stack);
};

class DBModuleObject : public mjs::CppModuleObject {
private:
    DBModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static DBModuleObject* New(mjs::Runtime* runtime) {
        return new DBModuleObject(runtime);
    }
};

class DBRowClassDef : public mjs::ClassDef {
public:
    DBRowClassDef(mjs::Runtime* runtime);
};

class DBRowObject : public mjs::Object {
private:
    DBRowObject(mjs::Context* context, ProtoMessageUnique message);

public:
    static DBRowObject* New(mjs::Context* context, ProtoMessageUnique message) {
        return new DBRowObject(context, std::move(message));
    }

    void SetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value&& value) override;
    bool GetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value* value) override;

    const db::DBRow& db_row() const { return db_row_;}
    db::DBRow& db_row() { return db_row_; }

private:
    db::DBRow db_row_;
};

class ConfigModuleObject : public mjs::CppModuleObject {
private:
    ConfigModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static ConfigModuleObject* New(mjs::Runtime* runtime) {
        return new ConfigModuleObject(runtime);
    }
};


class ConfigTableClassDef : public mjs::ClassDef {
public:
    ConfigTableClassDef(mjs::Runtime* runtime);

};

class ConfigTableObject : public mjs::Object {
private:
    ConfigTableObject(mjs::Context* context, config::ConfigTableShared config_table);

public:
    static ConfigTableObject* New(mjs::Context* context, config::ConfigTableShared config_table) {
        return new ConfigTableObject(context, std::move(config_table));
    }

    const config::ConfigTableShared& config_table() const { return config_table_; }
    config::ConfigTableShared& config_table() { return config_table_; }

private:
    config::ConfigTableShared config_table_;
};

class ConfigTableWeakClassDef : public mjs::ClassDef {
public:
    ConfigTableWeakClassDef(mjs::Runtime* runtime);
};

class ConfigTableWeakObject : public mjs::Object {
private:
    ConfigTableWeakObject(mjs::Context* context, config::ConfigTableWeakBase config_table_weak, const google::protobuf::Descriptor* descriptor);

public:
    static ConfigTableWeakObject* New(mjs::Context* context, config::ConfigTableWeakBase config_table_weak, const google::protobuf::Descriptor* descriptor) {
        return new ConfigTableWeakObject(context, std::move(config_table_weak), descriptor);
    }

    const config::ConfigTableWeakBase& config_table_weak() const { return config_table_weak_; }
    config::ConfigTableWeakBase& config_table_weak() { return config_table_weak_; }
    const google::protobuf::Descriptor* descriptor() const { return descriptor_; }

private:
    config::ConfigTableWeakBase config_table_weak_;
    const google::protobuf::Descriptor* descriptor_;
};

} // namespace jssvr
} // namespace million