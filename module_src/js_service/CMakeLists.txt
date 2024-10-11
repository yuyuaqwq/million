cmake_minimum_required(VERSION 3.10)
project(JS_SERVICE_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)

include_directories(${HEAD_PATH})
include_directories(${QUICK_JS_HEAD_PATH})

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${JS_SERVICE_LIB} SHARED ${SRC})

target_link_libraries(${JS_SERVICE_LIB} ${QUICK_JS_LIB})