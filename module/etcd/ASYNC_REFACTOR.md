# EtcdService 异步架构重构说明

## 重构背景

原有的EtcdService使用`SyncClient`同步客户端，在Service线程中直接调用etcd操作，这会导致以下问题：

1. **线程阻塞**：同步操作会阻塞Worker线程，影响其他Service的处理
2. **性能瓶颈**：网络延迟会直接影响Service的响应性能
3. **不符合框架设计**：million框架采用异步消息驱动架构，同步操作破坏了这一设计原则

## 重构方案

### 核心改进

1. **异步客户端**：使用`etcd::Client`替代`etcd::SyncClient`
2. **IO线程执行**：所有etcd操作在IO线程中执行，通过`asio::post`调度
3. **协程等待**：Service线程通过协程等待异步操作结果
4. **事件驱动**：Watcher事件通过线程安全的消息传递机制处理

### 架构对比

#### 重构前（同步阻塞）
```
Service线程 → SyncClient.get() → 网络等待 → 阻塞Service线程
```

#### 重构后（异步非阻塞）
```
Service线程 → asio::post(IO线程) → etcd::Client.get() → 消息回调 → Service线程继续
```

## 技术实现

### 1. 异步操作执行框架

```cpp
template<typename ReturnType, typename AsyncFunc>
std::future<ReturnType> ExecuteAsync(AsyncFunc&& func) {
    // 在IO线程中执行etcd操作，返回future
    // Service线程通过AwaitFuture协程等待结果
}
```

### 2. 协程等待机制

```cpp
template<typename T>
Task<T> AwaitFuture(std::future<T> future) {
    // 1. 创建session_id
    // 2. 在IO线程中等待future结果
    // 3. 通过消息发送结果回Service线程
    // 4. Service线程协程恢复并返回结果
}
```

### 3. 事件驱动Watcher

```cpp
void OnEtcdWatchEvent(const std::string& key, const etcd::Response& response) {
    // 1. 在etcd客户端线程中接收事件
    // 2. 通过asio::post安全传递到IO线程
    // 3. 再通过消息传递到Service线程
    // 4. Service线程处理配置变更事件
}
```

## 性能优势

### 1. 非阻塞特性
- Service线程不会被网络IO阻塞
- 并发处理能力显著提升
- 符合million框架的异步设计理念

### 2. 资源利用优化
- IO操作与CPU密集型操作分离
- 更好的线程池利用率
- 内存和连接复用

### 3. 响应性能
- 消除了网络延迟对Service响应的直接影响
- etcd操作可以并发执行
- Watcher事件实时响应

## 线程安全设计

### 1. 数据结构保护
- Service线程数据结构保持单线程访问
- 跨线程数据通过消息队列传递
- 避免了复杂的锁机制

### 2. 事件传递安全
```cpp
// etcd客户端线程 → asio::post → IO线程 → 消息队列 → Service线程
```

### 3. 生命周期管理
- Watcher和KeepAlive对象在适当的线程中创建和销毁
- 通过智能指针和RAII确保资源安全释放

## 向后兼容性

重构后的API保持完全兼容：

```cpp
// 使用方式保持不变
MILLION_MESSAGE_HANDLE(EtcdGetReq, msg) {
    auto value = co_await ProcessGetRequest(msg->key);
    co_return make_message<EtcdGetResp>(value);
}
```

用户代码无需修改，但获得了性能提升。

## 配置优化

```yaml
etcd:
  endpoints: "http://127.0.0.1:2379"
  heartbeat_check_interval_ms: 30000
  # 新增说明：所有操作现在都是异步的
```

## 错误处理

### 1. 异常传播
```cpp
// IO线程异常 → std::exception_ptr → 消息传递 → Service线程重新抛出
```

### 2. 超时处理
- 保持原有的协程超时机制
- 网络超时在IO线程中处理
- 避免超时影响Service线程

## 测试验证

### 1. 功能测试
- 所有原有功能保持正常
- 服务注册发现正常工作
- 配置监听事件及时响应

### 2. 性能测试
- Service线程响应延迟显著降低
- 并发处理能力大幅提升
- 内存和CPU使用更加均衡

### 3. 稳定性测试
- 长时间运行稳定
- 网络异常恢复正常
- 无内存泄漏和线程问题

## 总结

这次重构彻底解决了etcd模块的同步阻塞问题，将其完美融入million框架的异步架构中。主要收益：

1. **性能提升**：消除了网络IO对Service线程的阻塞
2. **架构统一**：符合framework的异步消息驱动设计
3. **可维护性**：代码结构更清晰，线程安全更可靠
4. **可扩展性**：为future的功能扩展奠定了良好基础

重构保持了100%的API兼容性，用户无感知地获得了显著的性能改进。 