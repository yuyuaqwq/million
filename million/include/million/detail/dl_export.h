#pragma once

#ifdef __linux__
#define MILLION_FUNC_EXPORT extern "C"
#define MILLION_CLASS_EXPORT
#define MILLION_OBJECT_IMPORT
#define MILLION_OBJECT_EXPORT __attribute__((visibility("default")))
#elif WIN32
#define MILLION_FUNC_EXPORT extern "C" __declspec(dllexport)
#define MILLION_CLASS_EXPORT __declspec(dllexport)
#define MILLION_OBJECT_IMPORT __declspec(dllimport)
#define MILLION_OBJECT_EXPORT __declspec(dllexport)
#endif