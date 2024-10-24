cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/lib/vcpkg/scripts/buildsystems/vcpkg.cmake -Dprotobuf_DIR=C:/lib/vcpkg/installed/x64-windows/share/protobuf -DMYSQL_HEADER="C:\Program Files\mysql-5.7.44-winx64\include" -DMYSQL_LIB="C:\Program Files\mysql-5.7.44-winx64\lib/libmysql.lib"
pause