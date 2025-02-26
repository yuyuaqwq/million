# protobuf
set(PROTOBUF_LIB protobuf_million CACHE STRING "protobuf lib")
add_library(${PROTOBUF_LIB} INTERFACE)
if (WIN32)
    message(STATUS "Compiling on Windows")

    find_library(PROTOBUF_PROTOC_LIB libprotocd find_library PATHS ${PROTOBUF_LIB_DIR})
    find_library(PROTOBUF_CORE_LIB libprotobufd find_library PATHS ${PROTOBUF_LIB_DIR})

    # 检查是否找到 protoc 库
    if(NOT PROTOBUF_PROTOC_LIB)
        message(FATAL_ERROR "Failed to find Protobuf protoc library in ${PROTOBUF_LIB_DIR}")
    endif()

    # 检查是否找到 protobuf 核心库
    if(NOT PROTOBUF_CORE_LIB)
        message(FATAL_ERROR "Failed to find Protobuf core library in ${PROTOBUF_LIB_DIR}")
    endif()

    target_compile_definitions(${PROTOBUF_LIB} INTERFACE PROTOBUF_USE_DLLS)
    target_include_directories(${PROTOBUF_LIB} INTERFACE ${PROTOBUF_INCLUDE_DIR})
    target_link_libraries(${PROTOBUF_LIB} INTERFACE ${PROTOBUF_PROTOC_LIB})
    target_link_libraries(${PROTOBUF_LIB} INTERFACE ${PROTOBUF_CORE_LIB})
elseif(UNIX)
    message(STATUS "Compiling on Linux")

    find_package(Protobuf REQUIRED)

    set(PROTOBUF_INCLUDE_DIR PRIVATE ${Protobuf_INCLUDE_DIRS} CACHE STRING "protobuf header dir")
    target_include_directories(${PROTOBUF_LIB} INTERFACE ${PROTOBUF_INCLUDE_DIR})
    target_link_libraries(${PROTOBUF_LIB} INTERFACE ${Protobuf_LIBRARIES})
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
set(SOCI_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/soci/include CACHE STRING "soci header dir")
set(SOCI_LIB soci_million CACHE STRING "soci lib")
set(MYSQL_INCLUDE_DIR "${MYSQL_HEADER_DIR}")
set(MYSQL_LIBRARIES "${MYSQL_LIB}")
add_subdirectory(soci)
add_library(${SOCI_LIB} INTERFACE)
target_link_libraries(${SOCI_LIB} INTERFACE soci_core_static)
target_link_libraries(${SOCI_LIB} INTERFACE soci_mysql_static)
target_include_directories(${SOCI_LIB} INTERFACE ${MYSQL_HEADER_DIR})
# target_include_directories(${SOCI_LIB} INTERFACE ${SOCI_HEADER_DIR})