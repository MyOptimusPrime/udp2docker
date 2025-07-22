# 🚀 UDP2Docker 首次构建指南

欢迎！这个指南将帮助你在Windows MSYS2环境中完成UDP2Docker项目的第一次构建。

## 📋 前提条件

确保你已经安装了MSYS2。如果没有，请先访问 https://www.msys2.org/ 下载安装。

## 🔥 开始构建（分步骤）

### 步骤1: 打开MSYS2终端

1. **启动MSYS2 MinGW64终端**：
   ```batch
   # 在Windows命令行或者直接点击桌面图标
   C:\msys64\msys2_shell.cmd -mingw64
   ```

   **重要**：确保使用的是**MinGW64**环境，不是MSYS2或UCRT64。

2. **切换到项目目录**：
   ```bash
   cd /d/workspace/udp2docker
   
   # 验证当前位置
   pwd
   ls -la
   ```

   你应该看到项目文件如 `CMakeLists.txt`、`include/`、`src/` 等目录。

### 步骤2: 自动环境设置（推荐）

运行一键设置脚本，这会自动安装所有必要的工具：

```bash
# 设置脚本权限
chmod +x scripts/setup-msys2-env.sh

# 运行环境设置脚本
./scripts/setup-msys2-env.sh
```

这个脚本会：
- ✅ 检查MSYS2环境
- ✅ 自动安装编译器和构建工具
- ✅ 配置环境变量
- ✅ 测试编译功能
- ✅ 创建有用的别名

**等待脚本完成**，如果一切正常，你会看到 "🎉 MSYS2环境配置成功！" 的消息。

### 步骤3: 重新加载环境

```bash
# 重新加载环境配置
source ~/.bashrc

# 或者关闭终端重新打开
```

### 步骤4: 执行构建

现在开始实际构建：

```bash
# 设置构建脚本权限
chmod +x scripts/build-msys2.sh

# 开始构建（Release版本）
./scripts/build-msys2.sh Release
```

**构建过程说明**：
- 📦 CMake配置阶段（生成构建文件）
- 🔨 编译阶段（编译源代码）
- 🔗 链接阶段（生成可执行文件和库）

**预期输出示例**：
```
====================================
UDP2Docker MSYS2构建脚本
====================================
当前环境: MINGW64
构建类型: Release

检查工具可用性...
✅ 找到 gcc: gcc (Rev2, Built by MSYS2 project) 13.2.0
✅ 找到 cmake: cmake version 3.28.1
✅ 找到 mingw32-make: GNU Make 4.4.1

配置项目...
-- The CXX compiler identification is GNU 13.2.0
-- Configuring done (2.1s)
-- Generating done (0.1s)

开始构建...
[ 10%] Building CXX object CMakeFiles/udp2docker_lib.dir/src/config_manager.cpp.obj
[ 20%] Building CXX object CMakeFiles/udp2docker_lib.dir/src/logger.cpp.obj
[ 30%] Building CXX object CMakeFiles/udp2docker_lib.dir/src/message_protocol.cpp.obj
[ 40%] Building CXX object CMakeFiles/udp2docker_lib.dir/src/udp_client.cpp.obj
[ 50%] Linking CXX static library lib\libudp2docker_lib.a
[ 60%] Building CXX object CMakeFiles/udp2docker.dir/examples/main.cpp.obj
[ 70%] Linking CXX executable bin\udp2docker.exe
[ 80%] Building CXX object CMakeFiles/test_udp2docker.dir/tests/test_main.cpp.obj
[ 90%] Linking CXX executable bin\test_udp2docker.exe
[100%] Built target test_udp2docker

🎉 构建成功完成！
```

### 步骤5: 验证构建结果

```bash
# 设置验证脚本权限并运行
chmod +x scripts/verify-build.sh
./scripts/verify-build.sh
```

你应该看到：
```
✅ 主程序存在: build/bin/udp2docker.exe
✅ 测试程序存在: build/bin/test_udp2docker.exe  
✅ 静态库存在: build/lib/libudp2docker_lib.a
✅ 程序可以正常启动
✅ 单元测试通过
```

### 步骤6: 首次运行测试

```bash
# 运行单元测试
echo "运行单元测试..."
./build/bin/test_udp2docker.exe

# 运行主程序（基本功能测试）
echo "测试主程序..."
./build/bin/udp2docker.exe
```

## 🐳 Docker集成测试（可选）

如果你安装了Docker，可以测试完整的UDP通信：

```bash
# 启动Docker服务器
docker-compose up -d

# 等待几秒钟让服务器启动
sleep 5

# 再次运行客户端，这次会连接到Docker服务器
./build/bin/udp2docker.exe

# 查看Docker日志
docker-compose logs udp-server

# 停止Docker服务器
docker-compose down
```

## ❌ 遇到问题？

### 快速诊断

如果构建过程中遇到任何问题：

```bash
# 运行诊断脚本
chmod +x scripts/troubleshoot.sh
./scripts/troubleshoot.sh
```

### 常见问题和解决方案

#### 问题1: "command not found" 错误
```bash
# 解决方案：确保在正确的环境中
echo $MSYSTEM  # 应该显示 MINGW64
export PATH="/mingw64/bin:$PATH"
```

#### 问题2: 权限错误
```bash
# 解决方案：设置所有脚本权限
chmod +x scripts/*.sh
```

#### 问题3: 编译器找不到
```bash
# 解决方案：重新运行环境设置
./scripts/setup-msys2-env.sh
```

#### 问题4: 链接错误
```bash
# 解决方案：清理并重新构建
rm -rf build/
./scripts/build-msys2.sh Release
```

### 从头开始

如果需要完全重新开始：

```bash
# 清理构建文件
rm -rf build/

# 重新设置环境
./scripts/setup-msys2-env.sh

# 重新构建
source ~/.bashrc
./scripts/build-msys2.sh Release
```

## 🎯 成功标志

构建成功后，你应该拥有：

```
udp2docker/
├── build/
│   ├── bin/
│   │   ├── udp2docker.exe        # ✅ 主程序
│   │   └── test_udp2docker.exe   # ✅ 测试程序
│   └── lib/
│       └── libudp2docker_lib.a   # ✅ 静态库
├── config/
│   └── default.ini               # ✅ 配置文件
└── docker/                       # ✅ Docker文件
```

## 🎉 恭喜！

如果你看到所有的✅标志，说明构建成功！你现在可以：

1. **开发和修改代码** - 修改后重新运行 `./scripts/build-msys2.sh Release`
2. **运行测试** - 使用 `./build/bin/test_udp2docker.exe`
3. **使用库功能** - 参考 `examples/main.cpp` 中的示例
4. **Docker集成** - 使用 `docker-compose up -d` 启动服务器

## 📚 下一步

- 📖 阅读 `README.md` 了解项目详细信息
- 🔍 查看 `examples/main.cpp` 学习如何使用API
- 🛠️ 修改 `config/default.ini` 调整配置
- 🐳 探索Docker集成功能

**需要帮助？** 查看项目文档或运行诊断脚本！ 