set(MILLION_ETCD_TEST_TARGET million_etcd_test)

add_executable(${MILLION_ETCD_TEST_TARGET} etcd_test.cpp)

target_link_libraries(${MILLION_ETCD_TEST_TARGET} PRIVATE million::core million::etcd)
