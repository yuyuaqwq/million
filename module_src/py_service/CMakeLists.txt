cmake_minimum_required(VERSION 3.1)
project(PY_SERVICE_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)

include_directories(${HEAD_PATH})
include_directories(${POCKET_PY_HEAD_PATH})

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${PY_SERVICE_LIB} SHARED ${SRC})

target_link_libraries(${PY_SERVICE_LIB} ${POCKET_PY_LIB})

find_package(Threads)
target_link_libraries(${PY_SERVICE_LIB} ${CMAKE_THREAD_LIBS_INIT})