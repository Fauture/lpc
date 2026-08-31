# -*- coding: utf-8 -*-
"""Check timer state via telnet shell."""
import socket
import sys
import time

s = socket.create_connection(('127.0.0.1', 9110), timeout=10)
s.settimeout(0.5)

def drain(ms=800):
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
    return out

drain(500)
s.sendall(b'123456\n')
drain(600)

cmds = [
    b'get_timer("sync_data")',
    b'get_timer("schGroupChatSave")',
    b'get_timer(0)',
]
for c in cmds:
    s.sendall(c + b'\n')
    time.sleep(1.5)
    out = drain(2500)
    sys.stdout.buffer.write(b'CMD: ' + c + b'\n')
    sys.stdout.buffer.write(out[:600] + b'\n---\n')
    sys.stdout.buffer.flush()
s.sendall(b'.\n')
drain(300)
s.close()
