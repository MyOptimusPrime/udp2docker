#!/bin/bash

# UDP2Docker MSYS2/MinGW构建脚本
# 适用于Windows上的MSYS2环境

set -e  # 遇到错误立即退出

echo "======================================"
echo "UDP2Docker MSYS2/MinGW构建脚本"
echo "======================================"

# 检查是否在MINGW64环境中
if [[ "$MSYSTEM" != "MINGW64" ]]; then
    echo "警告: 建议在MINGW64环境中运行此脚本"
    echo "请使用: msys2_shell.cmd -mingw64"
fi

# 设置构建类型
BUILD_TYPE=${1:-Release}
echo "构建类型: $BUILD_TYPE"

# 检查必要的工具
echo "检查构建环境..."

if ! command -v gcc &> /dev/null; then
    echo "错误: 找不到GCC编译器"
    echo "请安装: pacman -S mingw-w64-x86_64-toolchain"
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "错误: 找不到CMake"
    echo "请安装: pacman -S mingw-w64-x86_64-cmake"
    exit 1
fi

# 显示编译器信息
echo "编译器信息:"
echo "  GCC版本: $(gcc --version | head -n1)"
echo "  CMake版本: $(cmake --version | head -n1)"

# 设置编译器环境变量
export CC=gcc
export CXX=g++

# 获取CPU核心数
if command -v nproc &> /dev/null; then
    CORES=$(nproc)
else
    CORES=4
fi
echo "使用 $CORES 个并行任务"

# 创建构建目录
mkdir -p build
cd build

echo "正在配置CMake项目..."

# 配置CMake（使用MinGW Makefiles生成器）
cmake .. \
    -G "MinGW Makefiles" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_MAKE_PROGRAM=mingw32-make

if [ $? -ne 0 ]; then
    echo "错误: CMake配置失败"
    exit 1
fi

echo "正在构建项目..."

# 使用mingw32-make构建项目
mingw32-make -j$CORES

if [ $? -ne 0 ]; then
    echo "错误: 项目构建失败"
    exit 1
fi

echo ""
echo "======================================"
echo "构建成功！"
echo "======================================"
echo ""
echo "输出文件位置:"
echo "  可执行文件: build/bin/"
echo "  库文件:     build/lib/"
echo ""
echo "运行示例程序:"
echo "  ./bin/udp2docker.exe"
echo ""
echo "运行测试:"
echo "  ./bin/udp2docker_test.exe"
echo ""

cd ..

# 可选运行测试
if [ "$2" == "--run-tests" ]; then
    echo "正在运行测试..."
    ./build/bin/udp2docker_test.exe
fi

# 可选运行示例
if [ "$2" == "--run-example" ]; then
    echo "正在运行示例..."
    ./build/bin/udp2docker.exe
fi

echo "构建完成！" 