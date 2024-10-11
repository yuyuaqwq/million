cmake_minimum_required(VERSION 3.10)
project(milinet)

set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(MILINET_LIB milinet)
set(SERVICE_LIB service)

set(ASIO_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
set(YAML_CPP_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/yaml-cpp/include)
set(YAML_CPP_LIB yaml-cpp)
set(QUICK_JS_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs)
set(QUICK_JS_LIB quickjs)

add_subdirectory(milinet)
add_subdirectory(module_src)
add_subdirectory(example)

# add_definitions(-DYAML_BUILD_SHARED_LIBS=ON)
set(YAML_BUILD_SHARED_LIBS ON)
add_subdirectory(third_party/yaml-cpp)
# add_subdirectory(third_party/quickjs)