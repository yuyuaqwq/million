#include <protogen.h>

namespace million {

extern "C" MILLION_PROTOGEN_API const ProtoMeta& GetProtoMeta(){
	static const auto* pool = google::protobuf::DescriptorPool::generated_pool();
	static auto* db = pool->internal_generated_database();
	static auto* factory = google::protobuf::MessageFactory::generated_factory();
	static auto proto_meta = ProtoMeta(pool, db, factory);
	return proto_meta;
}

} // namespace million