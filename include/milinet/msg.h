#pragma once

namespace milinet {

class Msg {
public:
    virtual ~Msg() = default;
private:
    int test = 0;
};

} // namespace milinet