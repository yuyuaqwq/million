cmake_minimum_required(VERSION 3.10)
project(module_src)

set(GATEWAY_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pocketpy)
set(GATEWAY_LIB gateway)
set(PYSVR_LIB pysvr)

add_subdirectory(gateway)
add_subdirectory(pysvr)