cmake_minimum_required(VERSION 3.10)
project(module_src)

set(GATEWAY_LIB gateway)
set(PYSVR_LIB pysvr)

add_subdirectory(gateway)
# add_subdirectory(pysvr)