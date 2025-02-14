cmake_minimum_required(VERSION 3.10)
project(million)

set(MILLION_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

set(LIBRARY_OUTPUT_PATH ${MILLION_BUILD_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${MILLION_BUILD_DIR}/bin)

# proto
# set(MILLION_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/proto)

find_library(PROTOBUF_PROTOC_LIB libprotocd find_library PATHS ${PROTOBUF_LIB_DIR})
find_library(PROTOBUF_CORE_LIB libprotobufd find_library PATHS ${PROTOBUF_LIB_DIR})
set(PROTOBUF_LIB ${PROTOBUF_PROTOC_LIB} ${PROTOBUF_CORE_LIB})

if (WIN32)
    set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_API=__declspec\(dllimport\) CACHE STRING "protobuf compile definitions")
elseif(UNIX)
    set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_API=__attribute__\(\(visibility\(\"hidden\"\)\)\) CACHE STRING "protobuf compile definitions")
endif()
set(PROTOGEN_HEADER_DIR 
    ${CMAKE_CURRENT_SOURCE_DIR}/protogen/include
    CACHE STRING "protobuf header dir")
set(PROTOGEN_LIB protogen CACHE STRING "protobuf lib")

if (WIN32)
    message(STATUS "Compiling on Windows")

elseif(UNIX)
    message(STATUS "Compiling on Linux")

    find_package(Protobuf REQUIRED)
    set(PROTOBUF_INCLUDE_DIR PRIVATE ${Protobuf_INCLUDE_DIRS})
    set(PROTOBUF_LIB PRIVATE ${Protobuf_LIBRARIES})
endif()

# desc
set(MILLION_DESCGEN_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/descgen/include CACHE STRING "descgen header dir")
set(MILLION_DESCGEN_LIB million_descgen CACHE STRING "descgen lib")

# third_party
set(META_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/meta/include)

set(ASIO_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include CACHE STRING "asio header dir")

set(YAML_CPP_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/yaml-cpp/include CACHE STRING "yaml-cpp header dir")
set(YAML_CPP_LIB yaml-cpp CACHE STRING "yaml-cpp lib")

set(NLOHMANN_JSON_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/include)
set(NLOHMANN_JSON_LIB nlohmann_json::nlohmann_json)

set(SPDLOG_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include)
set(SPDLOG_LIB spdlog::spdlog)

set(QUICK_JS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs)
set(QUICK_JS_LIB qjs)

set(HIREDIS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hiredis)
set(HIREDIS_HEADER ${HIREDIS_HEADER_DIR})
set(HIREDIS_LIB hiredis)

set(REDIS_PLUS_PLUS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/redis-plus-plus/src)
set(REDIS_PLUS_PLUS_LIB redis++_static hiredis)

set(SOCI_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/soci/include ${MYSQL_HEADER_DIR})
set(SOCI_LIB soci_core_static soci_mysql_static)

# million
set(MILLION_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/million/include;${CMAKE_CURRENT_SOURCE_DIR};${META_HEADER_DIR}" CACHE STRING "million header dir")
set(MILLION_LIB million CACHE STRING "million lib")

# add
add_subdirectory(million)
add_subdirectory(module)
add_subdirectory(test)
add_subdirectory(protogen)
set(YAML_BUILD_SHARED_LIBS ON)
add_subdirectory(third_party/yaml-cpp)
add_subdirectory(third_party/nlohmann_json)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/quickjs/quickjs)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(third_party/hiredis/hiredis)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "Disable redis-plus-plus test" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "Build redis-plus-plus static library" FORCE)
add_subdirectory(third_party/redis-plus-plus)
set(MYSQL_INCLUDE_DIR "${MYSQL_HEADER_DIR}")
set(MYSQL_LIBRARIES "${MYSQL_LIB}")
add_subdirectory(third_party/soci)