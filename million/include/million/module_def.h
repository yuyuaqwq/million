#pragma once

#include <cstdint>
#include <cassert>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

namespace million {

using ModuleCode = uint32_t;
constexpr ModuleCode kModuleCodeInvalid = 0;

using ModuleId = uint16_t;
constexpr ModuleId kModuleIdInvalid = 0;

using ModuleSubCode = uint16_t;
constexpr ModuleSubCode kModuleSubCodeInvalid = 0;

template<typename ModuleIdT, typename SubCodeT>
ModuleCode EncodeModuleCode(ModuleIdT module_id, SubCodeT sub_code) {
    assert(module_id != kModuleIdInvalid);
    assert(sub_code != kModuleSubCodeInvalid);
    assert(module_id <= std::numeric_limits<ModuleId>::max());
    assert(sub_code <= std::numeric_limits<ModuleSubCode>::max());
    return (static_cast<ModuleCode>(module_id) << 16) | static_cast<ModuleCode>(sub_code & 0xffff);
}

template<typename ModuleExtIdT, typename SubCodeT>
ModuleCode EncodeModuleCode(ModuleExtIdT module_ext_id, const google::protobuf::EnumDescriptor* enum_descriptor, SubCodeT sub_code) {
    if (sub_code > std::numeric_limits<ModuleSubCode>::max()) {
        return kModuleCodeInvalid;
    }

    auto& options = enum_descriptor->options();
    auto module_id = options.GetExtension(module_ext_id);
    auto module_id_ui = static_cast<ModuleId>(module_id);
    if (module_id_ui > std::numeric_limits<ModuleId>::max()) {
        return kModuleCodeInvalid;
    }

    return EncodeModuleCode(module_id_ui, sub_code);
}

template<typename ModuleIdT, typename SubCodeT>
std::pair<ModuleIdT, SubCodeT> DecodeModuleCode(ModuleCode module_code) {
    return std::make_pair(ModuleIdT{ static_cast<ModuleId>(module_code >> 16) }, SubCodeT{ static_cast<ModuleSubCode>(module_code & 0xffff) });
}

} // namespace million