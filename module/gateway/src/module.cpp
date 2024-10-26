#include <atomic>
#include <optional>
#include <mutex>
#include <queue>
#include <sstream>
#include <format>
#include <iomanip>

#include <yaml-cpp/yaml.h>

#include <protogen/cs/cs_user.pb.h>

#include <million/imillion.h>
#include <million/imsg.h>
#include <million/proto_msg.h>

#include <db/cache.h>

#include <gateway/user_session_handle.h>

#include "gateway_service.h"

namespace million {
namespace gateway {

MILLION_FUNC_EXPORT bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<GatewayService>();
    return true;
}

} // namespace gateway
} // namespace million