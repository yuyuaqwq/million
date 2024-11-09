#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include "gateway_service.h"

namespace million {
namespace gateway {


#define AA(NAMESPACE_, MSG_ID_) \
    const bool _MILLION_PROTO_MSG_HANDLE_SET_MSG_ID_##MSG_ID_ = \
        [this] { \
            _MILLION_PROTO_MSG_HANDLE_CURRENT_MSG_ID_ = ::NAMESPACE_::MSG_ID_; \
            return true; \
        }() \

#define BB(MSG_ID_) AA(Cs, MSG_ID_)

MILLION_FUNC_API bool MillionModuleInit(IMillion* imillion) {
    
    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<GatewayService>();
    return true;
}

} // namespace gateway
} // namespace million