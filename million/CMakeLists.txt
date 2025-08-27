set(MILLION_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(MILLION_LIB_TARGET million)

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

add_library(${MILLION_LIB_TARGET} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${MILLION_LIB_TARGET} PUBLIC ${MILLION_INCLUDE_DIR})
target_include_directories(${MILLION_LIB_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${MILLION_LIB_TARGET} PUBLIC million::third_party::meta)
target_link_libraries(${MILLION_LIB_TARGET} PUBLIC million::third_party::asio)
target_link_libraries(${MILLION_LIB_TARGET} PUBLIC million::third_party::yaml-cpp)
target_link_libraries(${MILLION_LIB_TARGET} PUBLIC million::third_party::protobuf)

find_package(Threads)
target_link_libraries(${MILLION_LIB_TARGET} PRIVATE Threads::Threads)

add_library(million::core ALIAS ${MILLION_LIB_TARGET})