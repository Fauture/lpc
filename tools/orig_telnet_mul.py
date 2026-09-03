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
    'write(sprintf("A intp=%d floatp=%d\\n", intp(3*1.2), floatp(3*1.2)))',
    'write(sprintf("B=%f\\n", 2.5*2))',
    'write(sprintf("C=%f\\n", 3*1.5))',
    'write(sprintf("D=%f\\n", 1.5*1.5))',
    'write(sprintf("E intp=%d floatp=%d\\n", intp(2.5*2), floatp(2.5*2)))',
    'write(sprintf("F intp=%d floatp=%d\\n", intp(3*1.5), floatp(3*1.5)))',
    'write(sprintf("G=%f\\n", 1.2*3))',
    'write(sprintf("H=%f\\n", 0.5*2))',
]
for t in tests:
    s.sendall((t + '\r\n').encode())
    time.sleep(1.0)
    out = read()
    print('==>', t[:60])
    for line in out.split(b'\r\n'):
        if line.strip(): print('   ', line)
s.close()
