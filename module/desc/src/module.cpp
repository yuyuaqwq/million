#include <iostream>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <desc/api.h>

#include <desc/desc.pb.h>

MILLION_MODULE_INIT();

namespace million {
namespace desc {

// 1.֧�������ȸ�
// 2.�������ü����ع���ָ�룬�ȸ�ֻ��Ҫ���¸�ֵ����ָ�뼴��


extern "C" MILLION_DESC_API bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    
    return true;
}

} // namespace db
} // namespace million