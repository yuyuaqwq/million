cmake_minimum_required(VERSION 3.10)
project(${DESCGEN_LIB})

aux_source_directory(./src/ SRC)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

# 只在 SRC 列表不为空时添加库
if(SRC)
    add_library(${DESCGEN_LIB} STATIC ${SRC})
    target_include_directories(${DESCGEN_LIB} PRIVATE ${DESCGEN_HEADER})
else()
    message(WARNING "No source files found in ${DESCGEN_LIB}")
endif()