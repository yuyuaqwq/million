cmake_minimum_required(VERSION 3.10)
project(GATEWAY_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)
aux_source_directory(./net/ SRC)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${GATEWAY_LIB} SHARED ${SRC})

target_include_directories(${GATEWAY_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${GATEWAY_LIB} PRIVATE ${MILLION_HEADER})
target_include_directories(${GATEWAY_LIB} PRIVATE ${ASIO_HEADER})

target_include_directories(${GATEWAY_LIB} PRIVATE ${PROTOGEN_HEADER})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${PROTOGEN_LIB})

find_package(Threads)
target_link_libraries(${GATEWAY_LIB} PRIVATE Threads::Threads)

target_include_directories(${GATEWAY_LIB} PRIVATE ${YAML_CPP_HEADER})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${YAML_CPP_LIB})

if (WIN32)
    message(STATUS "Compiling on Windows")
    
    # cmake -DCMAKE_TOOLCHAIN_FILE=<vcpkg_path>/scripts/buildsystems/vcpkg.cmake -Dprotobuf_DIR=<vcpkg_path>/installed/x64-windows/share/protobuf ..
    find_package(protobuf CONFIG REQUIRED)
    target_link_libraries(${GATEWAY_LIB} PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)

elseif(UNIX)
    message(STATUS "Compiling on Linux")

    find_package(Protobuf REQUIRED)
    target_include_directories(${GATEWAY_LIB} PRIVATE ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(${GATEWAY_LIB} PRIVATE ${Protobuf_LIBRARIES})
endif()