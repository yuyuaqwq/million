#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#ifdef protogen_EXPORTS
    #ifdef __linux__
        #define PROTOGEN_FUNC_API __attribute__((visibility("default")))
    #elif defined(_WIN32) || defined(__WIN32__)
        #define PROTOGEN_FUNC_API __declspec(dllexport)
    #else
        #error "Unsupported platform"
    #endif
#else
    #ifdef __linux__
        #define PROTOGEN_FUNC_API __attribute__((visibility("hidden")))
    #elif defined(_WIN32) || defined(__WIN32__)
        #define PROTOGEN_FUNC_API __declspec(dllimport)
    #else
        #error "Unsupported platform"
    #endif
#endif

extern "C" PROTOGEN_FUNC_API const google::protobuf::DescriptorPool& GetDescriptorPool();
extern "C" PROTOGEN_FUNC_API google::protobuf::DescriptorDatabase& GetDescriptorDatabase();
extern "C" PROTOGEN_FUNC_API google::protobuf::MessageFactory& GetMessageFactory();