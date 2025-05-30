#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object_impl/cpp_module_object.h>

#include <db/db_row.h>
#include <config/config.h>

#include <million/imillion.h>

namespace million {
namespace jssvr {


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

    mjs::ConstIndex commit_const_index() const { return commit_const_index_; }

private:
    mjs::ConstIndex commit_const_index_;

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

class ConfigTableObject : public mjs::Object {
private:
    ConfigTableObject(mjs::Context* context, config::ConfigTableShared config_table);

public:
    static ConfigTableObject* New(mjs::Context* context, config::ConfigTableShared config_table) {
        return new ConfigTableObject(context, std::move(config_table));
    }

    void SetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value&& value) override;
    bool GetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value* value) override;

private:
    config::ConfigTableShared config_table_;
};

} // namespace jssvr
} // namespace million