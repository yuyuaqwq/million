set(GATEWAY_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE STRING "gateway header dir")
set(GATEWAY_LIB gateway CACHE STRING "gateway lib")

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

target_include_directories(${GATEWAY_LIB} PUBLIC ${GATEWAY_HEADER_DIR})
target_include_directories(${GATEWAY_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${GATEWAY_LIB} PRIVATE ${MILLION_LIB})