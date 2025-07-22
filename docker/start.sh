#!/bin/bash

echo "======================================"
echo "启动UDP2Docker服务器"
echo "======================================"

# 显示环境信息
echo "环境配置:"
echo "  主机: ${UDP_HOST:-0.0.0.0}"
echo "  端口: ${UDP_PORT:-8888}"
echo "  日志级别: ${LOG_LEVEL:-INFO}"
echo ""

# 检查网络连接性
echo "网络检查:"
echo "  容器IP: $(hostname -i)"
echo "  监听端口: ${UDP_PORT:-8888}/udp"
echo ""

# 创建日志目录
mkdir -p /app/logs

# 显示系统信息
echo "系统信息:"
echo "  操作系统: $(cat /etc/os-release | grep PRETTY_NAME | cut -d '"' -f2)"
echo "  Python版本: $(python3 --version)"
echo "  容器时间: $(date)"
echo ""

echo "======================================"
echo "启动UDP服务器..."
echo "======================================"

# 启动UDP服务器
exec python3 /app/udp_server.py 