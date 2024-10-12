#pragma once

#include <cstdint>

#include <filesystem>
#include <string_view>

#include "million/detail/noncopyable.h"

namespace million {

namespace detail {

class Dll : noncopyable {
public:
    Dll();
    ~Dll();

    bool Load(const std::filesystem::path& path);
    void Unload();
    bool Loaded() const;

    template <typename FuncPtrT>
    FuncPtrT GetFuncPtr(std::string_view func_name) const {
        auto func_addr = GetFuncAddr(func_name);
        FuncPtrT func = reinterpret_cast<FuncPtrT>(func_addr);
        return func;
    }

    template <typename FuncT>
    auto GetFunc(std::string_view func_name) const {
        auto func_addr = GetFuncAddr(func_name);
        using FuncPtrType = typename std::add_pointer<FuncT>::type;
        auto func = reinterpret_cast<FuncPtrType>(func_addr);
        return func;
    }

private:
    void* GetFuncAddr(std::string_view func_name) const;

    template <typename RetT, typename ...Args>
    RetT Call(void* func_addr, Args&&... args) const {
        if (!func_addr) {
            return {};
        }
        using FuncPtrType = RetT(*)(Args...);
        auto func = reinterpret_cast<FuncPtrType>(func_addr);
        return func(std::forward<Args>(args)...);
    }

private:
    void* handle_ = nullptr;
};

} // namespace detail

} // namespace million