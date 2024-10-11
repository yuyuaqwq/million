#pragma once

#ifdef __linux__
#define MILINET_FUNC_EXPORT extern "C"
#define MILINET_CLASS_EXPORT
#elif WIN32
#define MILINET_FUNC_EXPORT extern "C" __declspec(dllexport)
#define MILINET_CLASS_EXPORT __declspec(dllexport)
#endif