:: @echo off
set OUTPUT_DIR=..\protogen
set EXPORT_MACRO=PROTOGEN_API
set TEMP_DIR=%OUTPUT_DIR%\temp

:: 创建输出目录，如果已存在则跳过
mkdir %OUTPUT_DIR% 2>nul
mkdir %TEMP_DIR% 2>nul

:: 创建子目录，分别用于存放头文件和源文件
mkdir %OUTPUT_DIR%\include 2>nul
mkdir %OUTPUT_DIR%\src 2>nul

:: 遍历当前目录的所有子文件夹并生成 C++ 文件
for /d %%D in (*) do (
    protoc --cpp_out=dllexport_decl=%EXPORT_MACRO%:%TEMP_DIR% .\%%D\*.proto
)

:: 遍历 temp 目录，移动头文件和源文件
for /d %%d in ("%TEMP_DIR%\*") do (
    if exist "%%d\*.h" (
        if not exist "%OUTPUT_DIR%\include\%%~nd" (
            mkdir "%OUTPUT_DIR%\include\%%~nd"
        )
        xcopy "%%d\*.h" "%OUTPUT_DIR%\include\%%~nd" /s /e /y
    )
)

for /d %%d in ("%TEMP_DIR%\*") do (
    if exist "%%d\*.cc" (
        if not exist "%OUTPUT_DIR%\src\%%~nd" (
            mkdir "%OUTPUT_DIR%\src\%%~nd"
        )
        xcopy "%%d\*.cc" "%OUTPUT_DIR%\src\%%~nd" /s /e /y
    )
)

:: 删除临时目录
rmdir /s /q %TEMP_DIR%

pause