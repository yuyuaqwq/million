cmake_minimum_required(VERSION 3.10)
project(jssvr_test)

set(CMAKE_CXX_STANDARD 20)

link_directories(${LIB_PATH})

add_executable(jssvr_test jssvr_test.cpp)

target_include_directories(jssvr_test PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(jssvr_test PRIVATE ${MILLION_LIB})

target_include_directories(jssvr_test PRIVATE ${JSSVR_HEADER_DIR})
target_link_libraries(jssvr_test PRIVATE ${JSSVR_LIB})

target_include_directories(jssvr_test PRIVATE ${PROTOGEN_HEADER_DIR})
target_link_libraries(jssvr_test PRIVATE ${PROTOGEN_LIB})
target_compile_definitions(jssvr_test PRIVATE ${PROTOGEN_COMPILE_DEFINITIONS})

target_include_directories(jssvr_test PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(jssvr_test PRIVATE ${YAML_CPP_LIB})

find_package(Threads)
target_link_libraries(jssvr_test PRIVATE Threads::Threads)

if (UNIX)
    find_library(DL_LIBRARIES dl)
    target_link_libraries(jssvr_test PRIVATE ${DL_LIBRARIES})
endif()

target_include_directories(jssvr_test PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(jssvr_test PRIVATE ${PROTOBUF_LIB})