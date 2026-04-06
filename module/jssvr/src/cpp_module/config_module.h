#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object/cpp_module_object.h>
#include <mjs/object.h>
#include <atomic>
#include <vector>

#include "db_module.h"

namespace million {
namespace jssvr {

class ConfigModuleObject : public mjs::CppModuleObject {
private:
    ConfigModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static ConfigModuleObject* New(mjs::Runtime* runtime) {
        return new ConfigModuleObject(runtime);
    }
};

// JSConfigTableClassDef - 缓存配置表的类定义
class ConfigTableClassDef : public mjs::ClassDef {
public:
    ConfigTableClassDef(mjs::Runtime* runtime);
};

// ConfigTableObject - 缓存的配置表对象
class ConfigTableObject : public mjs::Object {
private:
    ConfigTableObject(mjs::Runtime* runtime, const google::protobuf::Descriptor* descriptor, std::vector<mjs::Value>&& cached_rows);

public:
    static ConfigTableObject* New(mjs::Runtime* runtime, const google::protobuf::Descriptor* descriptor, std::vector<mjs::Value>&& cached_rows) {
        return new ConfigTableObject(runtime, descriptor, std::move(cached_rows));
    }

    const google::protobuf::Descriptor* descriptor() const { return descriptor_; }
    const std::vector<mjs::Value>& cached_rows() const { return cached_rows_; }

    bool is_expired() const { return is_expired_; }

private:
    std::atomic<bool> is_expired_;
    const google::protobuf::Descriptor* descriptor_;
    std::vector<mjs::Value> cached_rows_;  // 预转换的JS行对象
};

class ConfigTableWeakClassDef : public mjs::ClassDef {
public:
    ConfigTableWeakClassDef(mjs::Runtime* runtime);
};

class ConfigTableWeakObject : public mjs::Object {
private:
    ConfigTableWeakObject(mjs::Context* context, mjs::Value&& table_object);

public:
    void GCForEachChild(mjs::Context* context, mjs::intrusive_list<Object>* list, void(*callback)(mjs::Context* context, mjs::intrusive_list<Object>* list, const mjs::Value& child)) override {
        callback(context, list, table_object_);
        return Object::GCForEachChild(context, list, callback);
    }

    static ConfigTableWeakObject* New(mjs::Context* context, mjs::Value&& table_object) {
        return new ConfigTableWeakObject(context, std::move(table_object));
    }

    mjs::Value Lock(mjs::Context* context);

private:
    mjs::Value table_object_;
};

} // namespace jssvr
} // namespace million
