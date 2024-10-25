cmake_minimum_required(VERSION 3.10)
project(${DESCGEN_LIB})

if(WIN32)
    file(GLOB_RECURSE PRIVATE_HEADERS 
        ./src/*.h
        ./src/*.hpp
        ./src/**/*.h
        ./src/**/*.hpp)
    source_group("Private Header Files" FILES ${PRIVATE_HEADERS})
    
    file(GLOB_RECURSE PUBLIC_HEADERS 
        ./include/*.h
        ./include/*.hpp
        ./include/**/*.h
        ./include/**/*.hpp)
    source_group("Public Header Files" FILES ${PUBLIC_HEADERS})
endif()
file(GLOB SRC
    ./src/*.cpp
    ./src/**/*.cpp)

# 只在 SRC 列表不为空时添加库
if(SRC)
    add_library(${DESCGEN_LIB} STATIC ${SRC})
    target_include_directories(${DESCGEN_LIB} PRIVATE ${DESCGEN_HEADER_DIR})
else()
    message(WARNING "No source files found in ${DESCGEN_LIB}")
endif()