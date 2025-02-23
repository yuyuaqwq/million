set(TASK_TEST task_test)

link_directories(${LIB_PATH})

add_executable(${TASK_TEST} task_test.cpp)

target_link_libraries(${TASK_TEST} PRIVATE ${MILLION_LIB})

target_link_libraries(${TASK_TEST} PRIVATE ${GATEWAY_LIB})