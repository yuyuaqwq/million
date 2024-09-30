cmake_minimum_required(VERSION 3.0)
project(milinet_example)

set(CMAKE_CXX_STANDARD 20)

include_directories(${HEAD_PATH})
link_directories(${LIB_PATH})

link_libraries(${MILINET_LIB})

find_package(Threads)

set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})
add_executable(game game.cpp)

target_link_libraries (game ${CMAKE_THREAD_LIBS_INIT})