#pragma once


#ifdef million_EXPORTS
    #ifdef __linux__
        #define MILLION_FUNC_API extern "C" __attribute__((visibility("default")))
        #define MILLION_CLASS_API __attribute__((visibility("default")))
        #define MILLION_OBJECT_API __attribute__((visibility("default")))
    #elif defined(_WIN32) || defined(__WIN32__)
        #define MILLION_FUNC_API extern "C" __declspec(dllexport)
        #define MILLION_CLASS_API __declspec(dllexport)
        #define MILLION_OBJECT_API __declspec(dllexport)
    #else
        #error "Unsupported platform"
    #endif
#else
    #ifdef __linux__
        #define MILLION_FUNC_API extern "C" __attribute__((visibility("hidden")))
        #define MILLION_CLASS_API __attribute__((visibility("hidden")))
        #define MILLION_OBJECT_API __attribute__((visibility("hidden")))
    #elif defined(_WIN32) || defined(__WIN32__)
        #define MILLION_FUNC_API extern "C" __declspec(dllexport)
        #define MILLION_CLASS_API __declspec(dllimport)
        #define MILLION_OBJECT_API __declspec(dllimport)
    #else
        #error "Unsupported platform"
    #endif
#endif