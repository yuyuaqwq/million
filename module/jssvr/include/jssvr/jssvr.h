#include <jssvr/api.h>

#include <million/imillion.h>

namespace million {
namespace jssvr {

MILLION_JSSVR_API std::optional<ServiceHandle> NewJSService(IMillion* imillion, std::string_view package);

MILLION_MESSAGE_DEFINE(MILLION_JSSVR_API, JsServiceLoadScript, (const std::string) package)

} // namespace jssvr
} // namespace million