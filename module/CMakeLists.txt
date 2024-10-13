cmake_minimum_required(VERSION 3.10)
project(module)

set(GATEWAY_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pocketpy)
set(DB_LIB db)
set(LOGGER_LIB logger)
set(GATEWAY_LIB gateway)
set(PYSVR_LIB pysvr)

add_subdirectory(db)
# add_subdirectory(logger)
add_subdirectory(gateway)
add_subdirectory(pysvr)