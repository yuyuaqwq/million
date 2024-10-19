#include <iostream>
#include <queue>
#include <any>

#include <million/imillion.h>
#include <million/imsg.h>

#include <yaml-cpp/yaml.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>


#undef GetMessage

#include "sql.h"
#include "cache.h"
#include "db.h"


//class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector {
//public:
//    void AddError(const std::string& filename, int line, int column, const std::string& message) override {
//        std::cerr << "Error in " << filename << ":" << line << ":" << column << " - " << message << std::endl;
//    }
//};
//
//int parseProtoFile()
//{
//    // ׼�����ú��ļ�ϵͳ
//    google::protobuf::compiler::DiskSourceTree sourceTree;
//    // ����ǰ·��ӳ��Ϊ��Ŀ��Ŀ¼ �� project_root �����Ǹ����֣����������Ҫ�ĺϷ�����.
//    // sourceTree.MapPath("data", ".");
//    sourceTree.MapPath("", "C:/Users/yuyu/Desktop/projects/milinet/proto/cs/");
//    // ���ö�̬������.
//    google::protobuf::compiler::Importer importer(&sourceTree, new ErrorCollector());
//    // ��̬����protoԴ�ļ��� Դ�ļ���./source/proto/test.proto .
//    auto fileDescriptor = importer.Import("message.proto");
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
//    // ���ڿ��Դӱ���������ȡ���͵�������Ϣ.
//    auto descriptor1 = importer.pool()->FindMessageTypeByName("T.Test.InMsg");
//
//    // ����һ����̬����Ϣ����.
//    google::protobuf::DynamicMessageFactory factory;
//    // ����Ϣ�����д�����һ������ԭ��.
//    auto proto1 = factory.GetPrototype(descriptor1);
//    // ����һ�����õ���Ϣ.
//    auto message1 = proto1->New();
//    // ������ͨ������ӿڸ��ֶθ�ֵ.
//    auto reflection1 = message1->GetReflection();
//    auto filed1 = descriptor1->FindFieldByName("id");
//    reflection1->SetUInt32(message1, filed1, 1);
//
//    // ��ӡ����
//    std::cout << message1->DebugString() << std::endl;
//
//    std::string output;
//    google::protobuf::util::MessageToJsonString(*message1, &output);
//    std::cout << output << std::endl;
//
//    // ɾ����Ϣ.
//    delete message1;
//#endif
//    return 0;
//}



MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    
    auto& config = imillion->config();
    //auto cache_service_handle = imillion->NewService<CacheService>();
    //auto sql_service_handle = imillion->NewService<SqlService>();

    return true;
}
