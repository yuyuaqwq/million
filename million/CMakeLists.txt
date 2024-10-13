cmake_minimum_required(VERSION 3.10)
project(MILLION_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)
aux_source_directory(./detail/ SRC)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${MILLION_LIB} SHARED ${SRC})

target_include_directories(${MILLION_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(${MILLION_LIB} PRIVATE ${MILLION_HEADER})

find_package(Threads)
target_link_libraries(${MILLION_LIB} PRIVATE Threads::Threads)

target_include_directories(${MILLION_LIB} PRIVATE ${ASIO_HEADER})

target_include_directories(${MILLION_LIB} PRIVATE ${YAML_CPP_HEADER})
target_link_libraries(${MILLION_LIB} PRIVATE ${YAML_CPP_LIB})