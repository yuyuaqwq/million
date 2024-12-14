cmake_minimum_required(VERSION 3.10)
project(pysvr_test)

set(CMAKE_CXX_STANDARD 20)

link_directories(${LIB_PATH})

add_executable(pysvr_test pysvr_test.cpp)

target_include_directories(pysvr_test PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(pysvr_test PRIVATE ${MILLION_LIB})

target_include_directories(pysvr_test PRIVATE ${PYSVR_HEADER_DIR})
target_link_libraries(pysvr_test PRIVATE ${PYSVR_LIB})

target_include_directories(pysvr_test PRIVATE ${MILLION_PROTOGEN_HEADER_DIR})
target_link_libraries(pysvr_test PRIVATE ${MILLION_PROTOGEN_LIB})
target_compile_definitions(pysvr_test PRIVATE ${MILLION_PROTOGEN_COMPILE_DEFINITIONS})

target_include_directories(pysvr_test PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(pysvr_test PRIVATE ${YAML_CPP_LIB})

find_package(Threads)
target_link_libraries(pysvr_test PRIVATE Threads::Threads)

if (UNIX)
    find_library(DL_LIBRARIES dl)
    target_link_libraries(pysvr_test PRIVATE ${DL_LIBRARIES})
endif()

if (WIN32)
    message(STATUS "Compiling on Windows")
    
    target_include_directories(pysvr_test PRIVATE ${PROTOBUF_INCLUDE_DIR})
    target_link_libraries(pysvr_test PRIVATE ${PROTOBUF_LIB})
elseif(UNIX)
    message(STATUS "Compiling on Linux")

    find_package(Protobuf REQUIRED)
    target_include_directories(pysvr_test PRIVATE ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(pysvr_test PRIVATE ${Protobuf_LIBRARIES})
endif()