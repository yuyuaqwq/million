#include <protogen.h>

namespace million {

extern "C" MILLION_PROTOGEN_API const google::protobuf::DescriptorPool & GetDescriptorPool() {
	static const auto* pool = google::protobuf::DescriptorPool::generated_pool();
	return *pool;
}

extern "C" MILLION_PROTOGEN_API google::protobuf::DescriptorDatabase & GetDescriptorDatabase() {
	static const auto& pool = GetDescriptorPool();
	static auto* db = pool.internal_generated_database();
	return *db;
}

extern "C" MILLION_PROTOGEN_API google::protobuf::MessageFactory & GetMessageFactory() {
	static auto* factory = google::protobuf::MessageFactory::generated_factory();
	return *factory;
}

} // namespace million