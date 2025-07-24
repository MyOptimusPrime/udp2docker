#!/usr/bin/env python3
"""
UDP2Docker服务器
用于接收来自Windows宿主机的UDP消息并进行处理
"""

import socket
import struct
import json
import logging
import os
import sys
import signal
import threading
import time
from datetime import datetime
from typing import Dict, Any, Optional, Tuple

# 配置日志
logging.basicConfig(
    level=getattr(logging, os.environ.get('LOG_LEVEL', 'INFO')),
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler('/app/logs/udp_server.log')
    ]
)

logger = logging.getLogger(__name__)

class MessageProtocol:
    """简单的消息协议处理器"""
    
    MAGIC_NUMBER = 0x55AA55AA
    HEADER_SIZE = 32
    
    @staticmethod
    def parse_header(data: bytes) -> Optional[Dict[str, Any]]:
        """解析消息头部"""
        if len(data) < MessageProtocol.HEADER_SIZE:
            return None
            
        try:
            # 解包头部数据
            magic, version, msg_type, priority, seq_id, timestamp, payload_size, checksum = \
                struct.unpack('<IHHIIIII', data[:MessageProtocol.HEADER_SIZE])
            
            if magic != MessageProtocol.MAGIC_NUMBER:
                return None
                
            return {
                'magic': magic,
                'version': version,
                'type': msg_type,
                'priority': priority,
                'sequence_id': seq_id,
                'timestamp': timestamp,
                'payload_size': payload_size,
                'checksum': checksum
            }
        except struct.error:
            return None
    
    @staticmethod
    def get_message_type_name(msg_type: int) -> str:
        """获取消息类型名称"""
        types = {
            1: "HEARTBEAT",
            2: "DATA", 
            3: "CONTROL",
            4: "RESPONSE",
            5: "MESSAGE_ERROR"
        }
        return types.get(msg_type, "UNKNOWN")
    
    @staticmethod
    def get_priority_name(priority: int) -> str:
        """获取优先级名称"""
        priorities = {
            1: "LOW",
            2: "NORMAL", 
            3: "HIGH",
            4: "CRITICAL"
        }
        return priorities.get(priority, "UNKNOWN")

