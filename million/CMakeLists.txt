set(MILLION_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE STRING "million header dir")
set(MILLION_LIB million CACHE STRING "million lib")

if(WIN32)
    file(GLOB_RECURSE PRIVATE_HEADERS 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.hpp)
    source_group("Private Header Files" FILES ${PRIVATE_HEADERS})
    
    file(GLOB_RECURSE PUBLIC_HEADERS 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp)
    source_group("Public Header Files" FILES ${PUBLIC_HEADERS})
endif()
file(GLOB SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp)

add_library(${MILLION_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${MILLION_LIB} PUBLIC ${MILLION_HEADER_DIR})
target_include_directories(${MILLION_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${MILLION_LIB} PUBLIC ${META_LIB})
target_link_libraries(${MILLION_LIB} PUBLIC ${ASIO_LIB})
target_link_libraries(${MILLION_LIB} PUBLIC ${PROTOGEN_LIB})
target_link_libraries(${MILLION_LIB} PRIVATE ${SPDLOG_LIB})
target_link_libraries(${MILLION_LIB} PUBLIC ${YAML_CPP_LIB})

find_package(Threads)
target_link_libraries(${MILLION_LIB} PRIVATE Threads::Threads)