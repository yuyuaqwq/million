set(ETCD_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE STRING "etcd header dir")
set(ETCD_LIB etcd CACHE STRING "etcd lib")

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
add_library(${ETCD_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${ETCD_LIB} PUBLIC ${ETCD_HEADER_DIR})
target_include_directories(${ETCD_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${ETCD_LIB} PUBLIC ${MILLION_LIB})
target_link_libraries(${ETCD_LIB} PUBLIC ${PROTOGEN_LIB})

# Link etcd-cpp-apiv3 library
target_link_libraries(${ETCD_LIB} PRIVATE ${ETCD_CPP_LIB})

# Include yaml-cpp for configuration
target_link_libraries(${ETCD_LIB} PRIVATE ${YAML_CPP_LIB}) 