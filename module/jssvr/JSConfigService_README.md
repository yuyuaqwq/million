# JSConfigService - 高性能线程安全JS配置服务

## 概述

JSConfigService是一个专门为JavaScript环境优化的配置服务，采用预转换缓存策略和线程安全设计来解决原有config模块的性能问题。

## 问题背景

原有的config模块存在以下性能问题：
1. **重复转换**：每次访问配置都需要通过`JSUtil::ProtoMessageToJSObject`进行protobuf到JS对象的转换
2. **性能瓶颈**：转换过程涉及遍历所有字段、递归转换嵌套对象、创建新的JS对象和属性
3. **内存开销**：每次都创建新的JS对象，增加GC压力
4. **并发安全**：原方案在并发访问和热更新时存在线程安全问题

## 解决方案

JSConfigService采用**线程安全的预转换缓存策略**：
- 在服务启动时预转换所有配置为JS对象并缓存
- 使用读写锁保护缓存，支持并发读取
- 配置更新时安全地重新转换并更新缓存  
- JS端异步获取缓存的JS对象，避免运行时转换

## 架构设计

### 核心组件

1. **JSConfigService**: 主服务类，负责配置的预加载和线程安全的缓存管理
2. **JSConfigModuleObject**: JS模块对象，提供异步的`jsconfig.load()`接口
3. **JSConfigTableObject**: 缓存的配置表对象，包含预转换的JS行对象
4. **JSConfigTableClassDef**: 配置表的类定义，提供`getRowByIndex`、`findRow`、`getRowCount`等方法

### 线程安全机制

- **读写锁**: 使用`std::shared_mutex`保护配置缓存，支持多读单写
- **更新锁**: 使用`std::mutex`防止同一配置的重复更新
- **异步处理**: 通过消息传递机制实现异步配置获取

### 数据流

```
启动时: ConfigService -> JSConfigService -> 预转换 -> 线程安全缓存JS对象
运行时: JS代码 -> jsconfig.load() -> 异步消息 -> 返回缓存的JS对象
热更新: ConfigUpdateReq -> 线程安全重新加载 -> 更新缓存
```

## 使用方法

### 旧的config模块（低效）
```javascript
import * as config from 'config';

// 每次都需要异步加载和protobuf转换
let config_table_weak = await config.load("pokeworld.config.network.ConnectionDesc");
let config_table = await config_table_weak.lock();

// 每次访问都会进行protobuf到JS的转换
let config_row = config_table.getRowByIndex(0);
```

### 新的jsconfig模块（高效且线程安全）
```javascript
import * as jsconfig from 'jsconfig';

// 异步获取缓存的JS对象，但避免了protobuf转换
let config_table = await jsconfig.load("pokeworld.config.network.ConnectionDesc");

// 直接访问缓存的JS对象，无需转换
let config_row = config_table.getRowByIndex(0);
let row_count = config_table.getRowCount();

// 支持函数式查找
let config_row2 = config_table.findRow(function(row) {
    return row.Port == 10086;
});
```

## 性能优势

1. **启动时预转换**: 所有protobuf到JS的转换在服务启动时完成，运行时零转换开销
2. **并发安全**: 支持多个JS服务并发访问配置，无竞态条件
3. **内存友好**: 每个配置行只转换一次，减少GC压力
4. **热更新安全**: 配置更新时不会影响正在进行的读取操作

## 线程安全特性

### 并发读取
- 多个JS服务可以同时读取配置缓存
- 使用共享锁，不会相互阻塞

### 安全更新
- 配置更新时使用独占锁，确保数据一致性
- 更新期间的读取请求会等待或使用旧缓存

### 重复更新保护
- 防止同一配置的并发更新
- 后续更新请求会等待正在进行的更新完成

## 配置热更新

当配置发生变化时，JSConfigService会：
1. 接收`ConfigUpdateReq`消息
2. 获取更新锁，防止重复更新
3. 重新从ConfigService加载最新配置
4. 线程安全地重新转换并更新缓存
5. 后续访问自动使用新的缓存对象

## 消息流程

### 配置查询流程
```
JS代码 -> jsconfig.load() -> JSConfigQueryReq -> JSConfigService -> JSConfigQueryResp -> Promise resolve
```

### 配置更新流程
```
ConfigService -> ConfigUpdateReq -> JSConfigService -> 重新加载缓存 -> ConfigUpdateResp
```

## 注意事项

1. **内存使用**: 由于预转换所有配置，会增加一定的内存使用
2. **启动时间**: 配置较多时可能会增加服务启动时间
3. **异步接口**: 保持异步接口以确保线程安全，但避免了运行时转换开销
4. **并发性能**: 在高并发场景下性能优异，读取操作几乎无锁竞争

## 实现细节

- 使用`std::unordered_map<const google::protobuf::Descriptor*, mjs::Value>`缓存配置表
- 使用`std::shared_mutex`实现读写锁保护
- 通过消息传递机制实现异步配置获取
- 在JSRuntimeService中自动创建和管理JSConfigService实例
- 支持所有原有的配置表操作方法

## 兼容性

JSConfigService与原有的config模块完全兼容，可以逐步迁移：
- 保留原有config模块用于特殊场景
- 使用jsconfig模块用于高频访问的配置
- 两者可以在同一个项目中并存使用
- API接口保持一致，只需将`config`改为`jsconfig` 