#include <cstdint>

#include <memory>

namespace milinet {

using SessionId = uint64_t;

class Msg;
using MsgUnique = std::unique_ptr<Msg>;

} // namespace milinet