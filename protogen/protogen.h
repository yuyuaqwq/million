#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#ifdef million_protogen_EXPORTS
    #ifdef __linux__
        #define MILLION_PROTOGEN_FUNC_API __attribute__((visibility("default")))
    #elif defined(_WIN32) || defined(__WIN32__)
        #define MILLION_PROTOGEN_FUNC_API __declspec(dllexport)
    #else
        #error "Unsupported platform"
    #endif
#else
    #ifdef __linux__
        #define MILLION_PROTOGEN_FUNC_API __attribute__((visibility("hidden")))
    #elif defined(_WIN32) || defined(__WIN32__)
        #define MILLION_PROTOGEN_FUNC_API __declspec(dllimport)
    #else
        #error "Unsupported platform"
    #endif
#endif

namespace million {

extern "C" MILLION_PROTOGEN_FUNC_API const google::protobuf::DescriptorPool& GetDescriptorPool();
extern "C" MILLION_PROTOGEN_FUNC_API google::protobuf::DescriptorDatabase& GetDescriptorDatabase();
extern "C" MILLION_PROTOGEN_FUNC_API google::protobuf::MessageFactory& GetMessageFactory();

} // namespace million