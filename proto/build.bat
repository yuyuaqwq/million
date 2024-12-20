:: @echo off
set OUTPUT_DIR=..\protogen
set EXPORT_MACRO=MILLION_PROTOGEN_API

:: 创建输出目录，如果已存在则跳过
mkdir %OUTPUT_DIR% 2>nul

:: 遍历当前目录的所有子文件夹并生成 C++ 文件
for /d %%D in (*) do (
    protoc --cpp_out=dllexport_decl=%EXPORT_MACRO%:%OUTPUT_DIR% .\%%D\*.proto
)

pause