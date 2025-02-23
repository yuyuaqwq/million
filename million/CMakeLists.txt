set(MILLION_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/million/include;${CMAKE_CURRENT_SOURCE_DIR};${META_HEADER_DIR}" CACHE STRING "million header dir")
set(MILLION_LIB million CACHE STRING "million lib")

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

add_library(${MILLION_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${MILLION_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${MILLION_LIB} PRIVATE ${MILLION_HEADER_DIR})

target_include_directories(${MILLION_LIB} PRIVATE ${PROTOGEN_HEADER_DIR})
target_link_libraries(${MILLION_LIB} PRIVATE ${PROTOGEN_LIB})
target_compile_definitions(${MILLION_LIB} PRIVATE ${PROTOGEN_COMPILE_DEFINITIONS})

target_include_directories(${MILLION_LIB} PRIVATE ${SPDLOG_HEADER_DIR})
target_link_libraries(${MILLION_LIB} PRIVATE ${SPDLOG_LIB})

target_include_directories(${MILLION_LIB} PRIVATE ${ASIO_HEADER_DIR})

find_package(Threads)
target_link_libraries(${MILLION_LIB} PRIVATE Threads::Threads)

target_include_directories(${MILLION_LIB} PRIVATE ${ASIO_HEADER_DIR})

target_include_directories(${MILLION_LIB} PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(${MILLION_LIB} PRIVATE ${YAML_CPP_LIB})

target_include_directories(${MILLION_LIB} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${MILLION_LIB} PRIVATE ${PROTOBUF_LIB})