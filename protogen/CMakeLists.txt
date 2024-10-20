cmake_minimum_required(VERSION 3.10)
project(${PROTOGEN_LIB})

aux_source_directory(./comm/ SRC)
aux_source_directory(./ss/ SRC)
aux_source_directory(./cs/ SRC)
aux_source_directory(./db/ SRC)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

# 无法生成在windows下会动态导出类的pb.h，选择静态库
add_library(${PROTOGEN_LIB} STATIC ${SRC})

target_include_directories(${PROTOGEN_LIB} PRIVATE ${PROTOGEN_HEADER})

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