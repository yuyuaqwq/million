cmake_minimum_required(VERSION 3.10)
project(million)

set(CMAKE_CXX_STANDARD 20)

set(MILLION_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

set(LIBRARY_OUTPUT_PATH ${MILLION_BUILD_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${MILLION_BUILD_DIR}/bin)

# add
add_subdirectory(third_party)
add_subdirectory(protogen)
add_subdirectory(million)
add_subdirectory(module)
add_subdirectory(test)