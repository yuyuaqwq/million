# protobuf
set(Protobuf_USE_STATIC_LIBS ON CACHE BOOL "Build only core protobuf")
cmake_policy(SET CMP0074 NEW)
find_package(Protobuf REQUIRED CONFIG)
# vcpkg安装的默认是动态版本
set(MILLION_PROTOBUF_LIB_TARGET million_protobuf)
add_library(${MILLION_PROTOBUF_LIB_TARGET} INTERFACE)
target_compile_definitions(${MILLION_PROTOBUF_LIB_TARGET} INTERFACE PROTOBUF_USE_DLLS)
target_include_directories(${MILLION_PROTOBUF_LIB_TARGET} INTERFACE ${Protobuf_INCLUDE_DIRS})
target_link_libraries(${MILLION_PROTOBUF_LIB_TARGET} INTERFACE protobuf::libprotobuf)
#target_link_libraries(${MILLION_PROTOBUF_LIB_TARGET} INTERFACE ${Protobuf_LIBRARIES})

find_package(absl QUIET)
#find_package(absl REQUIRED)
if (absl_FOUND)
    # target_link_libraries(${MILLION_PROTOBUF_LIB_TARGET} INTERFACE ${absl_cpp_LIBRARIES})
    target_link_libraries(${MILLION_PROTOBUF_LIB_TARGET} INTERFACE absl::base absl::strings)
else()
    message(WARNING "Abseil not found, proceeding without it.")
endif()
add_library(million::third_party::protobuf ALIAS ${MILLION_PROTOBUF_LIB_TARGET})

# meta
set(MILLION_META_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/meta/include)
set(MILLION_META_LIB_TARGET million_meta)
add_library(${MILLION_META_LIB_TARGET} INTERFACE)
target_include_directories(${MILLION_META_LIB_TARGET} INTERFACE ${MILLION_META_INCLUDE_DIR})
add_library(million::third_party::meta ALIAS ${MILLION_META_LIB_TARGET})

# asio
set(MILLION_ASIO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include)
set(MILLION_ASIO_LIB_TARGET asio)
add_library(${MILLION_ASIO_LIB_TARGET} INTERFACE)
target_include_directories(${MILLION_ASIO_LIB_TARGET} INTERFACE ${MILLION_ASIO_INCLUDE_DIR})
add_library(million::third_party::asio ALIAS ${MILLION_ASIO_LIB_TARGET})

# yaml-cpp
set(YAML_BUILD_SHARED_LIBS ON)
add_subdirectory(yaml-cpp)
set(MILLION_YAML_CPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)
set(MILLION_YAML_CPP_LIB_TARGET yaml-cpp)
target_include_directories(${MILLION_YAML_CPP_LIB_TARGET} PUBLIC ${MILLION_YAML_CPP_INCLUDE_DIR})
add_library(million::third_party::yaml-cpp ALIAS ${MILLION_YAML_CPP_LIB_TARGET})

# nlohmann_json
add_subdirectory(nlohmann_json)
set(MILLION_NLOHMANN_JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json/include)
set(MILLION_NLOHMANN_JSON_LIB_TARGET million_nlohmann_json)
add_library(${MILLION_NLOHMANN_JSON_LIB_TARGET} INTERFACE)
target_link_libraries(${MILLION_NLOHMANN_JSON_LIB_TARGET} INTERFACE nlohmann_json::nlohmann_json)
target_include_directories(${MILLION_NLOHMANN_JSON_LIB_TARGET} INTERFACE ${MILLION_NLOHMANN_JSON_INCLUDE_DIR})
add_library(million::third_party::nlohmann_json ALIAS ${MILLION_NLOHMANN_JSON_LIB_TARGET})

# spdlog
add_subdirectory(spdlog)
set(MILLION_SPDLOG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include)
set(MILLION_SPDLOG_LIB_TARGET million_spdlog)
add_library(${MILLION_SPDLOG_LIB_TARGET} INTERFACE)
target_link_libraries(${MILLION_SPDLOG_LIB_TARGET} INTERFACE spdlog::spdlog)
target_include_directories(${MILLION_SPDLOG_LIB_TARGET} INTERFACE ${MILLION_SPDLOG_INCLUDE_DIR})
add_library(million::third_party::spdlog ALIAS ${MILLION_SPDLOG_LIB_TARGET})

# quickjs
# cmake不允许引用项目外部路径作为include的路径，因此使用INTERFACE封装一层
add_subdirectory(quickjs/quickjs)
set(MILLION_QUICK_JS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/quickjs)
set(MILLION_QUICK_JS_LIB_TARGET million_quickjs)
add_library(${MILLION_QUICK_JS_LIB_TARGET} INTERFACE)
target_link_libraries(${MILLION_QUICK_JS_LIB_TARGET} INTERFACE qjs)
target_include_directories(${MILLION_QUICK_JS_LIB_TARGET} INTERFACE ${MILLION_QUICK_JS_INCLUDE_DIR})
add_library(million::third_party::quickjs ALIAS ${MILLION_QUICK_JS_LIB_TARGET})

