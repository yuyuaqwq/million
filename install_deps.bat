@echo off
echo Installing dependencies for etcd-cpp-apiv3...

REM 检查是否已安装 vcpkg
where vcpkg >nul 2>&1
if %errorlevel% neq 0 (
    echo vcpkg not found. Please install vcpkg first.
    echo You can download it from: https://github.com/microsoft/vcpkg
    exit /b 1
)

REM 安装 OpenSSL
echo Installing OpenSSL...
vcpkg install openssl:x64-windows

REM 安装 gRPC
echo Installing gRPC...
vcpkg install grpc:x64-windows

REM 安装 Protobuf
echo Installing Protobuf...
vcpkg install protobuf:x64-windows

echo Dependencies installed successfully!
echo Please set the following environment variables:
echo OPENSSL_ROOT_DIR to the OpenSSL installation directory
echo VCPKG_ROOT to your vcpkg installation directory