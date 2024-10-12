cmake_minimum_required(VERSION 3.10)
project(PYSVR_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)

include_directories(${HEAD_PATH})
include_directories(${POCKET_PY_HEAD_PATH})

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${PYSVR_LIB} SHARED ${SRC})

target_link_libraries(${PYSVR_LIB} ${POCKET_PY_LIB})