set(TASK_TEST task_test)

link_directories(${LIB_PATH})

add_executable(${TASK_TEST} task_test.cpp)

target_include_directories(${TASK_TEST} PRIVATE ${MILLION_HEADER_DIR})
target_link_libraries(${TASK_TEST} PRIVATE ${MILLION_LIB})

target_include_directories(${TASK_TEST} PRIVATE ${PROTOGEN_HEADER_DIR})
target_link_libraries(${TASK_TEST} PRIVATE ${PROTOGEN_LIB})
target_compile_definitions(${TASK_TEST} PRIVATE ${PROTOGEN_COMPILE_DEFINITIONS})

target_include_directories(${TASK_TEST} PRIVATE ${GATEWAY_HEADER_DIR})
target_link_libraries(${TASK_TEST} PRIVATE ${GATEWAY_LIB})

target_include_directories(${TASK_TEST} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${TASK_TEST} PRIVATE ${PROTOBUF_LIB})