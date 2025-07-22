#!/bin/bash

# UDP2Docker 一键快速开始脚本
# 自动检测环境并执行完整的构建流程

echo "🚀 UDP2Docker 一键快速开始"
echo "============================"

# 检查是否在项目根目录
if [[ ! -f "CMakeLists.txt" ]]; then
    echo "❌ 请在项目根目录中运行此脚本"
    echo "当前目录: $(pwd)"
    echo "正确的目录应包含 CMakeLists.txt 文件"
    exit 1
fi

echo "✅ 在正确的项目目录中"

# 检查操作系统和环境
if [[ "$OSTYPE" == "msys" ]]; then
    echo "🪟 检测到MSYS2环境"
    
    if [[ "$MSYSTEM" != "MINGW64" ]]; then
        echo "⚠️  警告: 当前不在MINGW64环境中"
        echo "当前环境: $MSYSTEM"
        echo "建议使用: C:\\msys64\\msys2_shell.cmd -mingw64"
        read -p "继续吗? (y/n): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
    
    BUILD_SCRIPT="./scripts/build-msys2.sh"
    SETUP_SCRIPT="./scripts/setup-msys2-env.sh"
    
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "🐧 检测到Linux环境"
    BUILD_SCRIPT="./scripts/build.sh"
    SETUP_SCRIPT=""
    
else
    echo "❓ 未识别的操作系统: $OSTYPE"
    echo "请手动选择构建脚本"
    exit 1
fi

# 设置所有脚本权限
echo "🔒 设置脚本权限..."
chmod +x scripts/*.sh

# 步骤1: 检查工具是否已安装
echo ""
echo "📋 检查必要工具..."

missing_tools=()
if ! command -v gcc &> /dev/null; then
    missing_tools+=("gcc")
fi
if ! command -v cmake &> /dev/null; then
    missing_tools+=("cmake")
fi

if [[ ${#missing_tools[@]} -gt 0 ]]; then
    echo "❌ 缺少必要工具: ${missing_tools[*]}"
    
    if [[ -n "$SETUP_SCRIPT" && -f "$SETUP_SCRIPT" ]]; then
        echo "🔧 自动安装工具..."
        echo "运行: $SETUP_SCRIPT"
        
        if $SETUP_SCRIPT; then
            echo "✅ 工具安装完成"
            echo "♻️  重新加载环境..."
            source ~/.bashrc 2>/dev/null || true
        else
            echo "❌ 工具安装失败"
            echo "请参考 FIRST_BUILD_GUIDE.md 手动安装"
            exit 1
        fi
    else
        echo "请手动安装缺少的工具："
        for tool in "${missing_tools[@]}"; do
            echo "  - $tool"
        done
        exit 1
    fi
else
    echo "✅ 所有必要工具已安装"
fi

# 步骤2: 检查现有构建
if [[ -d "build" ]]; then
    echo ""
    echo "📁 发现现有构建目录"
    read -p "是否清理重新构建? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "🧹 清理构建目录..."
        rm -rf build/
    fi
fi

# 步骤3: 执行构建
echo ""
echo "🔨 开始构建..."
echo "运行: $BUILD_SCRIPT Release"

if $BUILD_SCRIPT Release; then
    echo ""
    echo "🎉 构建成功！"
else
    echo ""
    echo "❌ 构建失败"
    echo ""
    echo "📋 故障排除建议:"
    echo "1. 运行诊断脚本: ./scripts/troubleshoot.sh"
    echo "2. 查看详细指南: FIRST_BUILD_GUIDE.md"
    echo "3. 检查错误日志以获取更多信息"
    exit 1
fi

# 步骤4: 验证构建结果
echo ""
echo "🔍 验证构建结果..."

if [[ -f "scripts/verify-build.sh" ]]; then
    chmod +x scripts/verify-build.sh
    if ./scripts/verify-build.sh; then
        echo "✅ 构建验证成功"
    else
        echo "⚠️  构建验证有警告，但构建可能仍然可用"
    fi
fi

# 步骤5: 运行快速测试
echo ""
echo "🧪 运行快速测试..."

if [[ -f "build/bin/test_udp2docker.exe" ]]; then
    TEST_PROGRAM="./build/bin/test_udp2docker.exe"
elif [[ -f "build/bin/test_udp2docker" ]]; then
    TEST_PROGRAM="./build/bin/test_udp2docker"
else
    echo "⚠️  未找到测试程序"
    TEST_PROGRAM=""
fi

if [[ -n "$TEST_PROGRAM" ]]; then
    echo "运行单元测试..."
    if $TEST_PROGRAM; then
        echo "✅ 单元测试通过"
    else
        echo "⚠️  单元测试有失败，但主程序可能仍然可用"
    fi
fi

# 步骤6: 显示结果和下一步
echo ""
echo "=============================="
echo "🎊 快速开始完成！"
echo "=============================="
echo ""

# 检查生成的文件
if [[ -f "build/bin/udp2docker.exe" ]]; then
    MAIN_PROGRAM="./build/bin/udp2docker.exe"
elif [[ -f "build/bin/udp2docker" ]]; then
    MAIN_PROGRAM="./build/bin/udp2docker"
else
    MAIN_PROGRAM=""
fi

if [[ -n "$MAIN_PROGRAM" ]]; then
    echo "✅ 主程序: $MAIN_PROGRAM"
    echo ""
    echo "📋 现在你可以："
    echo "1. 运行主程序: $MAIN_PROGRAM"
    echo "2. 修改配置: config/default.ini"
    echo "3. 启动Docker服务器: docker-compose up -d"
    echo "4. 查看示例代码: examples/main.cpp"
else
    echo "❌ 主程序未找到，构建可能有问题"
fi

echo ""
echo "📚 有用的资源:"
echo "- 详细使用指南: README.md"
echo "- 快速开始: QUICKSTART.md"  
echo "- 首次构建指南: FIRST_BUILD_GUIDE.md"
echo "- 故障排除: ./scripts/troubleshoot.sh"
echo ""

# 询问是否运行程序
if [[ -n "$MAIN_PROGRAM" ]]; then
    read -p "现在运行主程序吗? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "🚀 启动主程序..."
        echo "================================"
        $MAIN_PROGRAM
    fi
fi

echo ""
echo "感谢使用 UDP2Docker! 🎉" 