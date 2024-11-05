#include <iostream>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include "sql_service.h"
#include "cache_service.h"
#include "db_service.h"

namespace million {
namespace db {

MILLION_FUNC_API bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();

    auto cache_service_handle = imillion->NewService<CacheService>();
    auto sql_service_handle = imillion->NewService<SqlService>();
    auto db_service_handle = imillion->NewService<DbService>();

    imillion->Send<DbSqlInitMsg>(sql_service_handle, db_service_handle);

    return true;
}

} // namespace db
} // namespace million