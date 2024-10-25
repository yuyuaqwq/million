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
    ./**/*.cpp)

if(SRC)
    # 无法生成在windows下会动态导出类的pb.h，选择静态库
    add_library(${PROTOGEN_LIB} STATIC ${SRC})
    target_include_directories(${PROTOGEN_LIB} PRIVATE ${PROTOGEN_HEADER_DIR})
    
    if (WIN32)
        message(STATUS "Compiling on Windows")
        
        find_package(protobuf CONFIG REQUIRED)
        target_link_libraries(${PROTOGEN_LIB} PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)
    elseif(UNIX)
        message(STATUS "Compiling on Linux")

        find_package(Protobuf REQUIRED)
        target_include_directories(${PROTOGEN_LIB} PRIVATE ${Protobuf_INCLUDE_DIRS})
        target_link_libraries(${PROTOGEN_LIB} PRIVATE ${Protobuf_LIBRARIES})
    endif()
else()
    message(WARNING "No source files found in ${PROTOGEN_LIB}")
endif()