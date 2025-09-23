cmake_minimum_required(VERSION 3.15)
project(million
        VERSION 0.0.1
        DESCRIPTION "million"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置 MSVC 编译器的 UTF-8 选项
if(MSVC)
    add_compile_options(/utf-8)
endif()

set(MILLION_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

set(LIBRARY_OUTPUT_PATH ${MILLION_BUILD_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${MILLION_BUILD_DIR}/bin)

# add
add_subdirectory(third_party)
add_subdirectory(million)
add_subdirectory(proto)
add_subdirectory(module)
add_subdirectory(test)