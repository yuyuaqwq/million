cmake_minimum_required(VERSION 3.10)
project(million)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(MILLION_LIB million)

set(MILLION_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/million/include)
set(ASIO_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
set(YAML_CPP_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/yaml-cpp/include)
set(YAML_CPP_LIB yaml-cpp)
set(POCKET_PY_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pocketpy)
set(POCKET_PY_LIB pocketpy)

add_subdirectory(million)
add_subdirectory(module_src)
add_subdirectory(example)

# add_definitions(-DYAML_BUILD_SHARED_LIBS=ON)
set(YAML_BUILD_SHARED_LIBS ON)
add_subdirectory(third_party/yaml-cpp)
add_subdirectory(third_party/pocketpy)