cmake_minimum_required(VERSION 3.10)
project(module)

set(CLUSTER_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cluster/include CACHE STRING "cluster header dir")
set(CLUSTER_LIB cluster CACHE STRING "cluster lib")

set(DB_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/db/include CACHE STRING "db header dir")
set(DB_LIB db CACHE STRING "db lib")

set(DESC_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/desc/include CACHE STRING "desc header dir")
set(DESC_LIB desc CACHE STRING "desc lib")

set(AGENT_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/agent/include CACHE STRING "agent header dir")
set(AGENT_LIB agent CACHE STRING "agent lib")

set(GATEWAY_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gateway/include CACHE STRING "gateway header dir")
set(GATEWAY_LIB gateway CACHE STRING "gateway lib")

set(JSSVR_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jssvr/include CACHE STRING "jssvr header dir")
set(JSSVR_LIB jssvr CACHE STRING "jssvr lib")

add_subdirectory(cluster)
add_subdirectory(db)
add_subdirectory(desc)
add_subdirectory(agent)
add_subdirectory(gateway)
add_subdirectory(jssvr)