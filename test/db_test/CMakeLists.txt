set(DB_TEST db_test)

link_directories(${LIB_PATH})

add_executable(${DB_TEST} db_test.cpp)

target_include_directories(${DB_TEST} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${DB_TEST} PRIVATE ${MILLION_LIB})

target_include_directories(${DB_TEST} PRIVATE ${DB_HEADER_DIR})
target_link_libraries(${DB_TEST} PRIVATE ${DB_LIB})

target_include_directories(${DB_TEST} PRIVATE ${PROTOGEN_HEADER_DIR})
target_link_libraries(${DB_TEST} PRIVATE ${PROTOGEN_LIB})
target_compile_definitions(${DB_TEST} PRIVATE ${PROTOGEN_COMPILE_DEFINITIONS})

target_include_directories(${DB_TEST} PRIVATE ${YAML_CPP_HEADER_DIR})
target_link_libraries(${DB_TEST} PRIVATE ${YAML_CPP_LIB})

target_include_directories(${DB_TEST} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${DB_TEST} PRIVATE ${PROTOBUF_LIB})