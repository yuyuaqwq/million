cmake_minimum_required(VERSION 3.10)
project(${PYSVR_LIB})

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./ SRC)

add_library(${PYSVR_LIB} SHARED ${SRC})

target_include_directories(${PYSVR_LIB} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${PYSVR_LIB} PRIVATE ${MILLION_LIB})

target_include_directories(${PYSVR_LIB} PRIVATE ${POCKET_PY_HEADER_DIR})
target_link_libraries(${PYSVR_LIB} PRIVATE ${POCKET_PY_LIB})

if (WIN32)
    message(STATUS "Compiling on Windows")
    
    target_include_directories(${PYSVR_LIB} PRIVATE ${PROTOBUF_INCLUDE_DIR})
    target_link_libraries(${PYSVR_LIB} PRIVATE ${PROTOBUF_LIB})
elseif(UNIX)
    message(STATUS "Compiling on Linux")

    find_package(Protobuf REQUIRED)
    target_include_directories(${PYSVR_LIB} PRIVATE ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(${PYSVR_LIB} PRIVATE ${Protobuf_LIBRARIES})
endif()