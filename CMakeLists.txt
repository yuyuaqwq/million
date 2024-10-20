cmake_minimum_required(VERSION 3.10)
project(million)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# proto
# set(MILLION_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTOGEN_LIB protogen)
set(PROTOGEN_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/protogen)

#desc
set(DESCGEN_LIB descgen)
set(DESCGEN_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/descgen)

# module
set(DB_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/module/db/include)
set(DB_LIB db)

set(GATEWAY_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/module/gateway/include)
set(GATEWAY_LIB gateway)

# third_party
set(META_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/meta/include)

set(ASIO_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)

set(YAML_CPP_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/yaml-cpp/include)
set(YAML_CPP_LIB yaml-cpp)

set(NLOHMANN_JSON_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json/include)
set(NLOHMANN_JSON_LIB nlohmann_json::nlohmann_json)

set(SPDLOG_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include)
set(SPDLOG_LIB spdlog::spdlog)

set(POCKET_PY_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pocketpy)
set(POCKET_PY_LIB pocketpy)

set(HIREDIS_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hiredis)
set(HIREDIS_LIB hiredis)

set(REDIS_PLUS_PLUS_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/redis-plus-plus/src)
set(REDIS_PLUS_PLUS_LIB redis++_static hiredis)

set(SOCI_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/soci/include)

# million
set(MILLION_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/million/include ${CMAKE_CURRENT_SOURCE_DIR} ${META_HEADER})
set(MILLION_LIB million)

# add
add_subdirectory(million)
add_subdirectory(module)
add_subdirectory(example)
add_subdirectory(protogen)
add_subdirectory(descgen)
set(YAML_BUILD_SHARED_LIBS ON)
add_subdirectory(third_party/yaml-cpp)
add_subdirectory(third_party/nlohmann_json)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/pocketpy)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(third_party/hiredis/hiredis)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "Disable redis-plus-plus test" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "Build redis-plus-plus static library" FORCE)
add_subdirectory(third_party/redis-plus-plus)
add_subdirectory(third_party/soci)