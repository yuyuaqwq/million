set(MILLION_CLUSTER_TEST_TARGET million_cluster_test)

add_executable(${MILLION_CLUSTER_TEST_TARGET} cluster_test.cpp)

target_link_libraries(${MILLION_CLUSTER_TEST_TARGET} PRIVATE million::core million::cluster)