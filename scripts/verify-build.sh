#!/bin/bash

# UDP2Docker构建验证脚本
# 用于验证编译结果是否正确

echo "======================================"
echo "UDP2Docker 构建验证脚本"
echo "======================================"

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

BUILD_DIR="build"

# 检查构建目录
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}❌ 构建目录不存在：$BUILD_DIR${NC}"
    echo "请先运行构建脚本"
    exit 1
fi

echo "检查构建产物..."

# 检查可执行文件
EXECUTABLES=("udp2docker.exe" "udp2docker_test.exe")
LIB_FILES=("libudp2docker_lib.a")

success_count=0
total_checks=0

# 检查可执行文件
for exe in "${EXECUTABLES[@]}"; do
    ((total_checks++))
    exe_path="$BUILD_DIR/bin/$exe"
    if [ -f "$exe_path" ]; then
        echo -e "${GREEN}✅ 找到可执行文件: $exe_path${NC}"
        
        # 检查文件大小
        size=$(stat -f%z "$exe_path" 2>/dev/null || stat -c%s "$exe_path" 2>/dev/null)
        if [ "$size" -gt 1000 ]; then
            echo "   文件大小: $(($size / 1024)) KB"
            ((success_count++))
        else
            echo -e "${YELLOW}⚠️  文件太小，可能编译不完整${NC}"
        fi
    else
        echo -e "${RED}❌ 缺少可执行文件: $exe_path${NC}"
    fi
done

# 检查库文件
for lib in "${LIB_FILES[@]}"; do
    ((total_checks++))
    lib_path="$BUILD_DIR/lib/$lib"
    if [ -f "$lib_path" ]; then
        echo -e "${GREEN}✅ 找到库文件: $lib_path${NC}"
        
        # 检查文件大小
        size=$(stat -f%z "$lib_path" 2>/dev/null || stat -c%s "$lib_path" 2>/dev/null)
        if [ "$size" -gt 10000 ]; then
            echo "   文件大小: $(($size / 1024)) KB"
            ((success_count++))
        else
            echo -e "${YELLOW}⚠️  库文件太小${NC}"
        fi
    else
        echo -e "${RED}❌ 缺少库文件: $lib_path${NC}"
    fi
done

echo ""
echo "======================================"
echo "运行快速测试..."
echo "======================================"

# 运行测试程序
if [ -f "$BUILD_DIR/bin/udp2docker_test.exe" ]; then
    echo "运行单元测试..."
    cd "$BUILD_DIR/bin"
    
    if timeout 30s ./udp2docker_test.exe; then
        echo -e "${GREEN}✅ 单元测试通过${NC}"
        ((success_count++))
    else
        echo -e "${RED}❌ 单元测试失败${NC}"
    fi
    cd - > /dev/null
    ((total_checks++))
else
    echo -e "${YELLOW}⚠️  跳过单元测试（找不到测试程序）${NC}"
fi

# 检查依赖库
echo ""
echo "检查依赖库..."
if [ -f "$BUILD_DIR/bin/udp2docker.exe" ]; then
    if command -v ldd &> /dev/null; then
        echo "依赖关系:"
        ldd "$BUILD_DIR/bin/udp2docker.exe" | head -10
    elif command -v objdump &> /dev/null; then
        echo "依赖关系:"
        objdump -p "$BUILD_DIR/bin/udp2docker.exe" | grep "DLL Name" | head -5
    fi
fi

echo ""
echo "======================================"
echo "验证结果"
echo "======================================"

echo "成功项目: $success_count / $total_checks"

if [ $success_count -eq $total_checks ]; then
    echo -e "${GREEN}🎉 所有检查通过！构建成功！${NC}"
    echo ""
    echo "下一步:"
    echo "1. 运行示例程序: ./build/bin/udp2docker.exe"
    echo "2. 启动Docker服务器: docker-compose up -d"
    echo "3. 测试UDP通信功能"
    exit 0
elif [ $success_count -gt $(($total_checks / 2)) ]; then
    echo -e "${YELLOW}⚠️  部分检查通过，构建可能有问题${NC}"
    exit 1
else
    echo -e "${RED}❌ 多项检查失败，构建有严重问题${NC}"
    echo ""
    echo "建议："
    echo "1. 检查编译错误日志"
    echo "2. 确认所有依赖已安装"
    echo "3. 尝试清理重新构建"
    exit 2
fi 