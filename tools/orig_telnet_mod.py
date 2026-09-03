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
    'write(sprintf("mrl=%f\\n", 2.5%1))',
    'write(sprintf("mrl2=%f\\n", 2.5%1.0))',
    'write(sprintf("negm=%d\\n", -7%3))',
    'write(sprintf("negm2=%f\\n", -7%2.0))',
    'write(sprintf("big=%d\\n", 1000000*1000000))',
    'write(sprintf("intmul=%d\\n", 6*7))',
    'write(sprintf("divneg=%f\\n", -5/2.0))',
    'write(sprintf("adds=%s\\n", "ab"+1))',
]
for t in tests:
    s.sendall((t + '\r\n').encode())
    time.sleep(0.9)
    out = read()
    print('==>', t[:60])
    for line in out.split(b'\r\n'):
        if line.strip(): print('   ', line)
s.close()
