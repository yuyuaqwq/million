# Etcd 模块

## 简介

该模块为项目提供了与 etcd 分布式键值存储系统的集成。etcd 是一个强一致的分布式键值存储系统，通常用于共享配置和服务发现。

该模块遵循项目框架规范，提供了一个服务 (`EtcdService`) 和 API 接口，可以方便地在其他服务中使用。

## 功能特性

1. **键值操作**：支持设置、获取和删除键值对
2. **批量操作**：支持获取具有指定前缀的所有键值对
3. **监听机制**：支持监听键的变化
4. **配置管理**：支持配置的加载和监听
5. **服务注册与发现**：支持服务的注册、注销和发现
6. **符合框架规范**：作为标准服务集成到项目框架中

## 依赖项

该模块依赖于 etcd-cpp-apiv3 客户端库，需要以下依赖：

1. OpenSSL
2. gRPC
3. Protobuf

## 配置

在配置文件中添加 etcd 相关配置：

```yaml
etcd:
  endpoints: "http://127.0.0.1:2379"
```

## API 说明

### 初始化

在服务的 `OnInit` 方法中初始化 etcd 客户端：

```cpp
bool MyService::OnInit() override {
    auto handle = imillion().GetServiceByName(million::etcd::kEtcdServiceName);
    if (handle) {
        etcd_service_ = *handle;
    } else {
        logger().LOG_ERROR("Unable to find EtcdService");
        return false;
    }

    // 初始化 etcd 客户端
    if (!million::etcd::EtcdApi::Init(&imillion(), "http://127.0.0.1:2379")) {
        logger().LOG_ERROR("Failed to initialize etcd client");
        return false;
    }

    return true;
}
```

### 基本键值操作

```cpp
// 设置键值对
bool success = co_await million::etcd::EtcdApi::Set(this, etcd_service_, "key", "value");

// 获取键值
auto value = co_await million::etcd::EtcdApi::Get(this, etcd_service_, "key");

// 删除键
bool success = co_await million::etcd::EtcdApi::Del(this, etcd_service_, "key");

// 获取所有具有指定前缀的键值对
auto all = co_await million::etcd::EtcdApi::GetAll(this, etcd_service_, "prefix");
```

### 配置管理

```cpp
// 加载配置
auto config = co_await million::etcd::EtcdApi::LoadConfig(this, etcd_service_, "/config/myapp");

// 监听配置变化
bool watching = co_await million::etcd::EtcdApi::WatchConfig(this, etcd_service_, "/config/myapp", 
    [](const std::string& new_config) {
        // 处理配置变更
    });
```

### 服务注册与发现

```cpp
// 注册服务
bool registered = co_await million::etcd::EtcdApi::RegisterService(this, etcd_service_, 
    "my-service", "127.0.0.1:8080", 10); // TTL 10秒

// 注销服务
bool unregistered = co_await million::etcd::EtcdApi::UnregisterService(this, etcd_service_, 
    "my-service", "127.0.0.1:8080");

// 发现服务
auto services = co_await million::etcd::EtcdApi::DiscoverService(this, etcd_service_, "my-service");
```

## 使用示例

### 配置管理示例

```cpp
// 加载配置
auto config_json = co_await million::etcd::EtcdApi::LoadConfig(this, etcd_service_, "/config/myapp");
if (config_json) {
    // 解析 JSON 配置
    // ...
}

// 监听配置变化
co_await million::etcd::EtcdApi::WatchConfig(this, etcd_service_, "/config/myapp",
    [this](const std::string& new_config) {
        logger().LOG_INFO("Config updated: {}", new_config);
        // 重新加载配置
        // ...
    });
```

### 服务注册与发现示例

```cpp
// 服务启动时注册
bool registered = co_await million::etcd::EtcdApi::RegisterService(this, etcd_service_,
    "game-server", "192.168.1.100:9000", 10);
    
// 服务停止时注销
bool unregistered = co_await million::etcd::EtcdApi::UnregisterService(this, etcd_service_,
    "game-server", "192.168.1.100:9000");

// 发现其他服务
auto gateways = co_await million::etcd::EtcdApi::DiscoverService(this, etcd_service_, "gateway");
for (const auto& addr : gateways) {
    // 连接到网关服务
    // ...
}
```

有关完整示例，请参考 `example.cpp` 和 `config_discovery_service.cpp` 文件。