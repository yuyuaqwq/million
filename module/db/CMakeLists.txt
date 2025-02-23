set(DB_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE STRING "db header dir")
set(DB_LIB db CACHE STRING "db lib")

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
add_library(${DB_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${DB_LIB} PUBLIC ${DB_HEADER_DIR})
target_include_directories(${DB_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${DB_LIB} PRIVATE ${MILLION_LIB})

target_include_directories(${DB_LIB} PRIVATE ${REDIS_PLUS_PLUS_HEADER_DIR})
target_link_libraries(${DB_LIB} PRIVATE ${REDIS_PLUS_PLUS_LIB})

#target_include_directories(${DB_LIB} PRIVATE ${MYSQL_HEADER_DIR})
#target_link_libraries(${DB_LIB} PRIVATE ${MYSQL_LIB})
target_include_directories(${DB_LIB} PRIVATE ${SOCI_HEADER_DIR})
target_link_libraries(${DB_LIB} PRIVATE ${SOCI_LIB})