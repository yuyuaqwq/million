cmake_minimum_required(VERSION 3.10)
project(${PYSVR_LIB})

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    file(GLOB_RECURSE PRIVATE_HEADERS 
        ./src/*.h
        ./src/*.hpp
        ./src/**/*.h
        ./src/**/*.hpp)
    source_group("Private Header Files" FILES ${PRIVATE_HEADERS})
    
    file(GLOB_RECURSE PUBLIC_HEADERS 
        ./include/*.h
        ./include/*.hpp
        ./include/**/*.h
        ./include/**/*.hpp)
    source_group("Public Header Files" FILES ${PUBLIC_HEADERS})
endif()
file(GLOB SRC
    ./src/*.cpp
    ./src/**/*.cpp)
add_library(${PYSVR_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${PYSVR_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${PYSVR_LIB} PRIVATE ${PYSVR_HEADER_DIR})

target_include_directories(${PYSVR_LIB} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${PYSVR_LIB} PRIVATE ${MILLION_LIB})

target_include_directories(${PYSVR_LIB} PRIVATE ${POCKET_PY_HEADER_DIR})
target_link_libraries(${PYSVR_LIB} PRIVATE ${POCKET_PY_LIB})

target_include_directories(${PYSVR_LIB} PRIVATE ${QUICK_JS_HEADER_DIR})
target_link_libraries(${PYSVR_LIB} PRIVATE ${QUICK_JS_LIB})

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