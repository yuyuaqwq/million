#include "million/imsg.h"

#include <google/protobuf/message.h>

namespace million {



using ProtoMsgUnique = std::unique_ptr<google::protobuf::Message>;

} // namespace million