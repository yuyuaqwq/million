cmake_minimum_required(VERSION 3.10)
project(module)

set(DB_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/db/include CACHE STRING "db header dir")
set(DB_LIB db CACHE STRING "db lib")

set(GATEWAY_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gateway/include CACHE STRING "gateway header dir")
set(GATEWAY_LIB gateway CACHE STRING "gateway lib")

set(LOGGER_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/logger/include CACHE STRING "logger header dir")
set(LOGGER_LIB logger CACHE STRING "logger lib")

set(PYSVR_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pysvr/include CACHE STRING "pysvr header dir")
set(PYSVR_LIB pysvr CACHE STRING "pysvr lib")

add_subdirectory(db)
add_subdirectory(logger)
add_subdirectory(gateway)
add_subdirectory(pysvr)