cmake_minimum_required(VERSION 3.1)
project(MILINET_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)

include_directories(${HEAD_PATH})

include_directories(${ASIO_HEAD_PATH})
include_directories(${YAML_CPP_HEAD_PATH})

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${MILINET_LIB} STATIC ${SRC})

find_package(Threads)
target_link_libraries(${MILINET_LIB} PRIVATE Threads::Threads)

target_link_libraries(${MILINET_LIB} PRIVATE ${YAML_CPP_LIB})