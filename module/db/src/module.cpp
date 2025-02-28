#include <iostream>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <db/api.h>

#include "sql_service.h"
#include "cache_service.h"
#include "db_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace db {

extern "C" MILLION_DB_API bool MillionModuleInit(IMillion* imillion) {
    auto& settings = imillion->YamlSettings();

    auto cache_service_opt = imillion->NewService<CacheService>();
    if (!cache_service_opt) {
        return false;
    }
    imillion->SetServiceName(*cache_service_opt, "CacheService");

    auto sql_service_opt = imillion->NewService<SqlService>();
    if (!sql_service_opt) {
        return false;
    }
    imillion->SetServiceName(*sql_service_opt, "SqlService");

    auto db_service_opt = imillion->NewService<DbService>();
    if (!db_service_opt) {
        return false;
    }
    imillion->SetServiceName(*db_service_opt, "DbService");

    return true;
}

} // namespace db
} // namespace million