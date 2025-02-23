cmake_minimum_required(VERSION 3.10)

# protobuf
find_library(PROTOBUF_PROTOC_LIB libprotocd find_library PATHS ${PROTOBUF_LIB_DIR})
find_library(PROTOBUF_CORE_LIB libprotobufd find_library PATHS ${PROTOBUF_LIB_DIR})
set(PROTOBUF_LIB ${PROTOBUF_PROTOC_LIB} ${PROTOBUF_CORE_LIB} CACHE STRING "protobuf lib")

if (WIN32)
    message(STATUS "Compiling on Windows")
elseif(UNIX)
    message(STATUS "Compiling on Linux")
    find_package(Protobuf REQUIRED)
    set(PROTOBUF_INCLUDE_DIR PRIVATE ${Protobuf_INCLUDE_DIRS} CACHE STRING "protobuf header dir")
    set(PROTOBUF_LIB PRIVATE ${Protobuf_LIBRARIES} CACHE STRING "protobuf lib")
endif()

# meta
set(META_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/meta/include CACHE STRING "meta header dir")

# asio
set(ASIO_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include CACHE STRING "asio header dir")

# yaml-cpp
set(YAML_CPP_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include CACHE STRING "yaml-cpp header dir")
set(YAML_CPP_LIB yaml-cpp CACHE STRING "yaml-cpp lib")
set(YAML_BUILD_SHARED_LIBS ON)
add_subdirectory(yaml-cpp)

# nlohmann_json
set(NLOHMANN_JSON_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json/include CACHE STRING "nlohmann_json header dir")
set(NLOHMANN_JSON_LIB nlohmann_json::nlohmann_json CACHE STRING "nlohmann_json lib")
add_subdirectory(nlohmann_json)

# spdlog
set(SPDLOG_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include CACHE STRING "spdlog header dir")
set(SPDLOG_LIB spdlog::spdlog CACHE STRING "spdlog lib")
add_subdirectory(spdlog)

# quickjs
set(QUICK_JS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/quickjs CACHE STRING "quickjs header dir")
set(QUICK_JS_LIB qjs CACHE STRING "quickjs lib")
add_subdirectory(quickjs/quickjs)

# hiredis
set(HIREDIS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hiredis CACHE STRING "hiredis header dir")
set(HIREDIS_HEADER ${HIREDIS_HEADER_DIR} CACHE STRING "hiredis lib")
set(HIREDIS_LIB hiredis)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(hiredis/hiredis)

# redis++
set(REDIS_PLUS_PLUS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/redis-plus-plus/src CACHE STRING "redis++ header dir")
set(REDIS_PLUS_PLUS_LIB redis++_static hiredis CACHE STRING "redis++ lib")
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "Disable redis-plus-plus test" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "Build redis-plus-plus static library" FORCE)
add_subdirectory(redis-plus-plus)

# soci
set(SOCI_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/soci/include ${MYSQL_HEADER_DIR} CACHE STRING "soci header dir")
set(SOCI_LIB soci_core_static soci_mysql_static CACHE STRING "soci lib")
set(MYSQL_INCLUDE_DIR "${MYSQL_HEADER_DIR}")
set(MYSQL_LIBRARIES "${MYSQL_LIB}")
add_subdirectory(soci)