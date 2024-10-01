cmake_minimum_required(VERSION 3.1)
project(milinet)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(ASIO_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)

set(MILINET_LIB milinet)
set(SERVICE_LIB service)

add_subdirectory(src)
add_subdirectory(service_src)
add_subdirectory(example)