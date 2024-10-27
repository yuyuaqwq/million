#include <iostream>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include "sql_service.h"
#include "cache_service.h"
#include "db_service.h"

namespace million {
namespace db {

MILLION_FUNC_EXPORT bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();

    auto db_service_handle = imillion->NewService<DbService>();
    auto cache_service_handle = imillion->NewService<CacheService>();
    auto sql_service_handle = imillion->NewService<SqlService>();
    return true;
}

} // namespace db
} // namespace million