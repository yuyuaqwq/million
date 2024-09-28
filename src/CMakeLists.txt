cmake_minimum_required(VERSION 3.1)
project(milinet_lib)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)
include_directories(${HEAD_PATH})

find_package (Threads)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${MILINET_LIB} STATIC ${SRC})
target_link_libraries (${MILINET_LIB} ${CMAKE_THREAD_LIBS_INIT})