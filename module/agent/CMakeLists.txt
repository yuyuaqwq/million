set(AGENT_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE STRING "agent header dir")
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

target_include_directories(${AGENT_LIB} PUBLIC ${AGENT_HEADER_DIR})
target_include_directories(${AGENT_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${AGENT_LIB} PUBLIC ${MILLION_LIB})
target_link_libraries(${AGENT_LIB} PUBLIC ${GATEWAY_LIB})