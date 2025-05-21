cmake .. ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
    -DProtobuf_ROOT="?" ^
    -DMYSQL_HEADER_DIR="?/mysql-*/include" ^
    -DMYSQL_LIB="?/mysql-*/lib/libmysql.lib"
pause