cmake_minimum_required(VERSION 3.10)
project(example)

set(CMAKE_CXX_STANDARD 20)

link_directories(${LIB_PATH})

set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})
add_executable(game game.cpp)

target_include_directories(game PRIVATE ${MILLION_HEADER})

target_link_libraries(game PRIVATE ${MILLION_LIB})

find_package(Threads)
target_link_libraries(game PRIVATE Threads::Threads)

if (UNIX)
    find_library(DL_LIBRARIES dl)
    target_link_libraries(game PRIVATE ${DL_LIBRARIES})
endif()