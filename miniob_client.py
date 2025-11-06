"""
MiniOB 数据库客户端
用于连接和操作 MiniOB 数据库
"""

import socket
from typing import List

class MiniOBClient:
    """MiniOB 数据库客户端"""
    
    def __init__(self, host='127.0.0.1', port=6789, timeout=30):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.sock = None
        self._connected = False
    
    def connect(self):
        """建立与 MiniOB 的连接"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(self.timeout)
            self.sock.connect((self.host, self.port))
            self._connected = True
            print(f"✓ Connected to MiniOB at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"✗ Failed to connect to MiniOB: {e}")
            self._connected = False
            return False
    
    def is_connected(self):
        """检查是否已连接"""
        return self._connected and self.sock is not None
    
    def execute(self, sql: str) -> str:
        """
        执行 SQL 语句
        
        Args:
            sql: SQL 语句字符串
            
        Returns:
            MiniOB 返回的结果字符串
        """
        if not self.is_connected():
            if not self.connect():
                raise ConnectionError("Cannot connect to MiniOB")
        
        try:
            # 发送 SQL（MiniOB 协议：SQL + \0 结束符）
            sql_bytes = (sql.strip() + '\0').encode('utf-8')
            self.sock.sendall(sql_bytes)
            
            # 接收响应（直到遇到 \0）
            response = b''
            while True:
                try:
                    chunk = self.sock.recv(8192)
                    if not chunk:
                        break
                    response += chunk
                    
                    # 检查是否接收完整（遇到 \0 结束符）
                    if b'\0' in chunk:
                        break
                        
                except socket.timeout:
                    break
            
            # 解码并移除结束符
            result = response.decode('utf-8', errors='ignore')
            result = result.rstrip('\0')
            return result
            
        except Exception as e:
            print(f"✗ SQL execution error: {e}")
            self._connected = False
            raise
    
    def execute_query(self, sql: str) -> List[List[str]]:
        """
        执行查询并解析结果为二维数组
        
        Args:
            sql: SELECT 查询语句
            
        Returns:
            结果行列表，每行是字符串列表
        """
        result = self.execute(sql)
        rows = []
        
        # 解析 MiniOB 返回格式
        # 假设格式类似：value1 | value2 | value3
        for line in result.strip().split('\n'):
            line = line.strip()
            if not line:
                continue
            # 跳过状态行和分隔符
            if line.startswith('SUCCESS') or line.startswith('FAILURE'):
                continue
            if line.startswith('---') or line.startswith('==='):
                continue
            if '|' in line:
                # 解析以 | 分隔的行
                values = [v.strip() for v in line.split('|')]
                rows.append(values)
            elif line and not line.startswith('('):
                # 单值行
                rows.append([line])
        
        return rows
    
    def close(self):
        """关闭连接"""
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
            self.sock = None
            self._connected = False
            print("✓ MiniOB connection closed")
    
    def __enter__(self):
        """支持 with 语句"""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """支持 with 语句"""
        self.close()
        return False


# 辅助函数
def vector_to_string(vector: List[float]) -> str:
    """将向量转换为 MiniOB 接受的字符串格式 [1.0,2.0,3.0]"""
    # 使用 2 位小数精度以减少 SQL 长度（从 20KB 降到 5KB）
    return '[' + ','.join([f"{float(x):.2f}" for x in vector]) + ']'


def escape_sql_string(s: str) -> str:
    """转义 SQL 字符串中的特殊字符"""
    if s is None:
        return ''
    # MiniOB 不支持 '' 转义，直接移除单引号
    return s.replace("'", "").replace('\n', ' ').replace('\r', ' ').replace('\t', ' ')
