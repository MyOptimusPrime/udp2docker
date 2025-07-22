@echo off
rem UDP2Docker Windows构建脚本
rem 使用Visual Studio编译器构建项目

setlocal enabledelayedexpansion

echo ======================================
echo UDP2Docker Windows构建脚本
echo ======================================

rem 检查CMake是否可用
cmake --version >nul 2>&1
if errorlevel 1 (
    echo 错误: 找不到CMake，请确保CMake已安装并添加到PATH中
    pause
    exit /b 1
)

rem 设置构建类型
set BUILD_TYPE=Release
if not "%1"=="" set BUILD_TYPE=%1

echo 构建类型: %BUILD_TYPE%

rem 创建构建目录
if not exist build mkdir build
cd build

echo 正在配置CMake项目...

rem 检测Visual Studio版本
set VS_GENERATOR=""
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\" (
    set VS_GENERATOR="Visual Studio 17 2022"
    echo 检测到Visual Studio 2022
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\" (
    set VS_GENERATOR="Visual Studio 16 2019"
    echo 检测到Visual Studio 2019
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\" (
    set VS_GENERATOR="Visual Studio 15 2017"
    echo 检测到Visual Studio 2017
) else (
    echo 警告: 未找到Visual Studio，尝试使用默认生成器
    set VS_GENERATOR=""
)

rem 配置CMake
if not %VS_GENERATOR%=="" (
    cmake .. -G %VS_GENERATOR% -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
) else (
    cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
)

if errorlevel 1 (
    echo 错误: CMake配置失败
    cd ..
    pause
    exit /b 1
)

echo 正在构建项目...

rem 构建项目
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo 错误: 项目构建失败
    cd ..
    pause
    exit /b 1
)

echo.
echo ======================================
echo 构建成功！
echo ======================================
echo.
echo 输出文件位置:
echo   可执行文件: build\bin\%BUILD_TYPE%\
echo   库文件:     build\lib\%BUILD_TYPE%\
echo.
echo 运行示例程序:
echo   cd build\bin\%BUILD_TYPE%
echo   udp2docker.exe
echo.
echo 运行测试:
echo   cd build\bin\%BUILD_TYPE%
echo   udp2docker_test.exe
echo.

cd ..

if "%2"=="--run-tests" (
    echo 正在运行测试...
    build\bin\%BUILD_TYPE%\udp2docker_test.exe
)

if "%2"=="--run-example" (
    echo 正在运行示例...
    build\bin\%BUILD_TYPE%\udp2docker.exe
)

pause 