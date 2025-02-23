set(TABLE_TEST table_test)

link_directories(${LIB_PATH})

add_executable(${TABLE_TEST} table_test.cpp)

target_include_directories(${TABLE_TEST} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${TABLE_TEST} PRIVATE ${MILLION_LIB})

target_include_directories(${TABLE_TEST} PRIVATE ${TABLE_HEADER_DIR})
target_link_libraries(${TABLE_TEST} PRIVATE ${TABLE_LIB})

target_include_directories(${TABLE_TEST} PRIVATE ${PROTOGEN_HEADER_DIR})
target_link_libraries(${TABLE_TEST} PRIVATE ${PROTOGEN_LIB})
target_compile_definitions(${TABLE_TEST} PRIVATE ${PROTOGEN_COMPILE_DEFINITIONS})

target_include_directories(${TABLE_TEST} PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(${TABLE_TEST} PRIVATE ${YAML_CPP_LIB})

target_include_directories(${TABLE_TEST} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${TABLE_TEST} PRIVATE ${PROTOBUF_LIB})