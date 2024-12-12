#include <protogen.h>

extern "C" PROTOGEN_FUNC_API const google::protobuf::DescriptorPool& GetDescriptorPool() {
	static const auto* pool = google::protobuf::DescriptorPool::generated_pool();
	return *pool;
}

extern "C" PROTOGEN_FUNC_API google::protobuf::DescriptorDatabase& GetDescriptorDatabase() {
	static const auto& pool = GetDescriptorPool();
	static auto* db = pool.internal_generated_database();
	return *db;
}

extern "C" PROTOGEN_FUNC_API google::protobuf::MessageFactory& GetMessageFactory() {
	static auto* factory = google::protobuf::MessageFactory::generated_factory();
	return *factory;
}