set(MILLION_JSSVR_TEST_TARGET million_jssvr_test)

add_executable(${MILLION_JSSVR_TEST_TARGET} jssvr_test.cpp)

target_link_libraries(${MILLION_JSSVR_TEST_TARGET} PRIVATE million::core million::jssvr)