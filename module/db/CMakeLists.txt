cmake_minimum_required(VERSION 3.10)
project(DB_LIB)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./src/ SRC)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

add_library(${DB_LIB} SHARED ${SRC})

target_include_directories(${DB_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${DB_LIB} PRIVATE ${DB_HEADER})

target_include_directories(${DB_LIB} PRIVATE ${MILLION_HEADER})

target_include_directories(${DB_LIB} PRIVATE ${PROTOGEN_HEADER})
target_link_libraries(${DB_LIB} PRIVATE ${PROTOGEN_LIB})

target_include_directories(${DB_LIB} PRIVATE ${YAML_CPP_HEADER})
target_link_libraries(${DB_LIB} PRIVATE ${YAML_CPP_LIB})

target_include_directories(${DB_LIB} PRIVATE ${REDIS_PLUS_PLUS_HEADER})
target_link_libraries(${DB_LIB} PRIVATE ${REDIS_PLUS_PLUS_LIB})

target_include_directories(${DB_LIB} PRIVATE ${MYSQL_HEADER})
target_link_libraries(${DB_LIB} PRIVATE ${MYSQL_LIB})
target_include_directories(${DB_LIB} PRIVATE ${SOCI_HEADER})
target_link_libraries(${DB_LIB} PRIVATE soci_core_static soci_mysql_static)

if (WIN32)
    message(STATUS "Compiling on Windows")
    
    find_package(protobuf CONFIG REQUIRED)
    target_link_libraries(${DB_LIB} PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)

elseif(UNIX)
    message(STATUS "Compiling on Linux")

    find_package(Protobuf REQUIRED)
    target_include_directories(${DB_LIB} PRIVATE ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(${DB_LIB} PRIVATE ${Protobuf_LIBRARIES})
endif()