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
    'write(sprintf("e1=%d\\n", 1.0000001==1.0))',
    'write(sprintf("e2=%d\\n", 1.000001==1.0))',
    'write(sprintf("e3=%d\\n", 1.00001==1.0))',
    'write(sprintf("e4=%d\\n", 0.1+0.2==0.3))',
    'write(sprintf("e5=%d\\n", (1.0/3.0)*3.0==1.0))',
    'write(sprintf("e6=%f\\n", 1.0/3.0))',
    'float a; float b; a=0.1; b=0.2; write(sprintf("e7=%d\\n", a+b==0.3))',
    'write(sprintf("e8=%d\\n", 1.0/3.0==0.333333))',
    'write(sprintf("e9=%d\\n", 0.5==0.5))',
    'write(sprintf("e10=%f\\n", 100.0/3.0))',
    'write(sprintf("e11=%d\\n", 2.0==2))',
]
for t in tests:
    s.sendall((t + '\r\n').encode())
    time.sleep(0.9)
    out = read()
    print('==>', t[:60])
    for line in out.split(b'\r\n'):
        if line.strip(): print('   ', line)
s.close()
