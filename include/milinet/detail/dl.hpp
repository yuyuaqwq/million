#pragma once

#include <cstdint>

#include <dlfcn.h>

#include <filesystem>
#include <string_view>

#include "milinet/noncopyable.h"

namespace milinet {

namespace detail {

class Dll : noncopyable {
public:
    ~Dll() {
        if (handle_) {
            ::dlclose(handle_);
            handle_ = nullptr;
        }
    }

    bool Load(std::filesystem::path path) {
        handle_ = ::dlopen(path.c_str(), RTLD_LAZY);
        if (handle_ == nullptr) {
            return false;
        }
        return true;
    }

    bool Loaded() const {
        return handle_;
    }

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
    void* GetFuncAddr(std::string_view func_name) const {
        if (handle_ == nullptr) {
            return nullptr;
        }
        auto func = ::dlsym(handle_, func_name.data());
        if (func == nullptr) {
            return nullptr;
        }
        return func;
    }

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

} // namespace milinet