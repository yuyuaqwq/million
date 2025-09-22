set(MILLION_GATEWAY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(MILLION_GATEWAY_LIB_TARGET million_gateway)

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
add_library(${MILLION_GATEWAY_LIB_TARGET} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${MILLION_GATEWAY_LIB_TARGET} PUBLIC ${MILLION_GATEWAY_INCLUDE_DIR})
target_include_directories(${MILLION_GATEWAY_LIB_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${MILLION_GATEWAY_LIB_TARGET} PUBLIC million::core million::protogen)

add_library(million::gateway ALIAS ${MILLION_GATEWAY_LIB_TARGET})