#include <protogen/protogen.h>

#include <google/protobuf/message.h>

namespace million {

extern "C" MILLION_PROTOGEN_API void ProtogenInit() {
	static auto desc_pool = google::protobuf::DescriptorPool::generated_pool();
	static auto desc_db = desc_pool->internal_generated_database();
	static auto msg_factory = google::protobuf::MessageFactory::generated_factory();
}

} // namespace million