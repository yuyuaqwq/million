#include <iostream>

#include <million/imillion.h>
#include <million/imsg.h>

#include <yaml-cpp/yaml.h>



MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    auto& config = imillion->YamlConfig();

    return true;
}
