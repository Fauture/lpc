# -*- coding: utf-8 -*-
"""
gameshell LPC 驱动 RPC 客户端
================================================================
目标: 对 IDA 载入的服务端文件 magic_Linux32x 进行 RPC 连接通讯。

逆向结论(IDA + ida-pro-mcp 分析):
  * 服务端为 gameshell LPC 驱动, 版本 1.822 (LPC_DRIVER=1822)
  * 自定义 VM(yacc/lex), LPC 风格脚本语言, 函数需先声明
  * 二进制内嵌完整 RPC 子系统 (vm_rpc.c):
        - 线协议: magic 0x33333333 + type(0/1/2/3) + len + 混淆 body
        - type: 0=执行脚本 1=登录 2=LZSS压缩脚本 3=关闭
        - body 混淆: dec[i]=(i^enc[i])+123  即 enc[i]=((dec[i]+133)&0xFF)^i
        - 脚本可调用 rpc_call / rpc_info / rpc_pass_auth / rpc_set_port ...
  * 但当前运行实例 RPC 未启用 (rpc_info() 返回空数组, rpcMaxClient=0,
    无 -x 启动参数), 故不存在独立 RPC 端口
  * 唯一可用远程接口 = telnet shell (端口 1024, 密码 123456):
        连接后 "Welcome to shell of VM." -> "Input password:"
        认证后进入 "Shell> " REPL, 逐行求值 LPC 表达式
        write()/printf()/sprintf() 输出回送连接

因此本客户端通过 telnet shell 通道发送 LPC 脚本表达式,
实现远程过程调用 (远程执行任意 VM 脚本并取回输出)。

编码约定 (重要):
  * 服务端 LPC 字符串是 GBK 字节流; 发送方向本客户端默认按 GBK 编码
    (encoding='gbk'), call() 直接支持中文, 不要再按 latin1 发送.
  * 接收方向 read_until() 仍按 latin1 逐字节解码 (每个字节一个字符),
    调用方如需还原中文, 用 s.encode('latin1').decode('gbk') 还原
    (见 send_item_gui.py 的 fix_mojibake).
================================================================
"""

import socket
import time
import sys


class GameShellRpcError(Exception):
    pass


