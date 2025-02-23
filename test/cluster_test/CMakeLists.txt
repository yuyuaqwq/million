set(CLUSTER_TEST cluster_test)

link_directories(${LIB_PATH})

add_executable(${CLUSTER_TEST} cluster_test.cpp)

target_include_directories(${CLUSTER_TEST} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${CLUSTER_TEST} PRIVATE ${MILLION_LIB})

target_include_directories(${CLUSTER_TEST} PRIVATE ${CLUSTER_HEADER_DIR})
target_link_libraries(${CLUSTER_TEST} PRIVATE ${CLUSTER_LIB})

target_compile_definitions(${CLUSTER_TEST} PRIVATE ${PROTOGEN_COMPILE_DEFINITIONS})
target_include_directories(${CLUSTER_TEST} PRIVATE ${PROTOGEN_HEADER_DIR})
target_link_libraries(${CLUSTER_TEST} PRIVATE ${PROTOGEN_LIB})

target_include_directories(${CLUSTER_TEST} PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(${CLUSTER_TEST} PRIVATE ${YAML_CPP_LIB})

target_include_directories(${CLUSTER_TEST} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${CLUSTER_TEST} PRIVATE ${PROTOBUF_LIB})