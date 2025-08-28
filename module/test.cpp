#include <etcd/SyncClient.hpp>
#include <iostream>
#include <string>

int main() {
    try {
        // ����ͬ�� etcd �ͻ��ˣ����ӵ����� etcd ������
        etcd::SyncClient etcd("http://127.0.0.1:2379");

        // ���ü�ֵ�ԣ�ͬ��������
        etcd::Response set_response = etcd.set("/test/key", "hello world");

        if (set_response.is_ok()) {
            std::cout << "���ü�ֵ�ɹ�: " << set_response.value().as_string() << std::endl;
        }
        else {
            std::cout << "���ü�ֵʧ��: " << set_response.error_message() << std::endl;
            return 1;
        }

        // ��ȡ��ֵ��ͬ��������
        etcd::Response get_response = etcd.get("/test/key");

        if (get_response.is_ok()) {
            std::cout << "��ȡ��ֵ�ɹ�: " << get_response.value().as_string() << std::endl;
        }
        else {
            std::cout << "��ȡ��ֵʧ��: " << get_response.error_message() << std::endl;
        }

        // �г�����ͬ��������
        etcd::Response ls_response = etcd.ls("/test");

        if (ls_response.is_ok()) {
            std::cout << "�г����ɹ����ҵ� " << ls_response.keys().size() << " ����:" << std::endl;
            for (const auto& key : ls_response.keys()) {
                std::cout << "  - " << key << std::endl;
            }
        }

        // ɾ������ͬ��������
        etcd::Response delete_response = etcd.rm("/test/key");

        if (delete_response.is_ok()) {
            std::cout << "ɾ�����ɹ�" << std::endl;
        }
        else {
            std::cout << "ɾ����ʧ��: " << delete_response.error_message() << std::endl;
        }

    }
    catch (const std::exception& e) {
        std::cerr << "�����쳣: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}