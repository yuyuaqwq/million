cmake .. ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
    -DProtobuf_ROOT="[protobuf-root-root]" ^
    -DMYSQL_HEADER_DIR="[mysql-root]/include" ^
    -DMYSQL_LIB="[mysql-root]/lib/libmysql.lib" ^
    -DOPENSSL_ROOT_DIR="[openssl-root]" ^
    -DOPENSSL_USE_STATIC_LIBS=TRUE ^
    -DCMAKE_TOOLCHAIN_FILE="[vcpkg-root]/scripts/buildsystems/vcpkg.cmake"
pause