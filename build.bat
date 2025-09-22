mkdir build
cd build
cmake .. ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
    -DMYSQL_INCLUDE_DIR="D:\project\mmo-pokemon-server2\million\build\vcpkg_installed\x64-windows\include\mysql" ^
    -DCMAKE_TOOLCHAIN_FILE="C:\src\vcpkg\scripts\buildsystems\vcpkg.cmake"
pause                 