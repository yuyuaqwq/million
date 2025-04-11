#include <jssvr/api.h>

#include <million/imillion.h>

namespace million {
namespace jssvr {

MILLION_JSSVR_API std::optional<ServiceHandle> NewJsService(IMillion* imillion, std::string_view package);

MILLION_MSG_DEFINE(MILLION_JSSVR_API, JsServiceLoadScript, (const std::string) package)

} // namespace jssvr
} // namespace million