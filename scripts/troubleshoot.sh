#!/bin/bash

# UDP2Docker故障排除脚本
# 帮助诊断常见的构建和运行问题

echo "======================================"
echo "UDP2Docker 故障排除诊断脚本"
echo "======================================"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

issues_found=0

print_section() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
}

print_ok() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
    ((issues_found++))
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
    ((issues_found++))
}

print_section "系统环境检查"

# 检查操作系统
if [[ "$OSTYPE" == "msys" ]]; then
    print_ok "运行在MSYS2环境中"
    if [[ "$MSYSTEM" == "MINGW64" ]]; then
        print_ok "使用MINGW64环境"
    else
        print_warning "不是MINGW64环境，当前: $MSYSTEM"
        echo "  建议: 使用 'C:\msys64\msys2_shell.cmd -mingw64' 启动"
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    print_ok "运行在Linux环境中"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    print_ok "运行在macOS环境中"
else
    print_warning "未知操作系统: $OSTYPE"
fi

print_section "编译工具检查"

# 检查编译器
compilers=("gcc" "g++" "cmake")
for compiler in "${compilers[@]}"; do
    if command -v "$compiler" &> /dev/null; then
        version=$($compiler --version | head -n1)
        print_ok "$compiler: $version"
        
        # 检查GCC版本
        if [[ "$compiler" == "gcc" ]]; then
            gcc_version=$(gcc -dumpversion | cut -d. -f1)
            if [[ "$gcc_version" -ge 7 ]]; then
                print_ok "GCC版本支持C++17"
            else
                print_error "GCC版本过低 ($gcc_version)，需要7+支持C++17"
            fi
        fi
    else
        print_error "未找到 $compiler"
        case "$compiler" in
            "gcc"|"g++")
                if [[ "$OSTYPE" == "msys" ]]; then
                    echo "  解决方案: pacman -S mingw-w64-x86_64-toolchain"
                else
                    echo "  解决方案: sudo apt-get install build-essential (Ubuntu/Debian)"
                    echo "           yum install gcc-c++ (CentOS/RHEL)"
                fi
                ;;
            "cmake")
                if [[ "$OSTYPE" == "msys" ]]; then
                    echo "  解决方案: pacman -S mingw-w64-x86_64-cmake"
                else
                    echo "  解决方案: sudo apt-get install cmake"
                fi
                ;;
        esac
    fi
done

# 检查Make工具
make_tools=("make" "mingw32-make" "ninja")
make_found=false
for make_tool in "${make_tools[@]}"; do
    if command -v "$make_tool" &> /dev/null; then
        print_ok "构建工具: $make_tool"
        make_found=true
        break
    fi
done

if ! $make_found; then
    print_error "未找到构建工具 (make/mingw32-make/ninja)"
    echo "  解决方案: pacman -S base-devel mingw-w64-x86_64-ninja"
fi

print_section "项目结构检查"

# 检查关键文件
key_files=("CMakeLists.txt" "include/udp2docker/common.h" "src/udp_client.cpp")
for file in "${key_files[@]}"; do
    if [[ -f "$file" ]]; then
        print_ok "找到文件: $file"
    else
        print_error "缺少文件: $file"
        if [[ "$file" == "CMakeLists.txt" ]]; then
            echo "  请确保在项目根目录中运行此脚本"
        fi
    fi
done

# 检查构建脚本
build_scripts=("scripts/build.sh" "scripts/build-msys2.sh")
for script in "${build_scripts[@]}"; do
    if [[ -f "$script" ]]; then
        if [[ -x "$script" ]]; then
            print_ok "构建脚本可执行: $script"
        else
            print_warning "构建脚本不可执行: $script"
            echo "  解决方案: chmod +x $script"
        fi
    else
        print_warning "缺少构建脚本: $script"
    fi
done

print_section "依赖库检查"

# Windows特定检查
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    # 检查Windows Socket库
    if gcc -lws2_32 -lwsock32 2>/dev/null -xc - -o /tmp/test_socket <<< 'int main(){return 0;}'; then
        print_ok "Windows Socket库可用"
        rm -f /tmp/test_socket
    else
        print_error "Windows Socket库链接失败"
        echo "  这通常表示MinGW安装有问题"
    fi
    
    # 检查pthread支持
    if gcc -pthread 2>/dev/null -xc - -o /tmp/test_pthread <<< '#include <pthread.h>
    int main(){pthread_t t; return 0;}'; then
        print_ok "pthread库可用"
        rm -f /tmp/test_pthread
    else
        print_warning "pthread库可能有问题"
    fi
