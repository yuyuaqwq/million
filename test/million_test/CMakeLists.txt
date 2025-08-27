set(MILLION_TEST_TARGET million_test)

add_executable(${MILLION_TEST_TARGET} million_test.cpp)

target_link_libraries(${MILLION_TEST_TARGET} PRIVATE million::core million::gateway)