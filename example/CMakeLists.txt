cmake_minimum_required(VERSION 3.10)
project(example)

set(CMAKE_CXX_STANDARD 20)

link_directories(${LIB_PATH})

add_executable(example example.cpp)

target_include_directories(example PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(example PRIVATE ${MILLION_LIB})

find_package(Threads)
target_link_libraries(example PRIVATE Threads::Threads)

if (UNIX)
    find_library(DL_LIBRARIES dl)
    target_link_libraries(example PRIVATE ${DL_LIBRARIES})
endif()