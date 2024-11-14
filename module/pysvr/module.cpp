#include "iostream"

#include <google/protobuf/message.h>
#include <string>

#include <million/imillion.h>
#include <million/proto_msg.h>

#include <pocketpy/pocketpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;



namespace pybind11 {
    thread_local pkpy::VM* vm = nullptr;
}
inline static void switch_vm(pkpy::VM* vm_) {
    py::vm = vm_;
}

bool set_field(google::protobuf::Message* message, const std::string& field_name, const py::object& value) {
    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::Reflection* reflection = message->GetReflection();

    const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName(field_name);
    if (!field) {
        py::print("Field", field_name, "not found in message type", descriptor->name());
        return false;  // �ֶ�δ�ҵ�
    }

    // �����ֶ�����������Ӧ��ֵ
    if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
        reflection->SetString(message, field, value.cast<std::string>());
    }
    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT32) {
        reflection->SetInt32(message, field, value.cast<int>());
    }
    // ���Լ�����չ�������ͣ����� BOOL��FLOAT��DOUBLE ��

    return true;
}

void send(const std::string& message_type, pybind11::kwargs kwargs) {
    std::unique_ptr<google::protobuf::Message> message;

    // ������Ϣ�������ƴ�����Ϣʵ��
    if (message_type == "Person") {
        // message = ;
    }
    //else if (message_type == "Address") {
    //    message = ;
    //}
    else {
        throw std::runtime_error("Unsupported message type: " + message_type);
    }

    // �����ؼ��ֲ�����������Ϣ�ֶ�
    for (auto item : kwargs) {
        std::string field_name = item.first.cast<std::string>();
        py::object value = item.second;
        set_field(message.get(), field_name, value);
    }

    // �� Protobuf �������л�Ϊ�ֽ���
    std::string serialized_data;
    message->SerializeToString(&serialized_data);

    // return pybind11::bytes(serialized_data);
}


PYBIND11_EMBEDDED_MODULE(million, m) {
    m.doc() = "million core module";

    // �� send ����
    m.def("send", &send, "Create a protobuf message by type name and kwargs",
        py::arg("message_type"));
}

namespace million {
namespace pysvr {

struct Point {
    int x;
    int y;
};

//
//PYBIND11_EMBEDDED_MODULE(example, m) {
//    // m.doc() = "pybind11 example with dynamic Protobuf message creation";
//
//    // �� create_message ����
//    //m.def("create_message", &create_message, "Create a protobuf message by type name and kwargs",
//    //    py::arg("message_type"));
//}

struct SsProtoMgrHeader {

};

MILLION_FUNC_API bool MillionModuleInit(IMillion* imillion) {

    CommProtoMgr<SsProtoMgrHeader> ss_proto_mgr;
    ss_proto_mgr.Init();
    // ֧�ְ�full_name������Ϣ
    // �յ�py�ĵ��ú�ͨ��full_name�ҵ�ss��Ϣ����new����ͨ��field_name����set
    // Ȼ�����Ŀ��

    auto vm_ = new pkpy::VM(); // std::make_unique<pkpy::VM>
    // ���´�����vm����pybind11��ʼ��
    py::interpreter::initialize(vm_);

    // �л�����vm
    switch_vm(vm_);

    // auto guard = py::scoped_interpreter(vm_);
    // auto mod = vm->new_module("example");
    //vm->register_user_class<Point>(mod, "Point",
    //    [](auto vm, auto mod, auto type){
    //        // wrap field x
    //        vm->bind_field(type, "x", &Point::x);
    //        // wrap field y
    //        vm->bind_field(type, "y", &Point::y);

    //        // __init__ method
    //        vm->bind(type, "__init__(self, x, y, *sbsb)", [](auto vm, auto args){

    //            auto size = args.size();

    //            Point& self = _py_cast<Point&>(vm, args[0]);
    //            self.x = py_cast<int>(vm, args[1]);
    //            self.y = py_cast<int>(vm, args[2]);

    //            // auto sbsb = py_cast<>();
    //            return vm->None;
    //        });
    //    });

    // use the Point class
    vm_->exec("import million");
    vm_->exec("print('pysvr:')");         // 1
    //vm->exec("print(a.x)");         // 1
    //vm->exec("print(a.y)");         // 2
    vm_->exec("a = million.send(\"shabi\", a = 1, b = 2)");


    return true;
}

} // namespace pysvr
} // namespace million