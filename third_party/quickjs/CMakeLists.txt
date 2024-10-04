cmake_minimum_required(VERSION 3.10)
project(quickjs)

set(QUICKJS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

include_directories(${QUICKJS_SOURCE_DIR})

set(QUICKJS_SOURCES
    ${QUICKJS_SOURCE_DIR}/quickjs.h
    ${QUICKJS_SOURCE_DIR}/quickjs-libc.h
    ${QUICKJS_SOURCE_DIR}/quickjs.c
    ${QUICKJS_SOURCE_DIR}/libregexp.c
    ${QUICKJS_SOURCE_DIR}/libunicode.c
    ${QUICKJS_SOURCE_DIR}/libbf.c
    ${QUICKJS_SOURCE_DIR}/cutils.c
    ${QUICKJS_SOURCE_DIR}/quickjs-libc.c
)

add_definitions(-DCONFIG_VERSION=\"2024-01-13\")

add_library(quickjs STATIC ${QUICKJS_SOURCES})