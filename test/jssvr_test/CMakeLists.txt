set(JSSVR_TEST jssvr_test)

link_directories(${LIB_PATH})

add_executable(${JSSVR_TEST} jssvr_test.cpp)

target_link_libraries(${JSSVR_TEST} PRIVATE ${MILLION_LIB})

target_link_libraries(${JSSVR_TEST} PRIVATE ${JSSVR_LIB})