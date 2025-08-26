mkdir build
cd build
cmake .. ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
::    -DMYSQL_INCLUDE_DIR="[mysql-root]/include" ^
::    -DProtobuf_ROOT="[protobuf-root-root]" ^
::    -DOPENSSL_ROOT_DIR="[openssl-root]" ^
    -DCMAKE_TOOLCHAIN_FILE="[vcpkg-root]/scripts/buildsystems/vcpkg.cmake"
pause