class UDPServer:
    """UDP服务器类"""
    
    def __init__(self, host: str = '0.0.0.0', port: int = 8888):
        self.host = host
        self.port = port
        self.socket = None
        self.running = False
        self.stats = {
            'messages_received': 0,
            'bytes_received': 0,
            'errors': 0,
            'start_time': None
        }
        
    def start(self):
        """启动UDP服务器"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket.bind((self.host, self.port))
            
            logger.info(f"UDP服务器启动成功，监听 {self.host}:{self.port}")
            
            self.running = True
            self.stats['start_time'] = datetime.now()
            
            # 启动统计线程
            stats_thread = threading.Thread(target=self._stats_worker, daemon=True)
            stats_thread.start()
            
            # 主监听循环
            self._listen_loop()
            
        except Exception as e:
            logger.error(f"启动UDP服务器失败: {e}")
            sys.exit(1)
    
    def stop(self):
        """停止UDP服务器"""
        logger.info("正在关闭UDP服务器...")
        self.running = False
        
        if self.socket:
            self.socket.close()
            
        logger.info("UDP服务器已关闭")
    
    def _listen_loop(self):
        """主要的监听循环"""
        logger.info("开始监听UDP消息...")
        
        while self.running:
            try:
                # 接收数据
                data, addr = self.socket.recvfrom(65536)
                
                self.stats['messages_received'] += 1
                self.stats['bytes_received'] += len(data)
                
                # 处理消息
                self._handle_message(data, addr)
                
            except socket.error as e:
                if self.running:  # 只有在运行时才记录错误
                    logger.error(f"接收UDP消息时出错: {e}")
                    self.stats['errors'] += 1
            except Exception as e:
                logger.error(f"处理消息时出现未预期的错误: {e}")
                self.stats['errors'] += 1
    
    def _handle_message(self, data: bytes, addr: Tuple[str, int]):
        """处理接收到的消息"""
        client_ip, client_port = addr
        
        logger.info(f"收到来自 {client_ip}:{client_port} 的消息，大小: {len(data)} 字节")
        
        # 尝试解析协议消息
        header = MessageProtocol.parse_header(data)
        
        if header:
            # 这是一个协议消息
            msg_type = MessageProtocol.get_message_type_name(header['type'])
            priority = MessageProtocol.get_priority_name(header['priority'])
            
            logger.info(f"协议消息 - 类型: {msg_type}, 优先级: {priority}, 序列号: {header['sequence_id']}")
            
            # 提取负载数据
            payload_start = MessageProtocol.HEADER_SIZE
            payload_end = payload_start + header['payload_size']
            
            if len(data) >= payload_end:
                payload = data[payload_start:payload_end]
                self._process_payload(payload, header, addr)
            else:
                logger.warning("消息负载大小不匹配")
        else:
            # 这是一个普通的文本消息
            try:
                message = data.decode('utf-8')
                logger.info(f"文本消息: {message}")
                self._process_text_message(message, addr)
            except UnicodeDecodeError:
                logger.info(f"二进制消息，大小: {len(data)} 字节")
                self._process_binary_message(data, addr)
    
    def _process_payload(self, payload: bytes, header: Dict[str, Any], addr: Tuple[str, int]):
        """处理协议消息的负载"""
        msg_type = header['type']
        
        if msg_type == 1:  # HEARTBEAT
            logger.debug("收到心跳消息")
            self._send_response("heartbeat_ack", addr)
            
        elif msg_type == 2:  # DATA
            try:
                message = payload.decode('utf-8')
                logger.info(f"数据消息内容: {message}")
                self._send_response("data_received", addr)
            except UnicodeDecodeError:
                logger.info(f"二进制数据消息，大小: {len(payload)} 字节")
                
        elif msg_type == 3:  # CONTROL
            try:
                command = payload.decode('utf-8')
                logger.info(f"控制命令: {command}")
                self._process_control_command(command, addr)
            except UnicodeDecodeError:
                logger.warning("无法解析控制命令")
                
        elif msg_type == 4:  # RESPONSE
            logger.info("收到响应消息")
            
        elif msg_type == 5:  # MESSAGE_ERROR
            try:
                error_msg = payload.decode('utf-8')
                logger.warning(f"错误消息: {error_msg}")
            except UnicodeDecodeError:
                logger.warning("收到二进制错误消息")
    
    def _process_text_message(self, message: str, addr: Tuple[str, int]):
        """处理文本消息"""
        # 尝试解析为JSON
        try:
            json_data = json.loads(message)
            logger.info(f"JSON消息: {json_data}")
        except json.JSONDecodeError:
            logger.info(f"普通文本消息: {message}")
        
        # 发送确认回复
        self._send_response(f"收到消息: {message[:50]}...", addr)
    
    def _process_binary_message(self, data: bytes, addr: Tuple[str, int]):
        """处理二进制消息"""
        logger.info(f"处理二进制消息，大小: {len(data)} 字节")
        
        # 可以在这里添加特定的二进制消息处理逻辑
        # 例如：文件传输、图像数据等
    
    def _process_control_command(self, command: str, addr: Tuple[str, int]):
        """处理控制命令"""
        logger.info(f"执行控制命令: {command}")
        
        # 模拟执行Docker命令
        if command.startswith('docker'):
            response = f"模拟执行: {command}\n状态: 成功\n时间: {datetime.now().isoformat()}"
        else:
            response = f"未知命令: {command}"
        
        self._send_response(response, addr)
    
    def _send_response(self, message: str, addr: Tuple[str, int]):
        """发送响应消息"""
        try:
            response_data = message.encode('utf-8')
            self.socket.sendto(response_data, addr)
            logger.debug(f"发送响应到 {addr[0]}:{addr[1]}: {message[:50]}...")
        except Exception as e:
            logger.error(f"发送响应失败: {e}")
    
    def _stats_worker(self):
        """统计信息工作线程"""
        while self.running:
            time.sleep(30)  # 每30秒输出一次统计信息
            
            if self.stats['start_time']:
                uptime = datetime.now() - self.stats['start_time']
                logger.info(f"服务器统计 - 运行时间: {uptime}, "
                           f"收到消息: {self.stats['messages_received']}, "
                           f"接收字节: {self.stats['bytes_received']}, "
                           f"错误: {self.stats['errors']}")

def signal_handler(signum, frame):
    """信号处理器"""
    logger.info(f"收到信号 {signum}，准备关闭服务器")
    if 'server' in globals():
        server.stop()
    sys.exit(0)

def main():
    """主函数"""
    # 注册信号处理器
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    
    # 从环境变量获取配置
    host = os.environ.get('UDP_HOST', '0.0.0.0')
    port = int(os.environ.get('UDP_PORT', 8888))
    
    logger.info("启动UDP2Docker服务器...")
    logger.info(f"配置 - 主机: {host}, 端口: {port}")
    
    # 创建并启动服务器
    global server
    server = UDPServer(host, port)
    
    try:
        server.start()
    except KeyboardInterrupt:
        logger.info("收到中断信号")
        server.stop()
    except Exception as e:
        logger.error(f"服务器运行异常: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 