cmake_minimum_required(VERSION 3.10)
project(${GATEWAY_LIB})

set(CMAKE_CXX_STANDARD 20)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

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
target_include_directories(${GATEWAY_LIB} PRIVATE ${GATEWAY_HEADER})

target_include_directories(${GATEWAY_LIB} PRIVATE ${MILLION_HEADER})
target_include_directories(${GATEWAY_LIB} PRIVATE ${ASIO_HEADER})

target_include_directories(${GATEWAY_LIB} PRIVATE ${PROTOGEN_HEADER})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${PROTOGEN_LIB})

find_package(Threads)
target_link_libraries(${GATEWAY_LIB} PRIVATE Threads::Threads)

target_include_directories(${GATEWAY_LIB} PRIVATE ${YAML_CPP_HEADER})
target_link_libraries(${GATEWAY_LIB} PRIVATE ${YAML_CPP_LIB})

if (WIN32)
    message(STATUS "Compiling on Windows")
    
    # cmake -DCMAKE_TOOLCHAIN_FILE=<vcpkg_path>/scripts/buildsystems/vcpkg.cmake -Dprotobuf_DIR=<vcpkg_path>/installed/x64-windows/share/protobuf ..
    find_package(protobuf CONFIG REQUIRED)
    target_link_libraries(${GATEWAY_LIB} PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)

elseif(UNIX)
    message(STATUS "Compiling on Linux")

    find_package(Protobuf REQUIRED)
    target_include_directories(${GATEWAY_LIB} PRIVATE ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(${GATEWAY_LIB} PRIVATE ${Protobuf_LIBRARIES})
endif()