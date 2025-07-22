#!/bin/bash

# UDP2Docker Linux构建脚本
# 使用GCC或Clang编译器构建项目

set -e  # 遇到错误立即退出

echo "======================================"
echo "UDP2Docker Linux构建脚本"
echo "======================================"

# 检查CMake是否可用
if ! command -v cmake &> /dev/null; then
    echo "错误: 找不到CMake，请先安装CMake"
    echo "Ubuntu/Debian: sudo apt-get install cmake"
    echo "CentOS/RHEL: sudo yum install cmake"
    echo "macOS: brew install cmake"
    exit 1
fi

# 设置构建类型
BUILD_TYPE=${1:-Release}
echo "构建类型: $BUILD_TYPE"

# 检测编译器
if command -v g++ &> /dev/null; then
    echo "检测到编译器: GCC $(g++ --version | head -n1)"
    COMPILER="-DCMAKE_CXX_COMPILER=g++"
elif command -v clang++ &> /dev/null; then
    echo "检测到编译器: Clang $(clang++ --version | head -n1)"
    COMPILER="-DCMAKE_CXX_COMPILER=clang++"
else
    echo "错误: 找不到C++编译器，请安装g++或clang++"
    exit 1
fi

# 获取CPU核心数
CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "使用 $CORES 个并行任务"

# 创建构建目录
mkdir -p build
cd build

echo "正在配置CMake项目..."

# 配置CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    $COMPILER \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "正在构建项目..."

# 构建项目
cmake --build . --parallel $CORES

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
echo "  cd build/bin"
echo "  ./udp2docker"
echo ""
echo "运行测试:"
echo "  cd build/bin"
echo "  ./udp2docker_test"
echo ""

cd ..

# 可选运行测试
if [ "$2" == "--run-tests" ]; then
    echo "正在运行测试..."
    ./build/bin/udp2docker_test
fi

# 可选运行示例
if [ "$2" == "--run-example" ]; then
    echo "正在运行示例..."
    ./build/bin/udp2docker
fi

echo "构建完成！" 