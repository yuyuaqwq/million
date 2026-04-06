set(MILLION_TLOG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(MILLION_TLOG_LIB_GARGET million_tlog)

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
add_library(${MILLION_TLOG_LIB_GARGET} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_include_directories(${MILLION_TLOG_LIB_GARGET} PUBLIC ${MILLION_TLOG_INCLUDE_DIR})
target_include_directories(${MILLION_TLOG_LIB_GARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${MILLION_TLOG_LIB_GARGET} PUBLIC million::core)

target_link_libraries(${MILLION_TLOG_LIB_GARGET} PRIVATE million::third_party::spdlog)
target_link_libraries(${MILLION_TLOG_LIB_GARGET} PRIVATE million::third_party::nlohmann_json)

add_library(million::tlog ALIAS ${MILLION_TLOG_LIB_GARGET})
