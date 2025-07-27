cmake .. ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
::    -DProtobuf_ROOT="[protobuf-root-root]" ^
::    -DMYSQL_INCLUDE_DIR="[mysql-root]/include" ^
::    -DMYSQL_LIBRARIES="[mysql-root]/lib/libmysql.lib" ^
::    -DOPENSSL_ROOT_DIR="[openssl-root]" ^
    -DCMAKE_TOOLCHAIN_FILE="[vcpkg-root]/scripts/buildsystems/vcpkg.cmake"
pause