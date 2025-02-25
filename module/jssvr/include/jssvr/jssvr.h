#include <jssvr/api.h>

#include <million/imillion.h>

namespace million {
namespace jssvr {

MILLION_JSSVR_API std::optional<ServiceHandle> NewJsService(IMillion* imillion, std::string_view package);

} // namespace jssvr
} // namespace million