set(AGENT_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/agent/include CACHE STRING "agent header dir")
set(AGENT_LIB agent CACHE STRING "agent lib")

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
add_library(${AGENT_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${AGENT_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${AGENT_LIB} PRIVATE ${AGENT_HEADER_DIR})

target_include_directories(${AGENT_LIB} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${AGENT_LIB} PRIVATE ${MILLION_LIB})

target_include_directories(${AGENT_LIB} PRIVATE ${GATEWAY_HEADER_DIR})
target_link_libraries(${AGENT_LIB} PRIVATE ${GATEWAY_LIB})

target_include_directories(${AGENT_LIB} PRIVATE ${ASIO_HEADER_DIR})

target_compile_definitions(${AGENT_LIB} PRIVATE ${PROTOGEN_COMPILE_DEFINITIONS})
target_include_directories(${AGENT_LIB} PRIVATE ${PROTOGEN_HEADER_DIR})
target_link_libraries(${AGENT_LIB} PRIVATE ${PROTOGEN_LIB})

find_package(Threads)
target_link_libraries(${AGENT_LIB} PRIVATE Threads::Threads)

target_include_directories(${AGENT_LIB} PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(${AGENT_LIB} PRIVATE ${YAML_CPP_LIB})

if (WIN32)
    message(STATUS "Compiling on Windows")
    
    target_include_directories(${AGENT_LIB} PRIVATE ${PROTOBUF_INCLUDE_DIR})
    target_link_libraries(${AGENT_LIB} PRIVATE ${PROTOBUF_LIB})
elseif(UNIX)
    message(STATUS "Compiling on Linux")

    find_package(Protobuf REQUIRED)
    target_include_directories(${AGENT_LIB} PRIVATE ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(${AGENT_LIB} PRIVATE ${Protobuf_LIBRARIES})
endif()