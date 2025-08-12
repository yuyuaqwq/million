set(ETCD_TEST etcd_test)

link_directories(${LIB_PATH})

add_executable(${ETCD_TEST} etcd_test.cpp)

target_link_libraries(${ETCD_TEST} PRIVATE ${MILLION_LIB})

target_link_libraries(${ETCD_TEST} PRIVATE ${ETCD_LIB})
