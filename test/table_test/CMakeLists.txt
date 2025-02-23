set(TABLE_TEST table_test)

link_directories(${LIB_PATH})

add_executable(${TABLE_TEST} table_test.cpp)

target_link_libraries(${TABLE_TEST} PRIVATE ${MILLION_LIB})

target_link_libraries(${TABLE_TEST} PRIVATE ${TABLE_LIB})