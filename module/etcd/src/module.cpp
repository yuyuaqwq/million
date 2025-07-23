#include <million/module.h>
#include "etcd_service.h"

extern "C" MODULE_API Module* create_module() {
    static class EtcdModule : public Module {
    public:
        const char* name() override {
            return "etcd";
        }
        
        bool init() override {
            // 注册EtcdService到服务工厂
            ServiceFactory::Register<million::etcd::EtcdService>();
            return true;
        }
        
        void destroy() override {
            // 清理资源
        }
    } module;
    
    return &module;
}