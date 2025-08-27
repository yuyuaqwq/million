set(MILLION_DB_TEST_TARGET million_db_test)

add_executable(${MILLION_DB_TEST_TARGET} db_test.cpp)

target_link_libraries(${MILLION_DB_TEST_TARGET} PRIVATE million::core million::db)