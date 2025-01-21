@echo off
setlocal enabledelayedexpansion

:: 检查是否传入参数
if "%1"=="" (
    echo Please provide the package name as the first argument.
    exit /b
)

cd .\table\%1

:: 设置基本参数，使用传入的参数替代
set PACKAGE=million.%1
set TOOL_PATH=..\..\tool\tabtoy
set INDEX_PATH=Index.xlsx
set PROTO_OUT=../../../proto/table/%1.proto
set PBBIN_OUT=../../../tablegen/%1.pbb
set PBBIN_DIR=..\..\..\tablegen\%1

:: 生成 Proto 文件
echo Generating Proto file for %PACKAGE%...
%TOOL_PATH% -mode=v3 -index=%INDEX_PATH% -package=%PACKAGE% -proto_out=%PROTO_OUT%

:: 生成二进制 PBB 文件
:: echo Generating PBB binary file...
:: %TOOL_PATH% -mode=v3 -index=%INDEX_PATH% -package=%PACKAGE% -pbbin_out=%PBBIN_OUT%

:: 创建目录
echo Creating directory for generated tables...
mkdir %PBBIN_DIR%

:: 生成 PBB 文件到指定目录
echo Generating PBB binary files in directory %PBBIN_DIR%...
%TOOL_PATH% -mode=v3 -index=%INDEX_PATH% -package=%PACKAGE% -pbbin_dir=%PBBIN_DIR%

endlocal