fi

print_section "构建目录检查"

if [[ -d "build" ]]; then
    print_ok "构建目录存在"
    
    # 检查构建产物
    if [[ -f "build/bin/udp2docker.exe" ]] || [[ -f "build/bin/udp2docker" ]]; then
        print_ok "找到主程序"
    else
        print_warning "未找到主程序，可能需要重新构建"
    fi
    
    if [[ -f "build/lib/libudp2docker_lib.a" ]]; then
        print_ok "找到静态库"
    else
        print_warning "未找到静态库"
    fi
    
    # 检查构建缓存
    if [[ -f "build/CMakeCache.txt" ]]; then
        echo "构建配置信息:"
        grep -E "(CMAKE_BUILD_TYPE|CMAKE_CXX_COMPILER)" build/CMakeCache.txt | head -5
    fi
else
    print_warning "构建目录不存在"
    echo "  运行构建脚本将创建此目录"
fi

print_section "网络和Docker检查"

# 检查Docker
if command -v docker &> /dev/null; then
    if docker --version &> /dev/null; then
        docker_version=$(docker --version)
        print_ok "Docker: $docker_version"
        
        # 检查Docker是否运行
        if docker ps &> /dev/null; then
            print_ok "Docker服务正在运行"
        else
            print_warning "Docker服务未运行或权限不足"
            echo "  解决方案: 启动Docker Desktop或sudo systemctl start docker"
        fi
    else
        print_error "Docker命令执行失败"
    fi
else
    print_warning "未安装Docker"
    echo "  如果不需要Docker服务器测试，可以忽略此警告"
fi

# 检查网络端口
if command -v netstat &> /dev/null; then
    if netstat -an 2>/dev/null | grep -q ":8888"; then
        print_warning "端口8888已被占用"
        echo "  可能的解决方案:"
        echo "  - 修改docker-compose.yml中的端口配置"
        echo "  - 停止占用端口的服务"
    else
        print_ok "默认端口8888可用"
    fi
fi

print_section "常见问题和解决方案"

echo "以下是一些常见问题的解决方案:"
echo ""
echo "1. 编译错误: 'filesystem' not found"
echo "   解决方案: 项目已包含兼容性代码，确保使用GCC 7+"
echo ""
echo "2. 链接错误: undefined reference to 'WSAStartup'"
echo "   解决方案: 项目自动链接ws2_32，检查MinGW安装"
echo ""
echo "3. CMake错误: Could not find compiler"
echo "   解决方案: 确保在MINGW64环境中运行，安装toolchain"
echo ""
echo "4. 权限错误: Permission denied"
echo "   解决方案: chmod +x scripts/*.sh"
echo ""
echo "5. Docker连接失败"
echo "   解决方案: 启动Docker Desktop，检查防火墙设置"

print_section "诊断总结"

echo "发现的问题数量: $issues_found"

if [[ $issues_found -eq 0 ]]; then
    echo -e "${GREEN}🎉 没有发现问题！环境配置良好。${NC}"
    echo ""
    echo "建议的下一步:"
    echo "1. 运行构建脚本: ./scripts/build-msys2.sh Release"
    echo "2. 验证构建结果: ./scripts/verify-build.sh"
    echo "3. 启动Docker服务器: docker-compose up -d"
    echo "4. 运行示例程序: ./build/bin/udp2docker.exe"
elif [[ $issues_found -le 3 ]]; then
    echo -e "${YELLOW}⚠️  发现少量问题，但通常可以继续。${NC}"
    echo "建议先尝试构建，如果失败再解决这些问题。"
else
    echo -e "${RED}❌ 发现多个问题，建议先解决这些问题再继续。${NC}"
    echo ""
    echo "优先解决的问题:"
    echo "1. 安装缺失的编译工具"
    echo "2. 确保在正确的环境中运行(MINGW64)"
    echo "3. 检查文件权限"
fi

echo ""
echo "获取更多帮助:"
echo "- 详细配置指南: MSYS2_SETUP.md"
echo "- 快速开始: QUICKSTART.md"
echo "- 完整文档: README.md" 