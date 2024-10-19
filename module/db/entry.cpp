#include <iostream>
#include <queue>
#include <any>

#include <yaml-cpp/yaml.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

#include <million/imillion.h>
#include <million/imsg.h>

#undef GetMessage

#include "sql.h"
#include "cache.h"
#include "db.h"

MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    
    auto& config = imillion->config();
    auto cache_service_handle = imillion->NewService<CacheService>();
    auto sql_service_handle = imillion->NewService<SqlService>();

    return true;
}
