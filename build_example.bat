mkdir build
cd build
cmake .. ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.15 ^
::    -DMYSQL_INCLUDE_DIR="[mysql-root]/include/mysql" ^
    -DCMAKE_TOOLCHAIN_FILE="[vcpkg-root]/scripts/buildsystems/vcpkg.cmake"
pause