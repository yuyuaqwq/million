#include <protogen/protogen.h>

#include <google/protobuf/message.h>

namespace million {

extern "C" MILLION_PROTOGEN_API void ProtogenInit() {
	static const auto* pool = google::protobuf::DescriptorPool::generated_pool();
	static auto* db = pool->internal_generated_database();
	static auto* factory = google::protobuf::MessageFactory::generated_factory();
}

} // namespace million