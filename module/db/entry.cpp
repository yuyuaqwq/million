#include <iostream>
#include <queue>
#include <any>

#include "million/imillion.h"
#include "million/imsg.h"

#include <yaml-cpp/yaml.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

#undef GetMessage

// 通过RegisterDBMsg，来构建SQL映射

#include "sql.h"
#include "cache.h"
#include "db.h"



//void PrintField(const google::protobuf::Message& message, const google::protobuf::Reflection* reflection,
//    const google::protobuf::FieldDescriptor* field, int index, int indent) {
//    std::string indent_str(indent, ' ');  // 用于缩进嵌套消息的输出
//
//    if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT32) {
//        int32_t value = (field->is_repeated()) ? reflection->GetRepeatedInt32(message, field, index)
//            : reflection->GetInt32(message, field);
//        std::cout << indent_str << "Field: " << field->name() << " (int32): " << value << std::endl;
//    }
//    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
//        std::string value = (field->is_repeated()) ? reflection->GetRepeatedString(message, field, index)
//            : reflection->GetString(message, field);
//        std::cout << indent_str << "Field: " << field->name() << " (string): " << value << std::endl;
//    }
//    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
//        const google::protobuf::Message& nested_message = (field->is_repeated()) ? reflection->GetRepeatedMessage(message, field, index)
//            : reflection->GetMessage(message, field);
//        std::cout << indent_str << "Nested Message: " << field->name() << std::endl;
//        PrintMessageFields(nested_message, indent + 2);  // 递归调用处理嵌套消息
//    }
//}
//
//void PrintMessageFields(const google::protobuf::Message& message, int indent = 0) {
//    // 获取消息的描述符和反射
//    const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
//    const google::protobuf::Reflection* reflection = message.GetReflection();
//    std::string indent_str(indent, ' ');  // 用于缩进嵌套消息的输出
//
//    for (int i = 0; i < descriptor->field_count(); ++i) {
//        const google::protobuf::FieldDescriptor* field = descriptor->field(i);
//
//        if (field->is_repeated()) {
//            int field_size = reflection->FieldSize(message, field);
//            std::cout << indent_str << "Repeated Field: " << field->name() << " (size=" << field_size << ")" << std::endl;
//
//            for (int j = 0; j < field_size; ++j) {
//                PrintField(message, reflection, field, j, indent + 2);  // 处理每个重复字段
//            }
//        }
//        else {
//            PrintField(message, reflection, field, -1, indent);  // 处理非重复字段
//        }
//    }
//}

//
//class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector {
//public:
//    void AddError(const std::string& filename, int line, int column, const std::string& message) override {
//        std::cerr << "Error in " << filename << ":" << line << ":" << column << " - " << message << std::endl;
//    }
//};
//
//int parseProtoFile()
//{
//    // 准备配置好文件系统
//    google::protobuf::compiler::DiskSourceTree sourceTree;
//    // 将当前路径映射为项目根目录 ， project_root 仅仅是个名字，你可以你想要的合法名字.
//    // sourceTree.MapPath("data", ".");
//    sourceTree.MapPath("", "C:/Users/yuyu/Desktop/projects/milinet/module/db/proto/");
//    // 配置动态编译器.
//    google::protobuf::compiler::Importer importer(&sourceTree, new ErrorCollector());
//    // 动态编译proto源文件。 源文件在./source/proto/test.proto .
//    auto fileDescriptor = importer.Import("message.proto");
//
//
//    std::cout << fileDescriptor->message_type_count() << std::endl;
//    for (auto i = 0; i < fileDescriptor->message_type_count(); i++)
//    {
//        auto descriptor = fileDescriptor->message_type(i);
//
//        std::cout << descriptor->name() << " " << descriptor->field_count() << " " << descriptor->nested_type_count() << std::endl;
//
//        auto descriptor1 = descriptor->containing_type();
//
//        if (descriptor1)
//        {
//            std::cout << descriptor1->name() << std::endl;
//        }
//    }
//
//    std::cout << fileDescriptor->name() << std::endl;
//
//
//    auto descriptor = fileDescriptor->message_type(1);
//    for (auto i = 0; i < descriptor->field_count(); i++)
//    {
//        auto fieldDes = descriptor->field(i);
//        google::protobuf::SourceLocation outLocation;
//        if (fieldDes->GetSourceLocation(&outLocation))
//        {
//            printf("%s: %d %d %d %d\nleading_comments:%s\ntrailing_comments:%s\n",
//                fieldDes->full_name().c_str(),
//                outLocation.start_line, outLocation.start_column, outLocation.end_line, outLocation.end_column,
//                outLocation.leading_comments.c_str(), outLocation.trailing_comments.c_str());
//            for (auto comment : outLocation.leading_detached_comments)
//            {
//                printf("leading_detached_comments:%s\n", comment.c_str());
//            }
//        }
//        else
//        {
//            std::cout << "fail" << std::endl;
//        }
//    }
//
//#if 0
//    // 现在可以从编译器中提取类型的描述信息.
//    auto descriptor1 = importer.pool()->FindMessageTypeByName("T.Test.InMsg");
//
//    // 创建一个动态的消息工厂.
//    google::protobuf::DynamicMessageFactory factory;
//    // 从消息工厂中创建出一个类型原型.
//    auto proto1 = factory.GetPrototype(descriptor1);
//    // 构造一个可用的消息.
//    auto message1 = proto1->New();
//    // 下面是通过反射接口给字段赋值.
//    auto reflection1 = message1->GetReflection();
//    auto filed1 = descriptor1->FindFieldByName("id");
//    reflection1->SetUInt32(message1, filed1, 1);
//
//    // 打印看看
//    std::cout << message1->DebugString() << std::endl;
//
//    std::string output;
//    google::protobuf::util::MessageToJsonString(*message1, &output);
//    std::cout << output << std::endl;
//
//    // 删除消息.
//    delete message1;
//#endif
//    return 0;
//}

MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    // 初始化 Protobuf 库
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // 清理 Protobuf 资源
    // google::protobuf::ShutdownProtobufLibrary();

    auto& config = imillion->config();
    auto cache_service_handle = imillion->NewService<CacheService>();
    auto sql_service_handle = imillion->NewService<SqlService>();

    return true;
}
