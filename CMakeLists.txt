cmake_minimum_required(VERSION 3.15)
project(million)

set(CMAKE_CXX_STANDARD 20)

# 设置 OpenSSL 目录（如果已安装）
# set(OPENSSL_ROOT_DIR "path/to/openssl")

set(MILLION_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

set(LIBRARY_OUTPUT_PATH ${MILLION_BUILD_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${MILLION_BUILD_DIR}/bin)

# add
add_subdirectory(third_party)
add_subdirectory(million)
add_subdirectory(proto)
# 确保proto生成完成后再处理protogen
add_subdirectory(protogen)
add_subdirectory(module)
add_subdirectory(test)