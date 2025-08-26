cmake_minimum_required(VERSION 3.10)
project(million)

set(CMAKE_CXX_STANDARD 20)

# 设置 MSVC 编译器的 UTF-8 选项
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 设置 OpenSSL 目录（如果已安装）
# set(OPENSSL_ROOT_DIR "path/to/openssl")

set(MILLION_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

set(LIBRARY_OUTPUT_PATH ${MILLION_BUILD_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${MILLION_BUILD_DIR}/bin)

# add
add_subdirectory(third_party)
add_subdirectory(million)
add_subdirectory(proto)
add_subdirectory(protogen)
add_subdirectory(module)
add_subdirectory(test)