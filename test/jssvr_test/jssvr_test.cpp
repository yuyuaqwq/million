#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

MILLION_MODULE_INIT();

class TestApp : public million::IMillion {
    //using Base = million::IMillion;
    //using Base::Base;
};

int main() {
    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Start("jssvr_test_config.yaml")) {
        return 0;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10000));

    return 0;
}
