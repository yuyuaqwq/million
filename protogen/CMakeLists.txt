cmake_minimum_required(VERSION 3.10)
project(${MILLION_PROTOGEN_LIB})

if(WIN32)
    file(GLOB_RECURSE PRIVATE_HEADERS 
        ./src/*.h
        ./src/*.hpp
        ./src/**/*.h
        ./src/**/*.hpp)
    source_group("Private Header Files" FILES ${PUBLIC_HEADERS})
    file(GLOB_RECURSE PUBLIC_HEADERS 
        ./include/*.h
        ./include/*.hpp
        ./include/**/*.h
        ./include/**/*.hpp)
    source_group("Public Header Files" FILES ${PUBLIC_HEADERS})
endif()
file(GLOB SRC
    ./src/*.cpp
    ./src/**/*.cpp
    ./src/*.cc
    ./src/**/*.cc)
add_library(${MILLION_PROTOGEN_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${MILLION_PROTOGEN_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${MILLION_PROTOGEN_LIB} PRIVATE ${MILLION_PROTOGEN_HEADER_DIR})

if (WIN32)
    target_compile_definitions(${MILLION_PROTOGEN_LIB} PRIVATE MILLION_PROTOGEN_API=__declspec\(dllexport\))
elseif(UNIX)
    target_compile_definitions(${MILLION_PROTOGEN_LIB} PRIVATE MILLION_PROTOGEN_API=__attribute__\(\(visibility\(\"default\"\)\)\))
 endif()

target_include_directories(${MILLION_PROTOGEN_LIB} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${MILLION_PROTOGEN_LIB} PRIVATE ${PROTOBUF_LIB})
