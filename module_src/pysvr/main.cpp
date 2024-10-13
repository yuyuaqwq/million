#include "iostream"

#include "million/imillion.h"
#include "million/imsg.h"

#include <pocketpy/pocketpy.h>

struct Point {
    int x;
    int y;
};

MILLION_FUNC_EXPORT bool MiliModuleInit(million::IMillion* imillion) {
    auto vm = std::make_unique<pkpy::VM>();
    auto mod = vm->new_module("test");
    vm->register_user_class<Point>(mod, "Point",
        [](auto vm, auto mod, auto type){
            // wrap field x
            vm->bind_field(type, "x", &Point::x);
            // wrap field y
            vm->bind_field(type, "y", &Point::y);

            // __init__ method
            vm->bind(type, "__init__(self, x, y)", [](auto vm, auto args){
                Point& self = _py_cast<Point&>(vm, args[0]);
                self.x = py_cast<int>(vm, args[1]);
                self.y = py_cast<int>(vm, args[2]);
                return vm->None;
            });
        });

    // use the Point class
    vm->exec("import test");
    vm->exec("a = test.Point(1, 2)");
    vm->exec("print(a.x)");         // 1
    vm->exec("print(a.y)");         // 2

    return true;
}
