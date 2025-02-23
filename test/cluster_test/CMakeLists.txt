set(CLUSTER_TEST cluster_test)

link_directories(${LIB_PATH})

add_executable(${CLUSTER_TEST} cluster_test.cpp)

target_link_libraries(${CLUSTER_TEST} PRIVATE ${MILLION_LIB})

target_link_libraries(${CLUSTER_TEST} PRIVATE ${CLUSTER_LIB})