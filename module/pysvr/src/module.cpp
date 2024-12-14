#include "iostream"

#include <google/protobuf/message.h>
#include <string>

#include <million/imillion.h>
#include <million/proto.h>

#include <pysvr/api.h>

#include <pocketpy/pocketpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>


namespace py = pybind11;
//
//bool set_field(google::protobuf::Message* message, const std::string& field_name, const py::object& value) {
//    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
//    const google::protobuf::Reflection* reflection = message->GetReflection();
//
//    const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName(field_name);
//    if (!field) {
//        py::print("Field", field_name, "not found in message type", descriptor->name());
//        return false;  // 字段未找到
//    }
//
//    // 根据字段类型设置相应的值
//    if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
//        reflection->SetString(message, field, value.cast<std::string>());
//    }
//    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT32) {
//        reflection->SetInt32(message, field, value.cast<int>());
//    }
//    // 可以继续扩展更多类型，例如 BOOL、FLOAT、DOUBLE 等
//
//    return true;
//}
//
//void send(const std::string& message_type, pybind11::kwargs kwargs) {
//    std::unique_ptr<google::protobuf::Message> message;
//
//    // 根据消息类型名称创建消息实例
//    //if (message_type == "test") {
//        // message = ;
//    //}
//    //else {
//        // throw std::runtime_error("Unsupported message type: " + message_type);
//    //}
//
//    // 遍历关键字参数并设置消息字段
//    for (auto item : kwargs) {
//        std::string field_name = item.first.cast<std::string>();
//        py::object value = item.second;
//        set_field(message.get(), field_name, value);
//    }
//
//    // 将 Protobuf 对象序列化为字节流
//    std::string serialized_data;
//    message->SerializeToString(&serialized_data);
//
//
//    // return pybind11::bytes(serialized_data);
//}
//
//PYBIND11_EMBEDDED_MODULE(million, m) {
//    m.doc() = "million core module";
//
//    // 绑定 send 函数
//    m.def("send", &send, "Create a protobuf message by type name and kwargs",
//        py::arg("message_type"));
//}

//class PyService : public million::IService {
//public:
//    using Base = IService;
//    using Base::Base;
//
//    virtual bool OnInit() override {
//        vm_ = std::make_unique<pkpy::VM>();
//        py::interpreter::initialize(vm_.get());
//
//        // ss_proto_mgr.Init();
//
//        // vm_->exec("import million");
//        // vm_->exec("print('pysvr:')");         // 1
//        //vm->exec("print(a.x)");         // 1
//        //vm->exec("print(a.y)");         // 2
//
//        // auto r = vm_->exec("def my_generator():\n    yield 1\n    yield 2\n    yield 3");
//
//        auto mod = vm_->new_module("module");
//        // mod->type();
//        // auto func = mod->attr().try_get("");
//        // vm_->call(func);
//
//        
//
//        //pkpy::PyVar obj;
//        //vm_->bind(obj, "my_generator()", [](auto vm, auto args) {
//
//        //    return vm->None;
//        //});
//
//        // vm_->call("my_generator");
//
//        // vm_->exec("a = million.send(\"test\", a = 1, b = 2)");
//
//
//        //auto a= vm_->eval();
//        //
//        //vm_->ge();
//
//        return true;
//    }
//
//    virtual void OnStop() override {
//
//    }
//
//    virtual million::Task<> OnMsg(million::MsgUnique) override {
//        co_return;
//    }
//
//
//private:
//    struct SsProtoMgrHeader { };
//    // million::CommProtoMgr<SsProtoMgrHeader> ss_proto_mgr;
//
//    std::unique_ptr<pkpy::VM> vm_;
//};


namespace million {
namespace pysvr {

void interact_with_generator() {
    py::scoped_interpreter guard{};  // 初始化 Python 解释器

    //# example.py

    //def my_generator():
    //    for i in range(5):
    //        yield i

    //auto path = sys.attr("path");
    //py::print(path);

    py::module_ example = py::module_::import("example");

    // 获取生成器函数
    py::object gen = example.attr("my_generator")();
    
    for (py::object value : gen) {
        // 转换为 C++ 类型
        int result = value.cast<int>();
        std::cout << "Generated value: " << result << std::endl;
    }

}

extern "C" MILLION_PYSVR_API  bool MillionModuleInit(IMillion* imillion) {
    interact_with_generator();

    return true;
}

} // namespace pysvr
} // namespace million