mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=?/vcpkg/scripts/buildsystems/vcpkg.cmake -Dprotobuf_DIR=?/vcpkg/installed/x64-windows/share/protobuf -DMYSQL_HEADER_DIR="?/mysql-*/include" -DMYSQL_INCLUDE_DIR="?/mysql-*/include" -DMYSQL_LIB="?/mysql-*/lib/libmysql.lib"
pause