cmake_minimum_required(VERSION 3.10)
project(GATEWAY_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)
aux_source_directory(./net/ SRC)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${GATEWAY_LIB} SHARED ${SRC})

target_include_directories(${GATEWAY_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${GATEWAY_LIB} PRIVATE ${MILLION_HEAD_PATH})
target_include_directories(${GATEWAY_LIB} PRIVATE ${ASIO_HEAD_PATH})

find_package(Threads)
target_link_libraries(${GATEWAY_LIB} PRIVATE Threads::Threads)

include_directories(${YAML_CPP_HEAD_PATH})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${YAML_CPP_LIB})

#find_package(Protobuf REQUIRED)
#target_link_libraries(${GATEWAY_LIB} ${PROTOBUF_LIBRARIES})