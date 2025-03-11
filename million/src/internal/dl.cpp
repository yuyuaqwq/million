#include "internal/dl.h"

#ifdef __linux__
#include <dlfcn.h>
#elif WIN32
#include <Windows.h>
#endif

namespace million {
namespace internal {

Dll::Dll() = default;

Dll::~Dll() {
    Unload();
}

bool Dll::Load(const std::filesystem::path& path) {
    Unload();
#ifdef __linux__
    handle_ = ::dlopen(path.c_str(), RTLD_LAZY);
#elif WIN32
    handle_ = ::LoadLibraryW(path.c_str());
#endif
    if (handle_ == nullptr) {
#ifdef WIN32
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
#endif
        return false;
    }
    return true;
}

void Dll::Unload() {
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

bool Dll::Loaded() const {
    return handle_;
}

void* Dll::GetFuncAddr(std::string_view func_name) const {
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

} // namespace internal
} // namespace million