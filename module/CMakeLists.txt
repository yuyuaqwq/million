cmake_minimum_required(VERSION 3.10)
project(module)

set(LOGGER_LIB logger)
set(PYSVR_LIB pysvr)

add_subdirectory(db)
# add_subdirectory(logger)
add_subdirectory(gateway)
add_subdirectory(pysvr)