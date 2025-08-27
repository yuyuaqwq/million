set(MILLION_CONFIG_TEST_TARGET million_config_test)

add_executable(${MILLION_CONFIG_TEST_TARGET} config_test.cpp)

target_link_libraries(${MILLION_CONFIG_TEST_TARGET} PRIVATE million::core million::config)