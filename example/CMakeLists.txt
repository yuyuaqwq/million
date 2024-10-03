cmake_minimum_required(VERSION 3.1)
project(milinet_example)

set(CMAKE_CXX_STANDARD 20)

include_directories(${HEAD_PATH})
link_directories(${LIB_PATH})

set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})
add_executable(game game.cpp)

target_link_libraries(game PRIVATE ${MILINET_LIB})

find_package(Threads)
target_link_libraries(game PRIVATE Threads::Threads)

if (UNIX)
    find_library(DL_LIBRARIES dl)
    target_link_libraries(game PRIVATE ${DL_LIBRARIES})
endif()