class GameShellRpcClient:
    """通过 telnet Shell> 通道对 gameshell 服务端做 RPC 调用。"""

    # telnet 协议字节
    IAC = 0xFF
    WILL, WONT = 0xFB, 0xFC
    DO, DONT = 0xFD, 0xFE
    SB, SE = 0xFA, 0xF0

    PROMPT = b'Shell> '              # 认证后 REPL 提示符
    PASS_PROMPT = b'Input password:' # 认证前提示

    def __init__(self, host='192.168.200.128', port=1024, password='123456',
                 timeout=10.0, debug=False, encoding='gbk'):
        self.host = host
        self.port = port
        self.password = password
        self.timeout = timeout
        self.debug = debug
        self.encoding = encoding  # 发送编码: 默认 gbk (服务端字符串为 GBK 字节流)
        self.sock = None
        self._visible = bytearray()  # 已剥离 telnet 协商的可见字节
        self._pending = bytearray()   # 末尾未决的原始字节(可能是不完整 IAC 序列)

    # ---------- 连接 / 认证 ----------
    def connect(self):
        self.sock = socket.create_connection((self.host, self.port), self.timeout)
        self.sock.settimeout(self.timeout)
        banner = self.read_until(self.PASS_PROMPT, '等待密码提示')
        # 服务端 WILL ECHO 会回显输入, 直接发密码
        self._send(self.password + '\r\n')
        welcome = self.read_until(self.PROMPT, '等待 Shell> 提示符')
        if 'New telnet' not in welcome and 'Shell' not in welcome:
            raise GameShellRpcError('认证失败或未进入 Shell: %r' % welcome)
        return banner, welcome

    def close(self):
        if self.sock:
            try:
                self._send('exit\r\n')
            except Exception:
                pass
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None

    # ---------- 核心 RPC 调用 ----------
    def call(self, expr, wait=3.0):
        """
        发送一行 LPC 表达式/语句, 返回 Shell> 之间的输出文本 (已剥离 telnet 协商).
        例:
            rpc.call('write("hello")')                       -> 'hello'
            rpc.call('write(sprintf("%O", uptime()))')
            rpc.call('write(sprintf("%O", get_system_info()))')
        """
        line = expr.rstrip('\r\n')
        self._send(line + '\r\n')
        return self.read_until(self.PROMPT, '等待执行结果', max_wait=wait)

    def eval(self, expr, wait=3.0):
        """便捷封装: 求值表达式并以 %O 格式打印其值 (适合查看任意 LPC 值)."""
        return self.call('write(sprintf("%%O\\n", (%s)))' % expr, wait=wait)

    def call_gbk(self, expr, wait=8.0):
        """按 GBK 直发(兼容旧调用方): 与 call() 等价, 发送编码已默认 gbk."""
        return self.call(expr, wait=wait)

    # ---------- 底层 IO ----------
    def _send(self, s):
        # 关键: 服务端 LPC 字符串为 GBK 字节流, 必须按 GBK 编码发送.
        # 切勿用 latin1 —— 中文会被替换成 '?' 造成数据损坏.
        data = s.encode(self.encoding, 'replace') if isinstance(s, str) else s
        if self.debug:
            sys.stderr.write('>> %r\n' % data)
        self.sock.sendall(data)

    def _recv_more(self):
        try:
            chunk = self.sock.recv(8192)
        except socket.timeout:
            return False
        if not chunk:
            raise GameShellRpcError('连接被服务端关闭')
        self._pending += chunk
        self._strip_pending()
        return True

    def _strip_pending(self):
        """对 _pending 运行状态机, 完整可见字节移入 _visible, 不完整 IAC 留在 _pending."""
        i = 0
        n = len(self._pending)
        out = self._visible
        while i < n:
            b = self._pending[i]
            if b == self.IAC:
                if i + 1 >= n:
                    break  # IAC 不完整
                cmd = self._pending[i + 1]
                if cmd == self.IAC:               # IAC IAC -> 字面 0xFF
                    out.append(0xFF); i += 2; continue
                if cmd == self.SB:                # 子协商 IAC SB ... IAC SE
                    j = i + 2; ok = False
                    while j < n:
                        if self._pending[j] == self.IAC and j + 1 < n and self._pending[j + 1] == self.SE:
                            j += 2; ok = True; break
                        j += 1
                    if not ok:
                        break  # 子协商未结束, 留待下次
                    i = j; continue
                if cmd in (self.WILL, self.WONT, self.DO, self.DONT):
                    if i + 2 >= n:
                        break  # 三字节协商不完整
                    # 策略: 不主动应答, 服务端不依赖应答即可继续
                    i += 3; continue
                # 其他两字节命令 (如 NOP, GA 等)
                i += 2; continue
            out.append(b); i += 1
        self._pending = self._pending[i:]

    def read_until(self, marker, desc='', max_wait=8.0):
        """
        读取直到 marker (bytes) 在可见流中出现, 返回 marker 之前的可见文本 (str).
        marker 为 None 时, 返回当前全部可见文本.
        """
        if marker is None:
            self._recv_drain(max_wait)
            text = bytes(self._visible).decode('latin1')
            self._visible = bytearray()
            return text

        idx = self._visible.find(marker)
        if idx >= 0:
            text = bytes(self._visible[:idx]).decode('latin1')
            del self._visible[:idx + len(marker)]
            return text

        deadline = time.time() + max_wait
        while time.time() < deadline:
            if not self._recv_more():
                break
            idx = self._visible.find(marker)
            if idx >= 0:
                text = bytes(self._visible[:idx]).decode('latin1')
                del self._visible[:idx + len(marker)]
                return text
        # 超时: 返回当前已读可见文本
        text = bytes(self._visible).decode('latin1')
        self._visible = bytearray()
        if self.debug:
            sys.stderr.write('!! 超时(%s), 未找到 %r\n' % (desc, marker))
        return text

    def _recv_drain(self, max_wait):
        """尽力读取 max_wait 内能读到的所有数据."""
        deadline = time.time() + max_wait
        while time.time() < deadline:
            if not self._recv_more():
                break


# ===========================================================================
# 演示
# ===========================================================================
def fix_mojibake(s):
    """接收侧按 latin1 逐字节解码的 GBK 输出, 还原成中文."""
    try:
        return s.encode('latin1', 'replace').decode('gbk', 'replace')
    except Exception:
        return s


def main():
    rpc = GameShellRpcClient(host='192.168.200.128', port=1024, password='123456', debug=False)
    print('[*] 连接 192.168.200.128:1024 ...')
    banner, welcome = rpc.connect()
    print('[+] 已认证, 进入 Shell>')
    print('--- banner ---');  print(banner)
    print('--- welcome ---'); print(welcome)

    demos = [
        ('hello',        'write("hello world")'),
        ('中文直发',     'write("中文测试abc")'),
        ('系统信息',     'write(sprintf("%O", get_system_info()))'),
        ('运行时长',     'write(sprintf("%O\\n", uptime()))'),
        ('RPC 客户端',   'write(sprintf("%O\\n", rpc_info()))'),
        ('所有线程',     'write(sprintf("%O\\n", ps()))'),
        ('当前目录文件', 'write(sprintf("%O\\n", get_dir("/home/1.6/aaa/*")))'),
    ]
    for title, expr in demos:
        print('\n========== %s ==========' % title)
        print('>>> %s' % expr)
        try:
            out = rpc.call(expr, wait=5.0)
            print(fix_mojibake(out))
        except Exception as e:
            print('ERR:', e)

    rpc.close()
    print('\n[*] 已断开')


if __name__ == '__main__':
    main()
