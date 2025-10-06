#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object_impl/cpp_module_object.h>

#include <million/imillion.h>
#include <million/db/db_row.h>
#include <million/config/config.h>

namespace million {
namespace jssvr {

// JS配置查询消息
MILLION_MESSAGE_DEFINE(, JSConfigQueryReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MESSAGE_DEFINE(, JSConfigQueryResp, (mjs::Value) cached_table)

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
    static mjs::Value Reply(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

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
    DBRowObject(mjs::Context* context, db::DBRow&& db_row);

public:
    static DBRowObject* New(mjs::Context* context, db::DBRow&& db_row) {
        return new DBRowObject(context, std::move(db_row));
    }

    void SetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value&& value) override;
    bool GetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value* value) override;

    const db::DBRow& db_row() const { return db_row_;}
    db::DBRow& db_row() { return db_row_; }

private:
    db::DBRow db_row_;
};





// JSConfigService - 预转换缓存配置服务
class JSRuntimeService;
class JSConfigService : public IService {
    MILLION_SERVICE_DEFINE(JSConfigService);

public:
    using Base = IService;
    JSConfigService(IMillion* imillion, JSRuntimeService* js_runtime_service);
    ~JSConfigService();

    MILLION_MESSAGE_HANDLE(config::ConfigUpdateReq, msg) {
        co_await UpdateConfigCache(&msg->table_desc);
        co_return make_message<config::ConfigUpdateResp>();
    }

    MILLION_MESSAGE_HANDLE(JSConfigQueryReq, msg) {
        // 创建临时上下文用于获取配置
        auto cached_table = co_await GetCachedConfigTable(&msg->config_desc);
        co_return make_message<JSConfigQueryResp>(std::move(cached_table));
    }

private:
    virtual bool OnInit() override;
    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override;


    // 预加载所有配置
    Task<bool> PreloadAllConfigs();

    // 获取缓存的配置表JS对象
    Task<mjs::Value> GetCachedConfigTable(const google::protobuf::Descriptor* descriptor);

    // 更新特定配置的缓存
    Task<void> UpdateConfigCache(const google::protobuf::Descriptor* descriptor);

    // 后续通过注册配置更新回调

private:
    JSRuntimeService* js_runtime_service_;
    ServiceHandle config_service_handle_;

    // 缓存的JS配置表对象 - 使用descriptor作为key
    std::unordered_map<const google::protobuf::Descriptor*, mjs::Value> cached_config_tables_;

    // 配置名称到descriptor的映射
    std::unordered_map<std::string, const google::protobuf::Descriptor*> config_name_to_descriptor_;
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
    void GCForEachChild(mjs::Context* context, intrusive_list<Object>* list, void(*callback)(mjs::Context* context, intrusive_list<Object>* list, const mjs::Value& child)) override {
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