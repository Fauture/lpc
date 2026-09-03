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
    'write(sprintf("d1=%d\\n", 3*1.5))',
    'write(sprintf("d2=%d\\n", 1.5*3))',
    'write(sprintf("f3=%f\\n", 3+1.5))',
    'write(sprintf("f4=%f\\n", 1.5+3))',
    'write(sprintf("f5=%f\\n", 3-1.5))',
    'write(sprintf("f6=%f\\n", 1.5-3))',
    'write(sprintf("f7=%f\\n", 5.0/2))',
    'write(sprintf("d8=%d\\n", 5/2))',
    'write(sprintf("d9=%d\\n", 3.9))',
    'write(sprintf("f10=%f\\n", 3.9))',
    'write(sprintf("d11=%d\\n", 7%3))',
]
for t in tests:
    s.sendall((t + '\r\n').encode())
    time.sleep(1.0)
    out = read()
    print('==>', t[:60])
    for line in out.split(b'\r\n'):
        if line.strip(): print('   ', line)
s.close()
