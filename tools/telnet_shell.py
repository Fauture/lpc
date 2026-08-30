# -*- coding: utf-8 -*-
"""Telnet to lpcvm driver shell, send a command, print responses."""
import socket
import sys
import time

host, port = '127.0.0.1', int(sys.argv[1])
cmds = sys.argv[2:]

s = socket.create_connection((host, port), timeout=10)
s.settimeout(0.5)
buf = b''

def drain(ms=800):
    global buf
    end = time.time() + ms / 1000.0
    out = b''
    while time.time() < end:
        try:
            d = s.recv(4096)
            if not d:
                break
            out += d
        except socket.timeout:
            pass
    if out:
        try:
            print(out.decode('utf-8', 'replace'), end='')
        except Exception:
            pass
    return out

drain(500)
s.sendall(b'123456\n')
drain(600)
for c in cmds:
    s.sendall(c.encode('utf-8') + b'\n')
    drain(1500)
s.sendall(b'.\n')
drain(300)
s.close()
