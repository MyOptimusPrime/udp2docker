@echo off
rem UDP2Docker MSYS2/MinGW构建脚本
rem 适用于Windows上的MSYS2环境

setlocal enabledelayedexpansion

echo ======================================
echo UDP2Docker MSYS2/MinGW构建脚本
echo ======================================

rem 检查MSYS2是否可用
where msys2 >nul 2>&1
if errorlevel 1 (
    echo 注意: 如果找不到msys2命令，请手动启动MSYS2终端并运行build-msys2.sh
    echo 或者将MSYS2添加到PATH中
)

rem 设置构建类型
set BUILD_TYPE=Release
if not "%1"=="" set BUILD_TYPE=%1

echo 构建类型: %BUILD_TYPE%
echo.
echo 请在MSYS2终端中运行以下命令:
echo.
echo # 1. 安装必要的包
echo pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
echo.
echo # 2. 切换到项目目录
echo cd /d/workspace/udp2docker
echo.
echo # 3. 运行构建脚本
echo ./scripts/build-msys2.sh %BUILD_TYPE%
echo.

rem 尝试在MSYS2中运行
if exist "C:\msys64\msys2_shell.cmd" (
    echo 尝试启动MSYS2构建...
    C:\msys64\msys2_shell.cmd -mingw64 -c "cd '%~dp0..' && chmod +x scripts/build-msys2.sh && ./scripts/build-msys2.sh %BUILD_TYPE%"
) else if exist "C:\msys64\usr\bin\bash.exe" (
    echo 尝试使用MSYS2 bash构建...
    C:\msys64\usr\bin\bash.exe -l -c "cd '%~dp0..' && chmod +x scripts/build-msys2.sh && ./scripts/build-msys2.sh %BUILD_TYPE%"
) else (
    echo 请手动在MSYS2终端中执行上述命令
    pause
)

echo 完成! 