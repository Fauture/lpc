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
    'write(sprintf("u1=%f\\n", -3.9))',
    'write(sprintf("u2=%f\\n", -(2.5)))',
    'write(sprintf("u3=%f\\n", -0.0))',
    'float f; f=2.5; write(sprintf("u4=%f\\n", -f))',
    'int k; k=5; write(sprintf("u5=%f\\n", -k))',
    'write(sprintf("u6=%f\\n", -(3*1.5)))',
    'write(sprintf("u7=%f\\n", 0.1+0.2))',
    'write(sprintf("u8=%d\\n", 0.1+0.2))',
    'write(sprintf("u9=%d\\n", (0.1+0.2)==0.3))',
]
for t in tests:
    s.sendall((t + '\r\n').encode())
    time.sleep(0.9)
    out = read()
    print('==>', t[:60])
    for line in out.split(b'\r\n'):
        if line.strip(): print('   ', line)
s.close()
