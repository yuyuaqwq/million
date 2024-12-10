cmake_minimum_required(VERSION 3.10)
project(${PROTOGEN_LIB})

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

if(SRC)
    add_library(${PROTOGEN_LIB} SHARED ${SRC})
    target_include_directories(${PROTOGEN_LIB} PRIVATE ${PROTOGEN_HEADER_DIR})
    
    if (WIN32)
        message(STATUS "Compiling on Windows")
        
        target_compile_definitions(${PROTOGEN_LIB} PRIVATE PROTOGEN_CLASS_API=__declspec\(dllexport\))
        set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_CLASS_API=__declspec\(dllimport\) CACHE STRING "protobuf compile definitions")

        target_include_directories(${PROTOGEN_LIB} PRIVATE ${PROTOBUF_INCLUDE_DIR})
        target_link_libraries(${PROTOGEN_LIB} PRIVATE ${PROTOBUF_LIB})
        
    elseif(UNIX)
        message(STATUS "Compiling on Linux")

        find_package(Protobuf REQUIRED)
        
        target_compile_definitions(${PROTOGEN_LIB} PRIVATE PROTOGEN_CLASS_API=__attribute__\(\(visibility\(\"default\"\)\)\))
        set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_CLASS_API=__attribute__\(\(visibility\(\"hidden\"\)\)\) CACHE STRING "protobuf compile definitions")

        target_include_directories(${PROTOGEN_LIB} PRIVATE ${Protobuf_INCLUDE_DIRS})
        target_link_libraries(${PROTOGEN_LIB} PRIVATE ${Protobuf_LIBRARIES})
    endif()
else()
    message(WARNING "No source files found in ${PROTOGEN_LIB}")
endif()