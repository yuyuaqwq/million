#pragma once

#ifdef __linux__
#define MILLION_FUNC_EXPORT extern "C"
#define MILLION_CLASS_EXPORT
#elif WIN32
#define MILLION_FUNC_EXPORT extern "C" __declspec(dllexport)
#define MILLION_CLASS_EXPORT __declspec(dllexport)
#endif