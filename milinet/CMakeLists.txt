cmake_minimum_required(VERSION 3.10)
project(MILINET_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)
aux_source_directory(./detail/ SRC)
aux_source_directory(./net/ SRC)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${MILINET_LIB} SHARED ${SRC})

target_include_directories(${MILINET_LIB} PRIVATE ${HEAD_PATH})

target_include_directories(${MILINET_LIB} PRIVATE ${ROOT_PATH})

find_package(Threads)
target_link_libraries(${MILINET_LIB} PRIVATE Threads::Threads)

target_include_directories(${MILINET_LIB} PRIVATE ${ASIO_HEAD_PATH})

target_include_directories(${MILINET_LIB} PRIVATE ${YAML_CPP_HEAD_PATH})
target_link_libraries(${MILINET_LIB} PRIVATE ${YAML_CPP_LIB})