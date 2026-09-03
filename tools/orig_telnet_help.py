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
read()  # banner
s.sendall(b'123456\r\n')
time.sleep(1)
print('pwd:', read()[-100:])
s.sendall(b'help\r\n')
time.sleep(1.5)
print('help:', read())
s.sendall(b'?\r\n')
time.sleep(1.5)
print('?:', read())
s.close()
