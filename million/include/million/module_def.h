#pragma once

#include <cstdint>
#include <cassert>

namespace million {

using ModuleCode = uint32_t;
constexpr ModuleCode kModuleCodeInvalid = 0;

using ModuleId = uint16_t;
constexpr ModuleId kModuleIdInvalid = 0;

using ModuleSubCode = uint16_t;
constexpr ModuleSubCode kModuleSubCodeInvalid = 0;

template<typename ModuleIdT, typename SubCodeT>
ModuleCode EncodeModuleCode(ModuleIdT module_id, SubCodeT sub_code) {
    assert(module_id <= std::numeric_limits<ModuleId>::max());
    assert(sub_code <= std::numeric_limits<ModuleSubCode>::max());
    return (static_cast<ModuleCode>(module_id) << 16) | static_cast<ModuleCode>(sub_code);
}

template<typename ModuleIdT, typename SubCodeT>
std::pair<ModuleIdT, SubCodeT> DecodeModuleCode(ModuleCode module_code) {
    return std::make_pair(ModuleIdT{ static_cast<ModuleId>(module_code >> 16) }, SubCodeT{ static_cast<ModuleSubCode>(module_code & 0xffff) });
}

} // namespace million