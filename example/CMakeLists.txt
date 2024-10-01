cmake_minimum_required(VERSION 3.1)
project(milinet_example)

set(CMAKE_CXX_STANDARD 20)

include_directories(${HEAD_PATH})
link_directories(${LIB_PATH})

link_libraries(${MILINET_LIB})

link_libraries(netservice)

set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})
add_executable(game game.cpp)

find_package(Threads)
target_link_libraries(game ${CMAKE_THREAD_LIBS_INIT})
find_library(DL_LIBRARIES dl)
target_link_libraries(game ${DL_LIBRARIES})