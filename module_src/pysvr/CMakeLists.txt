cmake_minimum_required(VERSION 3.10)
project(PYSVR_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${PYSVR_LIB} SHARED ${SRC})

target_include_directories(${PYSVR_LIB} PRIVATE ${MILLION_HEAD_PATH})
target_include_directories(${PYSVR_LIB} PRIVATE ${POCKET_PY_HEAD_PATH})

target_link_libraries(${PYSVR_LIB} ${POCKET_PY_LIB})