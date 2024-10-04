#include "iostream"

#include "milinet/imilinet.hpp"
#include "milinet/imsg.hpp"

#include <pybind11/pybind11.h>
namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(example, m) {
    m.def("add", [](int a, int b) {
        return a + b;
    });

    auto math = m.def_submodule("math");
}


MILINET_FUNC_EXPORT bool MiliModuleInit(milinet::IMilinet* imilinet) {
    py_initialize();
    bool ok = py_exec("print('Hello world!')", "<string>", EXEC_MODE, NULL);

    return true;
}
