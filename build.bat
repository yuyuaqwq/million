mkdir build
cd build
cmake .. ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
    -DMYSQL_INCLUDE_DIR="C:\Users\Administrator\Desktop\projects\vcpkg\installed\x64-windows\include\mysql" ^
    -DCMAKE_TOOLCHAIN_FILE="C:\Users\Administrator\Desktop\projects\vcpkg\scripts\buildsystems\vcpkg.cmake"
pause