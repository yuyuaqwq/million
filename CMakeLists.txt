cmake_minimum_required(VERSION 3.10)
project(milinet)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(MILINET_LIB milinet)
set(SERVICE_LIB service)

set(ASIO_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
set(YAML_CPP_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/yaml-cpp/include)
set(YAML_CPP_LIB yaml-cpp)

add_subdirectory(src)
add_subdirectory(module_src)
add_subdirectory(example)

add_subdirectory(third_party/yaml-cpp)