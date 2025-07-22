# UDP2Docker 快速开始指南

本指南帮助你在5分钟内快速搭建和运行UDP2Docker通信系统。

## 🚀 一键启动

### Windows用户

#### 选项1：使用Visual Studio
```batch
# 1. 构建项目
.\scripts\build.bat Release

# 2. 启动Docker服务器
docker-compose up -d

# 3. 运行示例客户端
.\build\bin\Release\udp2docker.exe
```

#### 选项2：使用MSYS2（推荐轻量级方案）
```bash
# 1. 启动MSYS2 MinGW64终端
C:\msys64\msys2_shell.cmd -mingw64

# 2. 安装必要工具
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake

# 3. 切换到项目目录
cd /d/workspace/udp2docker

# 4. 构建项目
chmod +x scripts/build-msys2.sh
./scripts/build-msys2.sh Release

# 5. 启动Docker服务器
docker-compose up -d

# 6. 运行示例客户端
./build/bin/udp2docker.exe
```

### Linux用户

```bash
# 1. 构建项目
chmod +x scripts/build.sh
./scripts/build.sh Release

# 2. 启动Docker服务器
docker-compose up -d

# 3. 运行示例客户端
./build/bin/udp2docker
```

## 🧪 验证安装

### 1. 检查Docker服务器状态
```bash
docker-compose ps
docker-compose logs udp2docker-server
```

### 2. 发送测试消息
```bash
# 使用netcat发送简单消息（Linux）
echo "Hello Docker!" | nc -u localhost 8888

# 使用PowerShell发送消息（Windows）
$socket = New-Object System.Net.Sockets.UdpClient
$endpoint = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Parse("127.0.0.1"), 8888)
$message = [System.Text.Encoding]::UTF8.GetBytes("Hello Docker!")
$socket.Send($message, $message.Length, $endpoint)
$socket.Close()
```

### 3. 查看服务器日志
```bash
docker-compose logs -f udp2docker-server
```

## 📝 基本用法

### C++客户端代码
```cpp
#include "udp2docker/udp_client.h"

using namespace udp2docker;

int main() {
    UdpConfig config;
    config.server_host = "127.0.0.1";
    config.server_port = 8888;
    
    UdpClient client(config);
    client.initialize();
    
    client.send_string("Hello from C++!");
    
    client.close();
    return 0;
}
```

## 🔧 自定义配置

### 修改端口
```bash
# 修改docker-compose.yml中的端口配置
ports:
  - "9999:9999/udp"
  
# 修改环境变量
environment:
  - UDP_PORT=9999
```

### 修改日志级别
```bash
environment:
  - LOG_LEVEL=DEBUG
```

## 🐛 常见问题

### Windows防火墙
```batch
# 允许UDP通信
netsh advfirewall firewall add rule name="UDP2Docker" protocol=UDP dir=in localport=8888 action=allow
```

### Docker未启动
```bash
# 启动Docker服务
systemctl start docker  # Linux
# 或手动启动Docker Desktop（Windows）
```

### 端口被占用
```bash
# 检查端口占用
netstat -ano | findstr :8888    # Windows
lsof -i :8888                   # Linux

# 修改端口配置
vim docker-compose.yml
```

## ✅ 下一步

1. 查看 [README.md](README.md) 了解完整功能
2. 查看 [examples/](examples/) 目录的示例代码
3. 运行 [tests/](tests/) 目录的测试程序
4. 根据需要修改 [config/default.ini](config/default.ini) 配置

## 📞 需要帮助？

- 📚 查看完整文档：[README.md](README.md)
- 🐛 报告问题：[GitHub Issues](https://github.com/your-org/udp2docker/issues)
- 💬 讨论交流：[GitHub Discussions](https://github.com/your-org/udp2docker/discussions) 