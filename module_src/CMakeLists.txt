cmake_minimum_required(VERSION 3.1)
project(SERVICE_LIB)

set(NET_SERVICE_LIB netservice)
set(JS_SERVICE_LIB jsservice)

add_subdirectory(net_service)
# add_subdirectory(js_service)