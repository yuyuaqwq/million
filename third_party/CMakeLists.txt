# protobuf
set(PROTOBUF_LIB protobuf_million CACHE STRING "protobuf lib")
set(Protobuf_USE_STATIC_LIBS ON CACHE BOOL "Build only core protobuf")
add_library(${PROTOBUF_LIB} INTERFACE)
cmake_policy(SET CMP0074 NEW)
find_package(Protobuf REQUIRED CONFIG)
# vcpkg安装的默认是动态版本
target_compile_definitions(${PROTOBUF_LIB} INTERFACE PROTOBUF_USE_DLLS)
target_include_directories(${PROTOBUF_LIB} INTERFACE ${Protobuf_INCLUDE_DIRS})

target_link_libraries(${PROTOBUF_LIB} INTERFACE 
    protobuf::libprotobuf
)

#target_link_libraries(${PROTOBUF_LIB} INTERFACE ${Protobuf_LIBRARIES})

find_package(absl QUIET)
#find_package(absl REQUIRED)
if (absl_FOUND)
    # target_link_libraries(${PROTOBUF_LIB} INTERFACE ${absl_cpp_LIBRARIES})
    target_link_libraries(${PROTOBUF_LIB} INTERFACE absl::base absl::strings)
else()
    message(WARNING "Abseil not found, proceeding without it.")
endif()

# meta
set(META_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/meta/include CACHE STRING "meta header dir")
set(META_LIB meta CACHE STRING "meta lib")
add_library(${META_LIB} INTERFACE)
target_include_directories(${META_LIB} INTERFACE ${META_HEADER_DIR})

# asio
set(ASIO_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include CACHE STRING "asio header dir")
set(ASIO_LIB asio CACHE STRING "asio lib")
add_library(${ASIO_LIB} INTERFACE)
target_include_directories(${ASIO_LIB} INTERFACE ${ASIO_HEADER_DIR})

# yaml-cpp
set(YAML_CPP_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include CACHE STRING "yaml-cpp header dir")
set(YAML_CPP_LIB yaml-cpp CACHE STRING "yaml-cpp lib")
set(YAML_BUILD_SHARED_LIBS ON)
add_subdirectory(yaml-cpp)
target_include_directories(${YAML_CPP_LIB} PUBLIC ${YAML_CPP_HEADER_DIR})

# nlohmann_json
set(NLOHMANN_JSON_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json/include CACHE STRING "nlohmann_json header dir")
set(NLOHMANN_JSON_LIB nlohmann_json::nlohmann_json CACHE STRING "nlohmann_json lib")
add_subdirectory(nlohmann_json)
# target_include_directories(${NLOHMANN_JSON_LIB} PUBLIC ${NLOHMANN_JSON_HEADER_DIR})

# spdlog
set(SPDLOG_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include CACHE STRING "spdlog header dir")
set(SPDLOG_LIB spdlog::spdlog CACHE STRING "spdlog lib")
add_subdirectory(spdlog)
# target_include_directories(${SPDLOG_LIB} PUBLIC ${SPDLOG_HEADER_DIR})

# quickjs
# cmake不允许引用项目外部路径作为include的路径，因此使用INTERFACE封装一层
set(QUICK_JS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/quickjs CACHE STRING "quickjs header dir")
set(QUICK_JS_LIB quickjs_million CACHE STRING "quickjs lib")
add_subdirectory(quickjs/quickjs)
add_library(${QUICK_JS_LIB} INTERFACE)
target_link_libraries(${QUICK_JS_LIB} INTERFACE qjs)
target_include_directories(${QUICK_JS_LIB} INTERFACE ${QUICK_JS_HEADER_DIR})

# multjs
set(MULT_JS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/multjs/include CACHE STRING "multjs header dir")
set(MULT_JS_LIB mjs CACHE STRING "multjs lib")
add_subdirectory(multjs)
#add_library(${MULT_JS_LIB} INTERFACE)
#target_link_libraries(${MULT_JS_LIB} INTERFACE mjs)
target_include_directories(${MULT_JS_LIB} INTERFACE ${MULT_JS_HEADER_DIR})


# hiredis
set(HIREDIS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hiredis CACHE STRING "hiredis header dir")
set(HIREDIS_HEADER ${HIREDIS_HEADER_DIR} CACHE STRING "hiredis lib")
set(HIREDIS_LIB hiredis)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(hiredis/hiredis)
# target_include_directories(${HIREDIS_LIB} PUBLIC ${HIREDIS_HEADER_DIR})

# redis++
set(REDIS_PLUS_PLUS_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/redis-plus-plus/src CACHE STRING "redis++ header dir")
set(REDIS_PLUS_PLUS_LIB redis++_static hiredis CACHE STRING "redis++ lib")
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "Disable redis-plus-plus test" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "Build redis-plus-plus static library" FORCE)
add_subdirectory(redis-plus-plus)
# target_include_directories(${REDIS_PLUS_PLUS_LIB} PUBLIC ${REDIS_PLUS_PLUS_HEADER_DIR})

# soci
find_package(libmysql REQUIRED CONFIG)
set(SOCI_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/soci/include CACHE STRING "soci header dir")
set(SOCI_LIB soci_million CACHE STRING "soci lib")
add_subdirectory(soci)
add_library(${SOCI_LIB} INTERFACE)
target_link_libraries(${SOCI_LIB} INTERFACE soci_core_static)
target_link_libraries(${SOCI_LIB} INTERFACE soci_mysql_static)
target_include_directories(${SOCI_LIB} INTERFACE ${MYSQL_INCLUDE_DIR})
# target_include_directories(${SOCI_LIB} INTERFACE ${SOCI_HEADER_DIR})

# etcd-cpp-apiv3
set(BUILD_ETCD_CORE_ONLY ON CACHE BOOL "Build only core etcd-cpp-apiv3")
set(ETCD_CPP_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/etcd-cpp-apiv3 CACHE STRING "etcd-cpp-apiv3 header dir")
set(ETCD_CPP_LIB etcd-cpp-apiv3 CACHE STRING "etcd-cpp-apiv3 lib")
add_library(${ETCD_CPP_LIB} INTERFACE)
add_subdirectory(etcd-cpp-apiv3)
target_link_libraries(${ETCD_CPP_LIB} INTERFACE etcd-cpp-apiv3)
target_include_directories(${ETCD_CPP_LIB} INTERFACE ${ETCD_CPP_HEADER_DIR})