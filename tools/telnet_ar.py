# -*- coding: utf-8 -*-
"""Call auto_recover_read via ccs shell for several paths."""
import socket
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
    b'auto_recover_read("group_chat_index", "", "")',
    b'auto_recover_read("daemons", "SYS_PURCHASE_D", "")',
    b'auto_recover_read("daemons", "share_data", "")',
]
for c in cmds:
    s.sendall(c + b'\n')
    time.sleep(2.5)
    out = drain(4000)
    print('CMD:', c.decode())
    print(out.decode('utf-8', 'replace'))
    print('---')
s.sendall(b'.\n')
drain(300)
s.close()
