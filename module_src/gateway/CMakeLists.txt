cmake_minimum_required(VERSION 3.10)
project(GATEWAY_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)

include_directories(${HEAD_PATH})
include_directories(${ASIO_HEAD_PATH})

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${GATEWAY_LIB} SHARED ${SRC})

find_package(Threads)
target_link_libraries(${GATEWAY_LIB} PRIVATE Threads::Threads)

include_directories(${YAML_CPP_HEAD_PATH})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${YAML_CPP_LIB})

#find_package(Protobuf REQUIRED)
#target_link_libraries(${GATEWAY_LIB} ${PROTOBUF_LIBRARIES})