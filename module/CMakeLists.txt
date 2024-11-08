cmake_minimum_required(VERSION 3.10)
project(module)

set(CLUSTER_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cluster/include CACHE STRING "cluster header dir")
set(CLUSTER_LIB cluster CACHE STRING "cluster lib")

set(DB_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/db/include CACHE STRING "db header dir")
set(DB_LIB db CACHE STRING "db lib")

set(GATEWAY_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gateway/include CACHE STRING "gateway header dir")
set(GATEWAY_LIB gateway CACHE STRING "gateway lib")

set(PYSVR_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pysvr/include CACHE STRING "pysvr header dir")
set(PYSVR_LIB pysvr CACHE STRING "pysvr lib")

add_subdirectory(cluster)
add_subdirectory(db)
add_subdirectory(gateway)
add_subdirectory(pysvr)