set(JSSVR_TEST jssvr_test)

link_directories(${LIB_PATH})

add_executable(${JSSVR_TEST} jssvr_test.cpp)

target_include_directories(${JSSVR_TEST} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${JSSVR_TEST} PRIVATE ${MILLION_LIB})

target_include_directories(${JSSVR_TEST} PRIVATE ${JSSVR_HEADER_DIR})
target_link_libraries(${JSSVR_TEST} PRIVATE ${JSSVR_LIB})

target_include_directories(${JSSVR_TEST} PRIVATE ${PROTOGEN_HEADER_DIR})
target_link_libraries(${JSSVR_TEST} PRIVATE ${PROTOGEN_LIB})
target_compile_definitions(${JSSVR_TEST} PRIVATE ${PROTOGEN_COMPILE_DEFINITIONS})

target_include_directories(${JSSVR_TEST} PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(${JSSVR_TEST} PRIVATE ${YAML_CPP_LIB})

target_include_directories(${JSSVR_TEST} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${JSSVR_TEST} PRIVATE ${PROTOBUF_LIB})