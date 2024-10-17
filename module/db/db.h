#include <iostream>
#include <queue>
#include <any>

#include "million/imillion.h"
#include "million/imsg.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

class DbService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    virtual void OnInit() override {
        
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        co_return;
    }

    virtual void OnExit() override {
        // Close();
    }

    void Query() {

    }

private:

};
