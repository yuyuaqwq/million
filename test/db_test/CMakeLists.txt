set(DB_TEST db_test)

link_directories(${LIB_PATH})

add_executable(${DB_TEST} db_test.cpp)

target_link_libraries(${DB_TEST} PRIVATE ${MILLION_LIB})

target_link_libraries(${DB_TEST} PRIVATE ${DB_LIB})