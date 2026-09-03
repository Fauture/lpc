# -*- coding: utf-8 -*-
import socket, sys, time
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
s = socket.create_connection(('192.168.200.128', 1024), timeout=8)
s.settimeout(2)
def read():
    buf = b''
    try:
        while True:
            d = s.recv(4096)
            if not d: break
            buf += d
    except socket.timeout:
        pass
    return buf
read()
s.sendall(b'123456\r\n')
time.sleep(1); read()
tests = [
    'write("MARK1\\n")',
    'sprintf("%d", 3*1.2)',
    'sprintf("%f", 3*1.2)',
    'int a; a = 3*1.2; write(sprintf("A=%d\\n", a))',
    'write(sprintf("F=%f\\n", 3*1.2))',
]
for t in tests:
    s.sendall((t + '\r\n').encode())
    time.sleep(1.2)
    out = read()
    print('==>', t)
    print('   ', out[:300])
s.close()
