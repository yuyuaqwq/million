#pragma once

#include <cstdint>

#ifdef __linux__
#include <dlfcn.h>
#elif WIN32
#include <Windows.h>
#endif


#include <filesystem>
#include <string_view>

#include "milinet/noncopyable.h"

namespace milinet {

namespace detail {

class Dll : noncopyable {
public:
    ~Dll() {
        Unload();
    }

    bool Load(const std::filesystem::path& path) {
        Unload();
#ifdef __linux__
        handle_ = ::dlopen(path.c_str(), RTLD_LAZY);
#elif WIN32
        handle_ = ::LoadLibraryW(path.c_str());
#endif
        if (handle_ == nullptr) {
            return false;
        }
        return true;
    }

    void Unload() {
        if (!Loaded()) {
            return;
        }
#ifdef __linux__
        ::dlclose(handle_);
#elif WIN32
        ::FreeLibrary(static_cast<HMODULE>(handle_));
#endif
        handle_ = nullptr;
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
#ifdef __linux__
        auto func = ::dlsym(handle_, func_name.data());
#elif WIN32
        auto func = ::GetProcAddress(static_cast<HMODULE>(handle_), func_name.data());
#endif
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