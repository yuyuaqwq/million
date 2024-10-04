cmake_minimum_required(VERSION 3.1)
project(SERVICE_LIB)

set(NET_SERVICE_LIB netservice)
set(PY_SERVICE_LIB pyservice)

add_subdirectory(net_service)
add_subdirectory(py_service)