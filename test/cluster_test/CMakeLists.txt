cmake_minimum_required(VERSION 3.10)
project(cluster_test)

set(CMAKE_CXX_STANDARD 20)

link_directories(${LIB_PATH})

add_executable(cluster_test cluster_test.cpp)

target_include_directories(cluster_test PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(cluster_test PRIVATE ${MILLION_LIB})

target_include_directories(cluster_test PRIVATE ${CLUSTER_HEADER_DIR})
target_link_libraries(cluster_test PRIVATE ${CLUSTER_LIB})

target_compile_definitions(cluster_test PRIVATE ${MILLION_PROTOGEN_COMPILE_DEFINITIONS})
target_include_directories(cluster_test PRIVATE ${MILLION_PROTOGEN_HEADER_DIR})
target_link_libraries(cluster_test PRIVATE ${MILLION_PROTOGEN_LIB})

target_include_directories(cluster_test PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(cluster_test PRIVATE ${YAML_CPP_LIB})

find_package(Threads)
target_link_libraries(cluster_test PRIVATE Threads::Threads)

if (UNIX)
    find_library(DL_LIBRARIES dl)
    target_link_libraries(cluster_test PRIVATE ${DL_LIBRARIES})
endif()

target_include_directories(cluster_test PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(cluster_test PRIVATE ${PROTOBUF_LIB})