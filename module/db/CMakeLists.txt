set(MILLION_DB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(MILLION_DB_LIB_TARGET million_db)

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
add_library(${MILLION_DB_LIB_TARGET} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${MILLION_DB_LIB_TARGET} PUBLIC ${MILLION_DB_INCLUDE_DIR})
target_include_directories(${MILLION_DB_LIB_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${MILLION_DB_LIB_TARGET} PUBLIC million::core million::protogen)

target_link_libraries(${MILLION_DB_LIB_TARGET} PRIVATE million::third_party::redis++)
target_link_libraries(${MILLION_DB_LIB_TARGET} PRIVATE million::third_party::soci)

add_library(million::db ALIAS ${MILLION_DB_LIB_TARGET})