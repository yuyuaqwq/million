@echo off
setlocal enabledelayedexpansion

rem 遍历 .\table 下的所有文件夹
for /d %%D in (.\table\*) do (
    echo Make %%~nxD
    rem 执行 build.bat
    call make.bat %%~nxD
)

endlocal

pause