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
    # type probe: if (3*1.5) is int 4 -> int+real left int -> trunc(4.5)=4.0 ; if real 4.0 -> 4.5
    'write(sprintf("t1=%f\\n", (3*1.5)+0.5))',
    'write(sprintf("t2=%f\\n", (3*1.2)+0.9))',
    # negative truncation
    'write(sprintf("n1=%d\\n", -3*1.5))',
    'write(sprintf("n2=%f\\n", -3*1.5))',
    'write(sprintf("n3=%d\\n", -3.9))',
    'write(sprintf("n4=%f\\n", -3+1.5))',
    'write(sprintf("n5=%d\\n", -3+1.5))',
    # exponent-like: 2*0.5 int left -> 1.0
    'write(sprintf("m1=%f\\n", 2*0.5))',
    'write(sprintf("m2=%d\\n", 10*0.1))',
]
for t in tests:
    s.sendall((t + '\r\n').encode())
    time.sleep(1.0)
    out = read()
    print('==>', t[:60])
    for line in out.split(b'\r\n'):
        if line.strip(): print('   ', line)
s.close()
