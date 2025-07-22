#!/bin/bash

# UDP2Docker MSYS2环境一键设置脚本
# 自动安装所需的包和配置开发环境

echo "======================================"
echo "UDP2Docker MSYS2环境设置脚本"
echo "======================================"

# 检查是否在MSYS2环境中
if [[ -z "$MSYSTEM" ]]; then
    echo "错误: 请在MSYS2终端中运行此脚本"
    echo "启动方式: C:\\msys64\\msys2_shell.cmd -mingw64"
    exit 1
fi

echo "当前环境: $MSYSTEM"
if [[ "$MSYSTEM" != "MINGW64" ]]; then
    echo "警告: 建议在MINGW64环境中运行"
    read -p "继续吗? (y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 更新包数据库
echo "更新包数据库..."
pacman -Sy

# 定义需要安装的包
PACKAGES=(
    "base-devel"
    "mingw-w64-x86_64-toolchain"
    "mingw-w64-x86_64-cmake"
    "mingw-w64-x86_64-ninja"
    "mingw-w64-x86_64-gdb"
    "mingw-w64-x86_64-pkg-config"
    "git"
)

# 检查并安装包
echo "检查并安装必要的包..."
for package in "${PACKAGES[@]}"; do
    if pacman -Qs "$package" > /dev/null; then
        echo "✅ $package 已安装"
    else
        echo "📦 安装 $package..."
        if pacman -S --needed --noconfirm "$package"; then
            echo "✅ $package 安装成功"
        else
            echo "❌ $package 安装失败"
            exit 1
        fi
    fi
done

echo ""
echo "======================================"
echo "验证安装"
echo "======================================"

# 验证关键工具
tools=(
    "gcc:GCC编译器"
    "g++:G++编译器"
    "cmake:CMake构建工具"
    "mingw32-make:Make工具"
    "ninja:Ninja构建工具"
    "gdb:GDB调试器"
)

all_ok=true
for tool_info in "${tools[@]}"; do
    IFS=':' read -r tool desc <<< "$tool_info"
    if command -v "$tool" &> /dev/null; then
        version=$(eval "$tool --version | head -n1")
        echo "✅ $desc: $version"
    else
        echo "❌ 缺少 $desc"
        all_ok=false
    fi
done

if ! $all_ok; then
    echo ""
    echo "❌ 某些工具未正确安装"
    exit 1
fi

echo ""
echo "======================================"
echo "环境配置"
echo "======================================"

# 创建或更新 ~/.bashrc
if [ ! -f ~/.bashrc ]; then
    touch ~/.bashrc
fi

# 添加环境变量（如果不存在）
add_to_bashrc() {
    local line="$1"
    if ! grep -Fxq "$line" ~/.bashrc; then
        echo "$line" >> ~/.bashrc
        echo "添加到 ~/.bashrc: $line"
    fi
}

add_to_bashrc 'export PATH="/mingw64/bin:$PATH"'
add_to_bashrc 'export CC=gcc'
add_to_bashrc 'export CXX=g++'
add_to_bashrc 'export MAKE=mingw32-make'

# 创建有用的别名
add_to_bashrc 'alias ll="ls -la"'
add_to_bashrc 'alias build="./scripts/build-msys2.sh"'
add_to_bashrc 'alias verify="./scripts/verify-build.sh"'

echo ""
echo "======================================"
echo "测试编译"
echo "======================================"

# 创建简单的测试程序
echo "创建测试程序..."
cat > /tmp/test.cpp << 'EOF'
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Hello from C++17!" << std::endl;
    std::cout << "Compiler: " << __VERSION__ << std::endl;
    std::cout << "C++ Standard: " << __cplusplus << std::endl;
    
    // 测试线程支持
    std::thread t([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Thread support: OK" << std::endl;
    });
    t.join();
    
    return 0;
}
EOF

echo "编译测试程序..."
if g++ -std=c++17 -o /tmp/test.exe /tmp/test.cpp -pthread; then
    echo "✅ 编译成功"
    echo "运行测试程序:"
    /tmp/test.exe
    rm -f /tmp/test.exe /tmp/test.cpp
else
    echo "❌ 编译失败"
    rm -f /tmp/test.cpp
    exit 1
fi

echo ""
echo "======================================"
echo "设置完成！"
echo "======================================"
echo ""
echo "🎉 MSYS2环境配置成功！"
echo ""
echo "下一步:"
echo "1. 重新启动终端或运行: source ~/.bashrc"
echo "2. 切换到项目目录: cd /d/workspace/udp2docker"
echo "3. 构建项目: ./scripts/build-msys2.sh Release"
echo "4. 验证构建: ./scripts/verify-build.sh"
echo ""
echo "有用的命令:"
echo "- 构建项目: build Release"
echo "- 验证构建: verify"
echo "- 查看帮助: man gcc"
echo ""
echo "如果遇到问题，请参考: MSYS2_SETUP.md" 