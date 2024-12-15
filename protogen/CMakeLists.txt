cmake_minimum_required(VERSION 3.10)
project(${MILLION_PROTOGEN_LIB})

if(WIN32)
    file(GLOB_RECURSE PUBLIC_HEADERS 
        ./*.h
        ./*.hpp
        ./**/*.h
        ./**/*.hpp)
    source_group("Header Files" FILES ${PUBLIC_HEADERS})
endif()
file(GLOB SRC
    ./*.cpp
    ./**/*.cpp
    ./*.cc
    ./**/*.cc)

add_library(${MILLION_PROTOGEN_LIB} SHARED ${SRC})
target_include_directories(${MILLION_PROTOGEN_LIB} PRIVATE ${MILLION_PROTOGEN_HEADER_DIR})

if (WIN32)
    target_compile_definitions(${MILLION_PROTOGEN_LIB} PRIVATE MILLION_PROTOGEN_API=__declspec\(dllexport\))
elseif(UNIX)
    target_compile_definitions(${MILLION_PROTOGEN_LIB} PRIVATE MILLION_PROTOGEN_API=__attribute__\(\(visibility\(\"default\"\)\)\))
 endif()

target_include_directories(${MILLION_PROTOGEN_LIB} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${MILLION_PROTOGEN_LIB} PRIVATE ${PROTOBUF_LIB})
