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
    kJSConfigTableObject,
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

// JSConfigService - 预转换缓存配置服务
class JSConfigService : public IService {
    MILLION_SERVICE_DEFINE(JSConfigService);

public:
    using Base = IService;
    JSConfigService(IMillion* imillion, JSRuntimeService* js_runtime_service);
    ~JSConfigService();

private:
    virtual bool OnInit() override;
    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override;

    MILLION_MESSAGE_HANDLE(config::ConfigUpdateReq, msg) {
        co_await UpdateConfigCache(&msg->config_desc);
        co_return make_message<config::ConfigUpdateResp>();
    }

    MILLION_MESSAGE_HANDLE(JSConfigQueryReq, msg) {
        // 创建临时上下文用于获取配置
        auto cached_table = co_await GetCachedConfigTable(&msg->config_desc);
        co_return make_message<JSConfigQueryResp>(std::move(cached_table));
    }

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

// JSConfigModuleObject - 新的JS配置模块，使用JSConfigService
class JSConfigModuleObject : public mjs::CppModuleObject {
private:
    JSConfigModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static JSConfigModuleObject* New(mjs::Runtime* runtime) {
        return new JSConfigModuleObject(runtime);
    }
};

// JSConfigTableObject - 缓存的配置表对象
class JSConfigTableObject : public mjs::Object {
private:
    JSConfigTableObject(mjs::Context* context, const google::protobuf::Descriptor* descriptor, std::vector<mjs::Value>&& cached_rows);

public:
    static JSConfigTableObject* New(mjs::Context* context, const google::protobuf::Descriptor* descriptor, std::vector<mjs::Value>&& cached_rows) {
        return new JSConfigTableObject(context, descriptor, std::move(cached_rows));
    }

    const google::protobuf::Descriptor* descriptor() const { return descriptor_; }
    const std::vector<mjs::Value>& cached_rows() const { return cached_rows_; }

private:
    const google::protobuf::Descriptor* descriptor_;
    std::vector<mjs::Value> cached_rows_;  // 预转换的JS行对象
};

// JSConfigTableClassDef - 缓存配置表的类定义
class JSConfigTableClassDef : public mjs::ClassDef {
public:
    JSConfigTableClassDef(mjs::Runtime* runtime);
};

// 全局访问JSConfigService实例的函数
JSConfigService* GetJSConfigServiceInstance(ServiceHandle handle);

// JS配置查询消息
MILLION_MESSAGE_DEFINE(, JSConfigQueryReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MESSAGE_DEFINE(, JSConfigQueryResp, (mjs::Value) cached_table)

} // namespace jssvr
} // namespace million