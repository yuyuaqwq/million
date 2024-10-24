#include <iostream>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>
#include <million/imsg.h>

#include "sql.h"
#include "cache.h"
#include "db.h"

MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    
    auto& config = imillion->YamlConfig();
    auto cache_service_handle = imillion->NewService<CacheService>();
    auto sql_service_handle = imillion->NewService<SqlService>();

    return true;
}
