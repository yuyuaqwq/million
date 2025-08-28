#include <etcd/SyncClient.hpp>
#include <iostream>
#include <string>

int main() {
    try {
        // 创建同步 etcd 客户端，连接到本地 etcd 服务器
        etcd::SyncClient etcd("http://127.0.0.1:2379");

        // 设置键值对（同步操作）
        etcd::Response set_response = etcd.set("/test/key", "hello world");

        if (set_response.is_ok()) {
            std::cout << "设置键值成功: " << set_response.value().as_string() << std::endl;
        }
        else {
            std::cout << "设置键值失败: " << set_response.error_message() << std::endl;
            return 1;
        }

        // 获取键值（同步操作）
        etcd::Response get_response = etcd.get("/test/key");

        if (get_response.is_ok()) {
            std::cout << "获取键值成功: " << get_response.value().as_string() << std::endl;
        }
        else {
            std::cout << "获取键值失败: " << get_response.error_message() << std::endl;
        }

        // 列出键（同步操作）
        etcd::Response ls_response = etcd.ls("/test");

        if (ls_response.is_ok()) {
            std::cout << "列出键成功，找到 " << ls_response.keys().size() << " 个键:" << std::endl;
            for (const auto& key : ls_response.keys()) {
                std::cout << "  - " << key << std::endl;
            }
        }

        // 删除键（同步操作）
        etcd::Response delete_response = etcd.rm("/test/key");

        if (delete_response.is_ok()) {
            std::cout << "删除键成功" << std::endl;
        }
        else {
            std::cout << "删除键失败: " << delete_response.error_message() << std::endl;
        }

    }
    catch (const std::exception& e) {
        std::cerr << "发生异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}