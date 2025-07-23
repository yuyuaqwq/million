set(ETCD_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE STRING "etcd header dir")
set(ETCD_LIB etcd CACHE STRING "etcd lib")

# 定义编译宏
add_compile_definitions(BUILDING_ETCD_MODULE)

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

# Link with etcd-cpp-apiv3 if available
if(TARGET ${ETCD_CPP_LIB})
    target_link_libraries(${ETCD_LIB} PUBLIC ${ETCD_CPP_LIB})
    target_include_directories(${ETCD_LIB} PUBLIC ${ETCD_CPP_HEADER_DIR})
else()
    message(WARNING "etcd-cpp-apiv3 library not found")
endif()