# multjs
add_subdirectory(multjs)
set(MILLION_MULT_JS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/multjs/include)
set(MILLION_MULT_JS_LIB_TARGET million_mjs)
add_library(${MILLION_MULT_JS_LIB_TARGET} INTERFACE)
target_link_libraries(${MILLION_MULT_JS_LIB_TARGET} INTERFACE mjs)
target_include_directories(${MILLION_MULT_JS_LIB_TARGET} INTERFACE ${MILLION_MULT_JS_INCLUDE_DIR})
add_library(million::third_party::mjs ALIAS ${MILLION_MULT_JS_LIB_TARGET})

# hiredis
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(hiredis/hiredis)
set(MILLION_HIREDIS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hiredis)
set(MILLION_HIREDIS_LIB_TARGET million_hiredis)
add_library(${MILLION_HIREDIS_LIB_TARGET} INTERFACE)
target_link_libraries(${MILLION_HIREDIS_LIB_TARGET} INTERFACE hiredis)
target_include_directories(${MILLION_HIREDIS_LIB_TARGET} INTERFACE ${MILLION_HIREDIS_INCLUDE_DIR})
add_library(million::third_party::hiredis ALIAS ${MILLION_HIREDIS_LIB_TARGET})

# redis++
set(HIREDIS_HEADER ${MILLION_HIREDIS_INCLUDE_DIR} CACHE STRING "Hiredis header" FORCE)
set(HIREDIS_LIB hiredis CACHE STRING "Hiredis lib" FORCE)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "Disable redis-plus-plus test" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "Build redis-plus-plus static library" FORCE)
add_subdirectory(redis-plus-plus)
set(MILLION_REDIS_PLUS_PLUS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/redis-plus-plus/src)
set(MILLION_REDIS_PLUS_PLUS_LIB_TARGET million_redis_plus_plus)
add_library(${MILLION_REDIS_PLUS_PLUS_LIB_TARGET} INTERFACE)
target_link_libraries(${MILLION_REDIS_PLUS_PLUS_LIB_TARGET} INTERFACE redis++_static hiredis)
target_include_directories(${MILLION_REDIS_PLUS_PLUS_LIB_TARGET} INTERFACE ${MILLION_REDIS_PLUS_PLUS_INCLUDE_DIR})
add_library(million::third_party::redis++ ALIAS ${MILLION_REDIS_PLUS_PLUS_LIB_TARGET})

# soci
if(DEFINED MYSQL_INCLUDE_DIR)
    find_package(unofficial-libmysql REQUIRED)
    add_subdirectory(soci)
    set(MILLION_SOCI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/soci/include)
    set(MILLION_SOCI_LIB_TARGET million_soci)
    add_library(${MILLION_SOCI_LIB_TARGET} INTERFACE)
    target_link_libraries(${MILLION_SOCI_LIB_TARGET} INTERFACE soci_core_static)
    target_link_libraries(${MILLION_SOCI_LIB_TARGET} INTERFACE soci_mysql_static)
    target_link_libraries(${MILLION_SOCI_LIB_TARGET} INTERFACE ${MYSQL_LIBRARIES})
    target_include_directories(${MILLION_SOCI_LIB_TARGET} INTERFACE ${MYSQL_INCLUDE_DIR})
    target_include_directories(${MILLION_SOCI_LIB_TARGET} INTERFACE ${MILLION_SOCI_INCLUDE_DIR})
    add_library(million::third_party::soci ALIAS ${MILLION_SOCI_LIB_TARGET})
else()
	message(STATUS "Skipping MySQL/SOCI setup: MYSQL_INCLUDE_DIR is not defined")
endif()

# etcd-cpp-apiv3
set(BUILD_ETCD_CORE_ONLY ON CACHE BOOL "Build only core etcd-cpp-apiv3")
add_subdirectory(etcd-cpp-apiv3)
set(MILLION_ETCD_CPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/etcd-cpp-apiv3)
set(MILLION_ETCD_CPP_LIB_TARGET etcd-cpp-apiv3)
add_library(${MILLION_ETCD_CPP_LIB_TARGET} INTERFACE)
target_link_libraries(${MILLION_ETCD_CPP_LIB_TARGET} INTERFACE etcd-cpp-api)
target_include_directories(${MILLION_ETCD_CPP_LIB_TARGET} INTERFACE ${MILLION_ETCD_CPP_INCLUDE_DIR})
add_library(million::third_party::etcd-cpp-apiv3 ALIAS ${MILLION_ETCD_CPP_LIB_TARGET})