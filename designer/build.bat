@echo off
setlocal enabledelayedexpansion

rem ���� .\table �µ������ļ���
for /d %%D in (.\table\*) do (
    echo Make %%~nxD
    rem ִ�� build.bat
    call make.bat %%~nxD
)

endlocal

pause