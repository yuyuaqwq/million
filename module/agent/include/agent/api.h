#pragma once

#ifdef agent_EXPORTS
    #ifdef __linux__
        #define MILLION_AGENT_API __attribute__((visibility("default")))
    #elif defined(_WIN32) || defined(__WIN32__)
        #define MILLION_AGENT_API __declspec(dllexport)
    #else
        #error "Unsupported platform"
    #endif
#else
    #ifdef __linux__
        #define MILLION_AGENT_API __attribute__((visibility("hidden")))
    #elif defined(_WIN32) || defined(__WIN32__)
        #define MILLION_AGENT_API __declspec(dllimport)
    #else
        #error "Unsupported platform"
    #endif
#endif
