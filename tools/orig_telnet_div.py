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
    'write(sprintf("dv=%f\\n", 5/2.0))',
    'write(sprintf("dv2=%d\\n", 5/2.0))',
    'write(sprintf("dv3=%f\\n", 5.0/2))',
    'write(sprintf("dv4=%f\\n", 5.0/2.5))',
    'write(sprintf("md=%f\\n", 7%2.0))',
    'write(sprintf("md2=%d\\n", 7%2.0))',
    'write(sprintf("md3=%d\\n", 5%2.0))',
    'write(sprintf("neg=%d\\n", -3.9*1))',
    'write(sprintf("sub=%f\\n", 5-1.5))',
    'write(sprintf("sub2=%d\\n", 5-1.5))',
]
for t in tests:
    s.sendall((t + '\r\n').encode())
    time.sleep(0.9)
    out = read()
    print('==>', t[:60])
    for line in out.split(b'\r\n'):
        if line.strip(): print('   ', line)
s.close()
