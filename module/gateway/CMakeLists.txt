cmake_minimum_required(VERSION 3.10)
project(${GATEWAY_LIB})

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
add_library(${GATEWAY_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${GATEWAY_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${GATEWAY_LIB} PRIVATE ${GATEWAY_HEADER_DIR})

target_include_directories(${GATEWAY_LIB} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${MILLION_LIB})

target_include_directories(${GATEWAY_LIB} PRIVATE ${ASIO_HEADER_DIR})

target_compile_definitions(${GATEWAY_LIB} PRIVATE ${MILLION_PROTOGEN_COMPILE_DEFINITIONS})
target_include_directories(${GATEWAY_LIB} PRIVATE ${MILLION_PROTOGEN_HEADER_DIR})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${MILLION_PROTOGEN_LIB})

find_package(Threads)
target_link_libraries(${GATEWAY_LIB} PRIVATE Threads::Threads)

target_include_directories(${GATEWAY_LIB} PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${YAML_CPP_LIB})

target_include_directories(${GATEWAY_LIB} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${PROTOBUF_LIB})