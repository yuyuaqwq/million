#include <million/imillion.h>
#include "etcd_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace db {

extern "C" MILLION_DB_API bool MillionModuleInit(IMillion* imillion) {
    auto& settings = imillion->YamlSettings();


    return true;
}

} // namespace db
} // namespace million