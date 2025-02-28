set(CONFIG_TEST config_test)

link_directories(${LIB_PATH})

add_executable(${CONFIG_TEST} config_test.cpp)

target_link_libraries(${CONFIG_TEST} PRIVATE ${MILLION_LIB})

target_link_libraries(${CONFIG_TEST} PRIVATE ${CONFIG_LIB})