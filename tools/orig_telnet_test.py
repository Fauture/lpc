# -*- coding: utf-8 -*-
import socket, sys, time
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
s = socket.create_connection(('192.168.200.128', 1024), timeout=8)
s.settimeout(3)
# read banner
try:
    data = s.recv(4096)
    print('banner:', data[:200])
except socket.timeout:
    pass
s.sendall(b'123456\r\n')
time.sleep(1.5)
try:
    data = s.recv(8192)
    print('after pwd:', data[:300])
except socket.timeout:
    pass
# try an expression
s.sendall(b'3 * 1.2\r\n')
time.sleep(1.5)
try:
    data = s.recv(8192)
    print('expr:', data[:400])
except socket.timeout:
    pass
s.close()
