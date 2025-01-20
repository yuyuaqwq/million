#include <iostream>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <desc/api.h>

#include <desc/desc.pb.h>

MILLION_MODULE_INIT();

namespace million {
namespace desc {

// 1.支持数据热更
// 2.访问配置即返回共享指针，热更只需要重新赋值共享指针即可


extern "C" MILLION_DESC_API bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    
    return true;
}

} // namespace db
} // namespace million