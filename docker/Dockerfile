# UDP2Docker服务器容器
# 用于接收来自Windows宿主机的UDP消息

FROM ubuntu:22.04

LABEL maintainer="UDP2Docker Team"
LABEL description="UDP Server for receiving messages from Windows host"

# 安装必要的包
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    netcat-openbsd \
    iproute2 \
    && rm -rf /var/lib/apt/lists/*

# 创建应用目录
WORKDIR /app

# 复制UDP服务器脚本
COPY udp_server.py /app/
COPY start.sh /app/

# 设置权限
RUN chmod +x /app/start.sh

# 创建日志目录
RUN mkdir -p /app/logs

# 暴露UDP端口
EXPOSE 8888/udp

# 设置环境变量
ENV UDP_PORT=8888
ENV UDP_HOST=0.0.0.0
ENV LOG_LEVEL=INFO

# 启动服务器
CMD ["/app/start.sh"] 