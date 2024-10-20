#pragma once

#include <random>
#include <chrono>

namespace million {

class TokenGenerator {
public:
    TokenGenerator()
        : gen_(rd_())
        , dis_(0, std::numeric_limits<uint32_t>::max()) {}

    uint64_t Generate() {
        auto now = std::chrono::system_clock::now();
        uint32_t timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
        uint32_t random_value = dis_(gen_);
        // 64λtoken
        uint64_t token = (static_cast<uint64_t>(timestamp) << 32) | random_value;
        return token;
    }

private:
    std::random_device rd_;
    std::mt19937_64 gen_;
    std::uniform_int_distribution<uint32_t> dis_;
};


} // namespace million